/*
 * The media changer.
 *
 * hwsim/mediachg.c
 *
 * Out-of-band control of removable media: one line of text on S-100 port
 * 0xd2, answered on the same port.  sys/MEDIACHG.md is the protocol; this
 * is its simulator end.
 *
 * It belongs to the simulator and not to a card.  Port 0xd2 sits in the
 * page the sim keeps for itself - 0xd0 is the kernel's control port and
 * 0xd1 its program name, both in mpz80.c - and what these commands move
 * is the simulator's media mapping, not any controller's registers.  The
 * cards offer their drives here (media.h) and the changer says which of
 * those drives holds what.
 *
 * A bound medium is one of three things:
 *
 *   a path        an image that must already be there
 *   blank:name    a fresh image, created at that name
 *   set:vol       one volume of a named media set
 *
 * and the set is what makes "next" mean anything: the changer remembers
 * where in a set a drive is, not merely which file it opened.
 *
 * '@' is the leadin and the resync point, so a guest that died
 * mid-command is harmless - the next '@' discards the half-sent command
 * and any unread response.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#include "sim.h"
#include "hwsim.h"
#include "util.h"
#include "media.h"

#define MEDIA_PORT      0xd2    /* the changer's port (dev/devlist) */
#define MEDIA_MAX       80      /* longest command, '@' excluded */
#define RESP_MAX        512     /* longest answer: a drive list is not 80 bytes */

extern char *progname;

struct mediaset {
    char *name;
    char **vol;                 /* one image path per volume */
    int nvol;
};

static struct mediaset *msets;  /* the sets, from the -e manifest */
static int nmsets;

/*
 * One drive's state.  `set` is the media set bound there and `vol` where
 * in it the drive is; -1 means no set.  `path` is what the drive was last
 * told to hold and is what a query answers with when no set is involved.
 */
struct unit {
    int set;
    int vol;
    char *path;
};

/* a card's drives, as offered; the cards never see this side of it */
#define MAXMEDIADEV     8
struct mediadev_slot {
    struct mediadev *ops;
    struct unit *u;             /* ops->nunits of them */
};

static struct mediadev_slot dev[MAXMEDIADEV];
static int ndev;

void
register_mediadev(struct mediadev *m)
{
    int i;

    if (ndev == MAXMEDIADEV) {
        fprintf(stderr, "%s: more than %d media devices\n", progname,
            MAXMEDIADEV);
        exit(1);
    }
    dev[ndev].ops = m;
    dev[ndev].u = calloc(m->nunits, sizeof(struct unit));
    for (i = 0; i < m->nunits; i++)
        dev[ndev].u[i].set = dev[ndev].u[i].vol = -1;
    ndev++;
}

/* the staged response: one line, taken a byte at a time */
static char resp[RESP_MAX];
static int resp_len;
static int resp_pos;

/* the command that response answers, for the log line only */
static char resp_cmd[MEDIA_MAX + 1];

static void
stage_response(char *s)
{
    int n = strlen(s);

    if (n >= RESP_MAX)
        n = RESP_MAX - 1;
    memcpy(resp, s, n);
    resp[n++] = '\n';
    resp_len = n;
    resp_pos = 0;

    /*
     * And into the sim's own log, because the fire-and-forget spelling
     * ("echo @djdma:0 empty >/dev/mediachg") has nobody left to read the
     * answer: without this a refused command is silent.  The command is
     * named too, so a log is a transcript rather than a list of replies.
     */
    printf("media: %s -> %s\n", resp_cmd, s);
}

/* next whitespace-delimited token from *p, advanced past it; 0 at end */
static char *
token(char **p)
{
    char *s = *p;
    char *t;

    while (*s == ' ' || *s == '\t')
        s++;
    if (*s == '\0')
        return 0;
    t = s;
    while (*t && *t != ' ' && *t != '\t')
        t++;
    if (*t)
        *t++ = '\0';
    *p = t;
    return s;
}

/* "name:number" split at the colon; -1 if malformed */
static int
split_colon(char *s, char **namep, int *nump)
{
    char *colon = strchr(s, ':');
    char *d;

    if (!colon || colon == s || colon[1] == '\0')
        return -1;
    *colon = '\0';
    *namep = s;
    for (d = colon + 1; *d; d++)
        if (*d < '0' || *d > '9')
            return -1;
    *nump = atoi(colon + 1);
    return 0;
}

static int
find_set(char *name)
{
    int i;

    for (i = 0; i < nmsets; i++)
        if (strcmp(msets[i].name, name) == 0)
            return i;
    return -1;
}

