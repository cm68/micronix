/*
 * a small append-only key/value store
 *
 * lib/kv.c
 *
 * See kv.h for the format.  The index is an open-addressed hash of
 * (key, value offset, value length); the value itself stays in the log
 * and is read back on demand, so the index stays small no matter how
 * many sectors a format wrote.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "../include/kv.h"

#define NHASH 1021              /* the starting size; the table grows */

struct kvent {
    unsigned char *key;
    int klen;
    long off;                   /* the value's offset in the log */
    int vlen;
    int used;
};

struct kv {
    int fd;
    char *path;
    int nkeys;                  /* live entries in the index */
    int nput;                   /* records ever appended to the log */
    struct kvent *tab;          /* open-addressed hash, grown on demand */
    int ntab;                   /* slots in tab */
};

static unsigned long
hash(void *key, int klen)
{
    unsigned char *p = key;
    unsigned long h = 5381;
    int i;

    for (i = 0; i < klen; i++)
        h = h * 33 + p[i];
    return h;
}

static struct kvent *
find(struct kv *kv, void *key, int klen, int create)
{
    unsigned long h = hash(key, klen) % kv->ntab;
    int i;

    for (i = 0; i < kv->ntab; i++) {
        struct kvent *e = &kv->tab[(h + i) % kv->ntab];
        if (!e->used) {
            if (!create)
                return 0;
            e->key = malloc(klen);
            memcpy(e->key, key, klen);
            e->klen = klen;
            e->used = 1;
            kv->nkeys++;
            return e;
        }
        if (e->klen == klen && memcmp(e->key, key, klen) == 0)
            return e;
    }
    return 0;                   /* table full: not in a store this size */
}

/*
 * Widen the hash table: double it and move every live entry across.
 * The keys themselves stay put - only the slot pointers move.
 */
static void
grow(struct kv *kv)
{
    struct kvent *old = kv->tab;
    int oldsz = kv->ntab;
    int i;

    kv->ntab = oldsz * 2 + 1;
    kv->tab = calloc(kv->ntab, sizeof(*kv->tab));
    kv->nkeys = 0;
    for (i = 0; i < oldsz; i++) {
        struct kvent *e = &old[i];
        struct kvent *n;
        unsigned long h;
        int j;

        if (!e->used)
            continue;
        h = hash(e->key, e->klen) % kv->ntab;
        for (j = 0; j < kv->ntab; j++)
            if (!kv->tab[(h + j) % kv->ntab].used)
                break;
        n = &kv->tab[(h + j) % kv->ntab];
        *n = *e;                /* the key pointer moves across */
        kv->nkeys++;
    }
    free(old);
}

static struct kvent *
lookup(struct kv *kv, void *key, int klen, int create)
{
    struct kvent *e;

    if (create && kv->nkeys * 10 >= kv->ntab * 7)
        grow(kv);               /* keep the probe short */
    e = find(kv, key, klen, create);
    if (e || !create)
        return e;
    grow(kv);                   /* truly full: widen and retry */
    return find(kv, key, klen, create);
}

struct kv *
kv_open(char *path, int create)
{
    struct kv *kv;
    unsigned char hdr[3];
    int klen, vlen;
    long off;
    unsigned char *key;
    struct kvent *e;

    kv = calloc(1, sizeof(*kv));
    kv->ntab = NHASH;
    kv->tab = calloc(kv->ntab, sizeof(*kv->tab));
    kv->path = strdup(path);
    kv->fd = open(path, create ? (O_RDWR | O_CREAT) : O_RDONLY, 0777);
    if (kv->fd < 0) {
        free(kv->path);
        free(kv);
        return 0;
    }

    /* scan the log, rebuilding the index; a partial tail record drops */
    for (;;) {
        off = lseek(kv->fd, 0, SEEK_CUR);
        if (read(kv->fd, hdr, 3) != 3)
            break;
        klen = hdr[0];
        vlen = hdr[1] | (hdr[2] << 8);
        key = malloc(klen);
        if (read(kv->fd, key, klen) != klen) {
            free(key);
            break;
        }
        e = lookup(kv, key, klen, 1);
        free(key);
        e->vlen = vlen;
        e->off = lseek(kv->fd, 0, SEEK_CUR);
        if (lseek(kv->fd, vlen, SEEK_CUR) < 0)
            break;
        kv->nput++;
    }
    /* leave the append point at the end of the last whole record */
    lseek(kv->fd, off, SEEK_SET);
    return kv;
}

