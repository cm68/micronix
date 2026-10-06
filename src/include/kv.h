/*
 * a small append-only key/value store
 *
 * kv.h
 *
 * The delta overlay an IMD image is written through is an ordered key
 * store on disk: a log of records, each a key length, a value length,
 * the key and the value.  Putting appends a record; getting indexes
 * into the log and reads it back; nothing is ever rewritten, so a write
 * is durable the moment it lands and a half-written record at the tail
 * is simply dropped on the next open.  The index is an in-memory hash
 * rebuilt by scanning the log at open, so a store with a few hundred
 * changed sectors costs a few hundred bytes of index and one scan.
 *
 * Keys and values are opaque byte strings - what they mean is the
 * caller's business, which is what keeps the store free of any idea of
 * a disk's geometry.  A get that finds nothing returns 0 and leaves the
 * caller's buffer alone.
 */

struct kv;

/*
 * create says whether the store may be created and written: 1 opens for
 * read/write and makes the file if it is absent, 0 opens read-only and
 * answers 0 when there is nothing to read.  The caller frees the value a
 * get or foreach hands back.
 */
extern struct kv *kv_open(char *path, int create);
extern void kv_close(struct kv *kv);
extern int kv_put(struct kv *kv, void *key, int klen, void *val, int vlen);
extern int kv_get(struct kv *kv, void *key, int klen, void **val, int *vlen);
extern void kv_foreach(struct kv *kv,
    void (*fn)(void *key, int klen, void *val, int vlen, void *arg),
    void *arg);