/* is this card in the machine more than once?  then its name is not enough */
static int
card_ambiguous(char *card)
{
    int i, n = 0;

    for (i = 0; i < ndev; i++)
        if (strcmp(dev[i].ops->card, card) == 0 && ++n > 1)
            return 1;
    return 0;
}

/*
 * "djdma:2" to the card that offers it and the unit on it - or, when the
 * machine carries two of the card, "djdma.1:2" to say which.  A bare
 * card name in that machine is refused rather than guessed at.
 */
static struct mediadev_slot *
parse_drive(char *s, int *unitp)
{
    char *name, *dot, *d;
    int logical, inst = -1, i;
    struct mediadev_slot *found = 0;

    if (split_colon(s, &name, &logical) < 0)
        return 0;
    dot = strchr(name, '.');
    if (dot) {
        if (dot == name || dot[1] == '\0')
            return 0;
        *dot = '\0';
        for (d = dot + 1; *d; d++)
            if (*d < '0' || *d > '9')
                return 0;
        inst = atoi(dot + 1);
    }
    for (i = 0; i < ndev; i++) {
        struct mediadev *m = dev[i].ops;

        if (strcmp(m->card, name) != 0)
            continue;
        if (inst >= 0 && m->instance != inst)
            continue;
        if (logical < 0 || logical >= m->nunits)
            return 0;
        if (found)
            return 0;           /* two of them, and no "." to choose */
        *unitp = logical;
        found = &dev[i];
    }
    return found;
}

/* what a drive holds, as an answer */
static void
query_drive(struct mediadev_slot *d, int unit)
{
    char buf[RESP_MAX];

    if (d->u[unit].set >= 0)
        sprintf(buf, "ok %s:%d", msets[d->u[unit].set].name, d->u[unit].vol);
    else if (d->u[unit].path)
        sprintf(buf, "ok %s", d->u[unit].path);
    else
        sprintf(buf, "ok empty");
    stage_response(buf);
}

static void
set_binding(struct mediadev_slot *d, int unit, int set, int vol, char *path)
{
    free(d->u[unit].path);
    d->u[unit].path = path;
    d->u[unit].set = set;
    d->u[unit].vol = vol;
}

/*
 * A card reporting a drive it filled from outside the changer.  The card
 * hands back the same struct it registered, which is what makes this
 * unambiguous when there are two of the card in the machine.
 */
void
media_bound(struct mediadev *m, int unit, char *path)
{
    int i;

    for (i = 0; i < ndev; i++)
        if (dev[i].ops == m) {
            if (unit >= 0 && unit < m->nunits)
                set_binding(&dev[i], unit, -1, -1, strdup(path));
            return;
        }
}

/*
 * Put something in a drive.  `src` is a path, blank:name, set:vol or
 * empty; everything the card is told is a path, and blank is the one case
 * where the path is allowed not to exist yet.
 */
static void
bind_drive(struct mediadev_slot *d, int unit, char *src)
{
    char *name, *path;
    int num = -1, set = -1;

    if (strcmp(src, "empty") == 0) {
        d->ops->eject(unit);
        set_binding(d, unit, -1, -1, 0);
        stage_response("ok");
        return;
    }

    if (strncmp(src, "blank:", 6) == 0) {
        /*
         * Fresh, and not "create if absent": a blank that turns out to
         * hold last week's volume is a trap, and the name is the
         * caller's to choose, so it means what it says.
         */
        path = drive_resolve(src + 6);
        unlink(path);
    } else if (split_colon(src, &name, &num) == 0) {
        set = find_set(name);
        if (set < 0) {
            stage_response("error noset");
            return;
        }
        if (num < 0 || num >= msets[set].nvol) {
            stage_response("error novol");
            return;
        }
        path = drive_resolve(msets[set].vol[num]);
        if (access(path, R_OK) != 0) {
            free(path);
            stage_response("error ioerror");
            return;
        }
    } else {
        path = drive_resolve(src);
        if (access(path, R_OK) != 0) {
            free(path);
            stage_response("error ioerror");
            return;
        }
    }

    if (d->ops->load(unit, path) < 0) {
        free(path);
        stage_response("error ioerror");
        return;
    }
    set_binding(d, unit, set, set >= 0 ? num : -1, path);
    stage_response("ok");
}