/*
 * Rewrite the log with only the latest value of each key, dropping the
 * stale records every put has left behind.  Done on close and whenever
 * the log has grown to twice the live keys, so an append-only store
 * stays bounded by what it actually holds.
 */
static int
compact(struct kv *kv)
{
    char tmp[512];
    int tfd, i;
    struct kvent *e;
    unsigned char hdr[3];
    void *val;

    snprintf(tmp, sizeof(tmp), "%s.tmp", kv->path);
    tfd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (tfd < 0)
        return -1;

    for (i = 0; i < kv->ntab; i++) {
        e = &kv->tab[i];
        if (!e->used)
            continue;
        val = malloc(e->vlen);
        lseek(kv->fd, e->off, SEEK_SET);
        if (read(kv->fd, val, e->vlen) != e->vlen) {
            free(val);
            close(tfd);
            return -1;
        }
        hdr[0] = e->klen;
        hdr[1] = e->vlen & 0xff;
        hdr[2] = (e->vlen >> 8) & 0xff;
        write(tfd, hdr, 3);
        write(tfd, e->key, e->klen);
        e->off = lseek(tfd, 0, SEEK_CUR);
        write(tfd, val, e->vlen);
        free(val);
    }
    fsync(tfd);
    close(tfd);

    close(kv->fd);
    if (rename(tmp, kv->path) < 0)
        return -1;
    kv->fd = open(kv->path, O_RDWR, 0777);
    if (kv->fd < 0)
        return -1;
    kv->nput = kv->nkeys;
    return 0;
}

void
kv_close(struct kv *kv)
{
    int i;

    if (!kv)
        return;
    if (kv->nput > kv->nkeys)
        compact(kv);
    for (i = 0; i < kv->ntab; i++)
        if (kv->tab[i].used)
            free(kv->tab[i].key);
    free(kv->tab);
    close(kv->fd);
    free(kv->path);
    free(kv);
}

int
kv_put(struct kv *kv, void *key, int klen, void *val, int vlen)
{
    struct kvent *e;
    unsigned char hdr[3];
    long off;

    e = lookup(kv, key, klen, 1);
    if (!e)
        return -1;
    hdr[0] = klen;
    hdr[1] = vlen & 0xff;
    hdr[2] = (vlen >> 8) & 0xff;
    off = lseek(kv->fd, 0, SEEK_END);
    if (write(kv->fd, hdr, 3) != 3 ||
        write(kv->fd, key, klen) != klen ||
        write(kv->fd, val, vlen) != vlen)
        return -1;
    e->off = off + 3 + klen;
    e->vlen = vlen;
    kv->nput++;
    if (kv->nput > 2 * kv->nkeys)
        compact(kv);
    return 0;
}

int
kv_get(struct kv *kv, void *key, int klen, void **val, int *vlen)
{
    struct kvent *e;
    void *buf;

    e = lookup(kv, key, klen, 0);
    if (!e)
        return 0;
    buf = malloc(e->vlen);
    lseek(kv->fd, e->off, SEEK_SET);
    if (read(kv->fd, buf, e->vlen) != e->vlen) {
        free(buf);
        return 0;
    }
    *val = buf;
    *vlen = e->vlen;
    return 1;
}

/*
 * Visit every live key once, in no particular order.  The value is
 * malloc'd fresh and freed after fn returns, so fn must copy what it
 * keeps.  This is how a delta is replayed onto a freshly loaded image.
 */
void
kv_foreach(struct kv *kv,
    void (*fn)(void *key, int klen, void *val, int vlen, void *arg),
    void *arg)
{
    int i;
    struct kvent *e;
    void *val;

    for (i = 0; i < kv->ntab; i++) {
        e = &kv->tab[i];
        if (!e->used)
            continue;
        val = malloc(e->vlen);
        lseek(kv->fd, e->off, SEEK_SET);
        if (read(kv->fd, val, e->vlen) != e->vlen) {
            free(val);
            continue;
        }
        fn(e->key, e->klen, val, e->vlen, arg);
        free(val);
    }
}