/* the next volume of whatever set the drive is in */
static void
next_drive(struct mediadev_slot *d, int unit)
{
    char *path;
    int set, num;

    if (d->u[unit].set < 0) {
        stage_response("error nomedia");
        return;
    }
    set = d->u[unit].set;
    num = d->u[unit].vol + 1;
    if (num >= msets[set].nvol) {
        stage_response("error novol");
        return;
    }
    path = drive_resolve(msets[set].vol[num]);
    if (d->ops->load(unit, path) < 0) {
        free(path);
        stage_response("error ioerror");
        return;
    }
    free(d->u[unit].path);
    d->u[unit].path = path;
    d->u[unit].vol = num;
    stage_response("ok");
}

/*
 * Every drive the machine can address, and what is in it.  The spelling
 * is the command's own ("djdma:2=...") and not a card name glued to a
 * number, so a listing can be read back as commands.
 */
static void
list_drives(void)
{
    char buf[RESP_MAX];
    int i, u, n;

    n = sprintf(buf, "ok");
    for (i = 0; i < ndev; i++) {
        struct mediadev *m = dev[i].ops;

        for (u = 0; u < m->nunits; u++) {
            char nm[64];
            char one[256];

            if (card_ambiguous(m->card))
                snprintf(nm, sizeof nm, "%s.%d:%d", m->card, m->instance, u);
            else
                snprintf(nm, sizeof nm, "%s:%d", m->card, u);
            if (dev[i].u[u].set >= 0)
                snprintf(one, sizeof one, " %s=%s:%d", nm,
                    msets[dev[i].u[u].set].name, dev[i].u[u].vol);
            else if (dev[i].u[u].path)
                snprintf(one, sizeof one, " %s=%.200s", nm,
                    dev[i].u[u].path);
            else
                snprintf(one, sizeof one, " %s=empty", nm);
            if (n + (int)strlen(one) >= RESP_MAX - 1)
                break;
            strcpy(buf + n, one);
            n += strlen(one);
        }
    }
    stage_response(buf);
}

static void
list_sets(void)
{
    char buf[RESP_MAX];
    int i, n = 0;

    n += sprintf(buf + n, "ok");
    for (i = 0; i < nmsets; i++) {
        char one[128];

        if (msets[i].name[0] == '.')
            continue;           /* the .N blanks of a set under construction */
        sprintf(one, " %s:%d", msets[i].name, msets[i].nvol);
        if (n + (int)strlen(one) >= RESP_MAX - 1)
            break;
        strcpy(buf + n, one);
        n += strlen(one);
    }
    if (n == 2)
        strcpy(buf + n, " none");
    stage_response(buf);
}

static void
list_dir(char *arg)
{
    char buf[RESP_MAX];
    char *dir = arg ? arg : drive_getdir();
    DIR *dp;
    struct dirent *e;
    int n;

    if (!dir) {
        stage_response("ok (none)");
        return;
    }
    dp = opendir(dir);
    if (!dp) {
        stage_response("error nodir");
        return;
    }
    n = sprintf(buf, "ok");
    while ((e = readdir(dp)) != 0) {
        char one[300];

        if (e->d_name[0] == '.')
            continue;
        sprintf(one, " %s", e->d_name);
        if (n + (int)strlen(one) >= RESP_MAX - 1)
            break;
        strcpy(buf + n, one);
        n += strlen(one);
    }
    closedir(dp);
    stage_response(buf);
}

static void
media_execute(char *s, int n)
{
    char line[MEDIA_MAX + 1];
    char *p, *verb, *a1, *a2;
    struct mediadev_slot *d;
    int unit;

    if (n > MEDIA_MAX)
        n = MEDIA_MAX;
    memcpy(line, s, n);
    line[n] = '\0';
    strcpy(resp_cmd, line);
    p = line;

    verb = token(&p);
    if (!verb) {
        stage_response("error syntax");
        return;
    }
    a1 = token(&p);

    /*
     * A drive always carries a colon and no verb does, so the two forms
     * never collide: "<drive>" alone is "what is in it", and "<drive>
     * <source>" is "put this in it".
     */
    if (strchr(verb, ':')) {
        d = parse_drive(verb, &unit);
        if (!d) {
            stage_response("error nodrive");
            return;
        }
        if (!a1)
            query_drive(d, unit);
        else
            bind_drive(d, unit, a1);
        return;
    }

    if (strcmp(verb, "next") == 0) {
        if (!a1 || !(d = parse_drive(a1, &unit))) {
            stage_response(a1 ? "error nodrive" : "error syntax");
            return;
        }
        next_drive(d, unit);
        return;
    }

    if (strcmp(verb, "drives") == 0) {
        list_drives();
        return;
    }

    if (strcmp(verb, "sets") == 0) {
        list_sets();
        return;
    }

    if (strcmp(verb, "dir") == 0) {
        if (a1) {
            drive_setdir(a1);
            stage_response("ok");
        } else if (drive_getdir()) {
            char one[RESP_MAX];

            snprintf(one, sizeof one, "ok %s", drive_getdir());
            stage_response(one);
        } else {
            stage_response("ok (none)");
        }
        return;
    }

    if (strcmp(verb, "ls") == 0) {
        list_dir(a1);
        return;
    }

    /*
     * The original spellings, kept because the scripts that walk an
     * install use them.  Each is one of the forms above said the long
     * way: load is "bind", unload and eject are "empty", and query is
     * either the bare drive or "sets".
     */
    if (strcmp(verb, "query") == 0) {
        if (!a1) {
            stage_response("error syntax");
            return;
        }
        if (strchr(a1, ':')) {
            if (!(d = parse_drive(a1, &unit)))
                stage_response("error nodrive");
            else
                query_drive(d, unit);
            return;
        }
        list_sets();
        return;
    }

    if (strcmp(verb, "load") == 0) {
        a2 = token(&p);
        if (!a1 || !a2 || strcmp(a2, "onto") != 0) {
            stage_response("error syntax");
            return;
        }
        a2 = token(&p);
        if (!a2 || !(d = parse_drive(a2, &unit))) {
            stage_response("error nodrive");
            return;
        }
        bind_drive(d, unit, a1);
        return;
    }

    if (strcmp(verb, "unload") == 0 || strcmp(verb, "eject") == 0) {
        if (!a1 || !(d = parse_drive(a1, &unit))) {
            stage_response(a1 ? "error nodrive" : "error syntax");
            return;
        }
        bind_drive(d, unit, "empty");
        return;
    }

    stage_response("error syntax");
}

/* the response side of port 0xd2: the next staged byte, 0 when exhausted */
static byte
media_in(portaddr p)
{
    if (resp_pos < resp_len)
        return resp[resp_pos++];
    return 0;
}

/* the command side: '@' resyncs, '\n' runs, anything else accumulates */
static void
media_out(portaddr p, byte v)
{
    static char mcbuf[MEDIA_MAX];
    static int mcn;
    static int mc_idle = 1;

    if (v == '@') {
        mcn = 0;
        mc_idle = 0;
        resp_len = 0;           /* a stale response dies with the leadin */
        resp_pos = 0;
        return;
    }
    if (mc_idle)
        return;                 /* no leadin: drop it */
    if (v == '\n') {
        media_execute(mcbuf, mcn);
        mcn = 0;
        mc_idle = 1;
        return;
    }
    if (mcn >= MEDIA_MAX) {     /* overlong line: resync on the next '@' */
        stage_response("error syntax");
        mcn = 0;
        mc_idle = 1;
        return;
    }
    mcbuf[mcn++] = v;
}

/* load the -e manifest: one line per set, name then image paths */
static void
media_load_manifest(char *path)
{
    FILE *fp;
    char line[4096];

    if (!path)
        return;
    fp = fopen(path, "r");
    if (!fp) {
        printf("media: could not open %s\n", path);
        return;
    }
    while (fgets(line, sizeof line, fp)) {
        struct mediaset *s;
        char *p, *tok, *vol;
        int nalloc;

        p = strchr(line, '\n');
        if (p)
            *p = '\0';
        p = line;
        while (*p == ' ' || *p == '\t')
            p++;
        if (*p == '\0' || *p == '#')
            continue;

        tok = token(&p);
        if (!tok)
            continue;

        msets = realloc(msets, (nmsets + 1) * sizeof *msets);
        s = &msets[nmsets];
        s->name = strdup(tok);
        s->nvol = 0;
        s->vol = 0;
        nalloc = 0;
        while ((vol = token(&p)) != 0) {
            if (s->nvol == nalloc) {
                nalloc = nalloc ? nalloc * 2 : 8;
                s->vol = realloc(s->vol, nalloc * sizeof *s->vol);
            }
            s->vol[s->nvol++] = strdup(vol);
        }
        nmsets++;
    }
    fclose(fp);
}

static int
mediachg_startup(void)
{
    register_input(MEDIA_PORT, &media_in);
    register_output(MEDIA_PORT, &media_out);
    media_load_manifest(medias_file);
    return 0;
}

struct driver mediachg_driver = {
    "mediachg",
    0,
    0,
    &mediachg_startup,
    0
};

/*
 * this grammar makes the compiler call this function before main()
 * this means we can add drivers by just adding them to the link
 */
__attribute__((constructor))
void
register_mediachg_driver()
{
    register_driver(&mediachg_driver);
}
