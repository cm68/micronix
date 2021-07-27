/*
 * patch a hitech-c compiled binary for CP/M to run on micronix
 *
 * runs on linux, and snarfs the entire object file into memory
 * porting this to micronix is not worth it.
 *
 * it turns out that the hitech compiler suite uses a unix-like libc
 * for i/o.  so, we grunge through the binaries looking for bdos calls,
 * and then go up the call tree until we find the unix api functions.
 * then, we replace them with micronix calls.
 * finally, we scribble an appropriate micronix exec struct at the
 * start of the file and presto, we have a micronix hitech-c.
 *
 * this is driven from a pattern-matching file with conditional blocks and
 * replacements.
 *
 * a block is either true or false. it must contain a match block that has 
 * either a fixed address, a relative address from a variable, or no anchor 
 * at all.  if it has no anchor, the entire file is searched for the match.
 *
 * if a match is found, the variable in the block name is assigned to the 
 * base address of the match, and the extract, patch, and  replace rules are 
 * applied. patch uses an assembler, but replace is just hex data
 *
 * if no match is found, the block is ignored and the next block is processed.
 *
 * grammar:
 *
 * block <variable name>
 *   match [<address>]
 *     <pattern bytes>	  [ ; comment ]
 *   end
 *   extract <address> <variable name>
 *   ...
 *   patch <address>
 *     include <filename>
 *     <assembly code>    [ ; comment ] 
 *	 end
 *   replace <address>
 *     <replacement bytes>
 *   end
 *   ... more extracts, patches and replacements
 * end
 *
 * hexdigit : [0-9a-f]
 * pattern bytes :  hexdigit hexdigit , ANY, <variable>.L, <variable>.H
 *               CALL JUMP <variable> are specially handled. expanding into
 *				 0xcd, 0xc3, and variable.L, variable.H respectively
 *
 * address :  <variable> , <variable> + <offset>, hexdigit...
 * offset : 0xhexdigit..., integer
 *
 * note that integer constants are heavily hexadecimal.  the only place
 * where decimal is assumed is in <address>+<offset>, and that can be 
 * overridden with an explicit 0x.
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned short UINT;
#include "../micronix/include/obj.h"

#define MAX_PATTERN 4096
#define MAX_PATCH 4096

char *patchfile = "hitech.pat";
char *pname;
int sflag;
int verbose;
int pf;

typedef unsigned short word;

unsigned char membuf[65536]; /* binary image */
int objsize;

char patchbuf[65536]; /* patch text */
char *cursor; /* pointer into patch text */

int hit; /* state of most recent match */
word matchaddr;
int dirty;
char *blockname;

usage(char c)
{
    fprintf(stderr, "usage:\n%s [<options>] objectfile ...\n", pname);
    fprintf(stderr, "\t-v\t\tincrease verbosity\n");
    fprintf(stderr, "\t-s\t\tproduce sym file\n");
    fprintf(stderr, "\t-p <patchfile>\n");
    if (c) {
        fprintf(stderr, "\n\tinvalid flag %c\n", c);
    }
    exit(1);
}

char getbuf[20];

struct var {
    char *name;
    word value;
    struct var *next;
} *vars;

word
getvar(char *n)
{
    struct var *v;
    word vv = 0xffff;

    for (v = vars; v; v = v->next) {
        if (strcmp(n, v->name) == 0) {
            vv = v->value;
            break;
        }
    }
    //	if (verbose > 1) printf("getvar %s = %04x\n", n, vv); 
    return vv;
}

void
putvar(char *n, word new)
{
    struct var *v;

    if (verbose > 1) printf("putvar: %s %04x\n", n, new);

    for (v = vars; v; v = v->next) {
        if (strcmp(n, v->name) == 0) {
            if (v->value != new) {
                printf("putvar: name %s already present value %x new %x\n",
                        n, v->value, new);
                v->value = new;
            }
            return;
        }
    }
    v = malloc(sizeof (*v));
    v->name = strdup(n);
    v->value = new;
    v->next = vars;
    vars = v;
}

word
wordat(word addr)
{
    return membuf[addr] + (membuf[addr + 1] << 8);
}

char *
get()
{
    char *s = getbuf;

    while (*cursor && (*cursor == ' ' || *cursor == '\t'))
        cursor++;
    *s = '\0';

    while (*cursor && *cursor != '\n' && *cursor != ';' && *cursor != ' ' && *cursor != '\t') {
        *s++ = *cursor++;
        *s = '\0';
    }
    return getbuf;
}

skiptoeol()
{
    while (*cursor && *cursor != '\n') {
        cursor++;
    }
}

symchar(char c)
{
    if (c == '_') return 1;
    if (c >= '0' && c <= '9') return 1;
    if (c >= 'A' && c <= 'Z') return 1;
    if (c >= 'a' && c <= 'z') return 1;
    return 0;
}

isymchar(char c)
{
    if (c >= '0' && c <= '9') return 0;
    return symchar(c);
}

/*
 * skip white space (also newlines and comments)
 */
skipwhite()
{
    for (; *cursor; cursor++) {
        if (*cursor == '\n') continue;
        if (*cursor == ' ') continue;
        if (*cursor == '\t') continue;
        if (*cursor == ';') {
            skiptoeol();
            continue;
        }
        break;
    }
}

wordmatch(char *m)
{
    int i = strlen(m);
    if (strncmp(cursor, m, i) == 0) {
        cursor += i;
        return 1;
    }
    return 0;
}

word
nibble(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    c |= 0x20;
    if (c >= 'a' && c <= 'f') return (c - 'a') + 0xa;
    return 0xffff;
}

word
decin(char *s)
{
    word r = 0;
    while (*s >= '0' && *s <= '9') {
        r *= 10;
        r += *s++ -'0';
    }
    return r;
}

/*
 * return a hex int
 * the upper word is the number of hex digits
 * terminate at a non-hex.
 */
unsigned int
hexin(char *s)
{
    word r = 0;
    word d;
    int i = 0;

    while (s[i]) {
        d = nibble(s[i]);
        if (d == 0xffff)
            break;
        r <<= 4;
        r += d;
        i++;
    }

    return r | (i << 16);
}

word
numin(char *s)
{
    if (s[0] == '0' && s[1] == 'x')
        return hexin(s + 2) & 0xffff;
    return decin(s);
}

/*
 * given a string, return the address.
 * grammar: 
 * address :  <variable> , <variable> +|- <offset>, hexdigit...
 * 
 * LEXICAL NOTE: 
 *		spaces are not tolerated anywhere in addresses.  there is no space 
 *		allowed before or after the '+'.  this is NOT an expression parser.
 */
word
getaddr(char *in)
{
    char *s;
    unsigned int v;
    word o = 0;
    char sign = 1;

    if (verbose > 1) printf("getaddr %s", in);

    if ((s = index(in, '+')) != 0) {
        *s++ = '\0';
        sign = 1;
    } else if ((s = index(in, '-')) != 0) {
        *s++ = '\0';
        sign = -1;
    }

    v = hexin(in);
    if ((v >> 16) != 4) {
        v = getvar(in);
        if ((verbose > 1) && v == 0xffff) {
            printf("variable %s not set\n", in);
        }
    } else {
        v &= 0xffff;
    }

    if (s) {
        o = numin(s);
        if (o == 0xffffffff) {
            printf("malformed offset %s\n", s);
            o = 0;
        }
        o *= sign;
    }
    if (verbose > 1) printf(" = %04x\n", v + o);
    return v + o;
}

/*
 * this processes 00 - ff, ANY, and <variable>.L, variable.H
 * integer constants here are ALWAYS hex.
 *
 * there's a little syntactic sugar here, doing a substitute for
 * the words CALL and JUMP, and even a little layer violation when
 * a variable name or 4 byte hex number is found.  in this case, we 
 * return 2 packed match bytes which the caller needs to unpack.
 */
#define BYTE 0
#define WORD 1
#define ANY  2
#define UNK  3
unsigned int val;

int
getval()
{
    char *t = get();
    unsigned int r;
    int i;

    if (strcmp(t, "ANY") == 0) {
        return ANY;
    }
    if (strcmp(t, "CALL") == 0) {
        val = 0xcd;
        return BYTE;
    }

    if (strcmp(t, "JUMP") == 0) {
        val = 0xc3;
        return BYTE;
    }

    i = strlen(t);
    r = hexin(t);

    if ((r >> 16) == 2) {
        val = r & 0xff;
        return BYTE;
    }

    if ((r >> 16) == 4) {
        val = r & 0xffff;
        return WORD;
    }

    if (t[i - 2] == '.' && (t[i - 1] == 'L' || t[i - 1] == 'H')) {
        t[i - 2] = '\0';
        r = getvar(t);
        if (r == 0xffff) {
            if (verbose) printf("variable %s not set\n", t);
            return UNK;
        }
        if (t[i - 1] == 'H') {
            val = r >> 8;
        } else {
            val = r;
        }
        r &= 0xff;
        return BYTE;
    }

    r = getaddr(t);
    if (r == 0xffff) {
        if (verbose) printf("variable %s not set\n", t);
        return UNK;
    }
    val = r;
    return WORD;
}

int
patmatch(word *pat, int pl, word base)
{
    int i;

    if (!pl) return 0;

    for (i = 0; i < pl; i++) {
        if (pat[i] == 0xffff) continue;
        if (pat[i] == membuf[base + i]) continue;
        if ((verbose > 1) && (i > 4)) {
            printf("miscompare at index %d: %04x wanted %04x got %02x\n",
                    i, base + i, pat[i], membuf[base + i]);
        }
        return 0;
    }
    matchaddr = base;
    return 1;
}

/*
 * extract addr name
 */
void
extract()
{
    char *a;
    word ad;
    word v;
    char *aa;

    if (!hit) {
        skiptoeol();
        return;
    }
    if (verbose > 1) printf("extract\n");

    a = get();
    aa = strdup(a);

    ad = getaddr(a);

    v = wordat(ad);

    a = get();
    printf("\t%10s %04x (%s:%04x)\n", a, v, aa, ad);
    putvar(a, v);
    skiptoeol();
}

void
replace()
{
    unsigned char *repl = malloc(MAX_PATTERN);
    int rl = 0;
    word ad;
    char *a;
    int i;

    a = get();
    if (hit) {
        ad = getaddr(strdup(a));
    } else {
        ad = 0;
    }

    if (verbose > 1) printf("replace %s(%04x)\n", a, ad);

    while (*cursor) {
        skipwhite();
        if (wordmatch("end")) {
            skiptoeol();
            break;
        }
        switch (getval()) {
        case WORD:
            repl[rl++] = val & 0xff;
            val >>= 8;
        case BYTE:
            repl[rl++] = val & 0xff;
            break;
        case ANY:
            printf("ANY in replacement\n");
            hit = 0;
            break;
        case UNK:
            if (hit) {
                printf("unknown value in replacement: hit cancelled");
                hit = 0;
            }
            break;
        }
    }

    if (hit) {
        printf("\t%10s %04x %d\n", "replace", ad, rl);
    } else {
        return;
    }

    if (verbose > 1) {
        printf("replacement bytes %d\n", rl);
    }
    for (i = 0; i < rl; i++) {
        if (verbose > 1) {
            printf("%02x ", repl[i]);
            if ((i % 8) == 7) printf("\n");
        }
        membuf[ad + i] = repl[i];
        dirty = 1;
    }
    if (verbose > 1) printf("\n");
}

/*
 * this is a bit bizarre, but we invoke an assembler to generate the
 * bytes to put into our binary
 */
void
patch()
{
    unsigned char *repl = malloc(MAX_PATTERN);
    int rl = 0;
    word ad;
    char *a;
    int i;
    char *asmtext;
    char *optr;
    struct var *v;

    asmtext = optr = malloc(MAX_PATCH);

    a = get();
    if (hit) {
        ad = getaddr(strdup(a));
    } else {
        ad = 0;
    }

    /*
     * build the assembly prologue:
     * send all our defined variables to the assembly as equates
     */
    if (hit) {
        for (v = vars; v; v = v->next) {
            optr += sprintf(optr, "%s\tequ\t0x%04x\n", v->name, v->value);
        }
        optr += sprintf(optr, "\n\t.org\t0x%04x\n", ad);
    }

    /*
     * copy the entire text of the patch into the asm buffer
     * including the end line
     * all lines get indented, except label lines
     */
    while (*cursor) {
        char *s;
        char c;

        while (*cursor == '\n') cursor++;

        /*
         * process a line
         * eat leading white space
         */
        while (*cursor && (*cursor == ' ' || *cursor == '\t')) cursor++;

        /* process included source: include[\t ]+ ["']filename["'] \n */
        if (wordmatch("include")) {
            char *incfile = malloc(100);
            char *ifn = incfile;
            int fd;

            while (*cursor && (*cursor == ' ' || *cursor == '\t')) cursor++;
            i = *cursor;
            switch (i) {
            case '\'':
            case '\"':
                cursor++;
                break;
            default:
                i = 0;
                break;
            }
            while (*cursor) {
                if (i) {
                    if (*cursor == i)
                        break;
                } else {
                    if (*cursor == ' ' || *cursor == '\t' || *cursor == '\n')
                        break;
                }
                *ifn++ = *cursor++;
                *ifn = '\0';
            }
            while (*cursor && (*cursor != '\n')) cursor++;
            printf("got filename %s\n", incfile);
            fd = open(incfile, 0);
            if (fd < 0) {
                printf("could not open %s\n", incfile);
            } else {
                optr += sprintf(optr, "; include %s\n", incfile);
                while (1) {
                    i = read(fd, incfile, 1);
                    if (!i) break;
                    *optr++ = *incfile;
                }
                close(fd);
            }
        }

        /*
         * do we have a label? _foo0:
         * this code looks like it simply outputs a tab before a label 
         */
        s = cursor;
        if (isymchar(*s)) {
            while (*s && symchar(*s)) s++;
        }
        if (*s != ':') {
            *optr++ = '\t';
        }

        s = cursor;

        /* copy the rest of the line, compressing out redundant space */
        while (*cursor && *cursor != '\n') {
            if (*cursor == ' ' || *cursor == '\t') *cursor = '\t';
            *optr++ = *cursor++;
            if (optr[-1] == '\t') {
                while ((*cursor == '\t') || (*cursor == ' ')) cursor++;
            }
        }
        *optr++ = '\n';
        *optr = '\0';

        if (strncmp(s, "end\n", 4) == 0)
            break;
    }

    if (verbose > 1) printf("asm text: %s\n", asmtext);
    if (hit) {
        i = creat("asmtext.asm", 0777);
        write(i, asmtext, strlen(asmtext));
        close(i);
        i = system("../tools/zmac -o asmtext.cim asmtext.asm");
        if (i != 0) {
            fprintf(stderr, "patch assembly for block %s failed\n", blockname);
            exit(9);
        }
        i = open("asmtext.cim", 0);
        rl = read(i, repl, MAX_PATTERN);
        close(i);
        if (verbose > 1) {
            unlink("asmtext.cim");
            unlink("asmtext.asm");
        }
    }

    if (hit)
        printf("\t%10s %04x %d\n", "patch", ad, rl);

    if (verbose > 1) {
        printf("replacement bytes %d\n", rl);
    }
    for (i = 0; i < rl; i++) {
        if (verbose > 1) {
            printf("%02x ", repl[i]);
            if ((i % 8) == 7) printf("\n");
        }
        if (hit) {
            membuf[ad + i] = repl[i];
            dirty = 1;
        }
    }
    if (verbose > 1) printf("\n");

    if (verbose > 1) printf("patch end\n");
}

/*
 * this function is really the engine of the patcher.
 * it does all the searching in the object file, and is where we spend the 
 * vast majority of our time. first, we snarf the entire match text into a 
 * buffer, where we do value substititions as needed.
 */
void
match()
{
    char *anchor;
    word *pat = malloc(MAX_PATTERN * 2);
    int pl = 0;
    int i;
    word base;
    char *msg = "unanchored";

    anchor = get();
    hit = 1;
    if (anchor[0]) {
        base = getaddr(anchor);
        msg = anchor;
        if (base == 0xffff) {
            msg = "invalid";
        }
    } else {
        anchor = 0;
    }

    if (verbose) printf("match %s\n", msg);

    /* read in the match block */
    while (*cursor) {
        skipwhite();
        if (wordmatch("end")) {
            skiptoeol();
            break;
        }
        switch (getval()) {
        case WORD:
            pat[pl++] = val & 0xff;
            val >>= 8;
        case BYTE:
            pat[pl++] = val & 0xff;
            break;
        case ANY:
            pat[pl++] = 0xffff;
            break;
        case UNK:
            hit = 0;
        }
    }

    if (hit == 0 || base == 0xffff) {
        if (verbose) printf("miss forced due to unknown values\n");
        return;
    }

    hit = 0;

    if (verbose > 1) {
        printf("checking match patlen %d\n", pl);
        for (i = 0; i < pl; i++) {
            printf("%04x ", pat[i]);
            if ((i % 8) == 7) printf("\n");
        }
        printf("\n");
    }

    if (anchor) {
        if (base != 0xffff) {
            if (patmatch(pat, pl, base)) {
                hit = 1;
            }
        }
    } else {
        for (base = 0x100; base < objsize + 0x100; base++) {
            if (patmatch(pat, pl, base)) {
                if (hit) {
                    printf("multiple hit\n");
                }
                hit = 1;
            }
        }
    }
    if (hit)
        printf("match\t%10s %04x\n", blockname, matchaddr);
    if (verbose > 1) {
        if (hit) {
            printf("hit at %04x\n", base);
        } else {
            printf("miss\n");
        }
        printf("match end\n");
    }
}

void
block()
{
    word addr;
    blockname = get();

    if (!blockname[0]) {
        fprintf(stderr, "block must have a name\n");
        exit(6);
    }
    if (verbose) printf("\nblock %s\n", blockname);
    blockname = strdup(blockname);

    skiptoeol();
    while (*cursor) {
        skipwhite();
        if (wordmatch("match")) {
            match();
            if (hit) {
                addr = getvar(blockname);
                if (getvar(blockname) == 0xffff) {
                    putvar(blockname, matchaddr);
                } else if (addr != matchaddr) {
                    printf("block %s matchaddr %04x already %04x\n",
                            blockname, matchaddr, addr);
                }
            } else {
                if (verbose)
                    printf("block %s miss\n", blockname);
            }
        }
        if (wordmatch("extract")) {
            extract();
        }
        if (wordmatch("patch")) {
            patch();
        }
        if (wordmatch("replace")) {
            replace();
        }
        if (wordmatch("end")) {
            skiptoeol();
            break;
        }
    }
    if (verbose > 1) printf("block end\n");
}

void
process(char *fn)
{
    int fd;
    struct obj hdr;
    struct var *v;
    char *s;
    char *nfn;

    dirty = 0;

    if (verbose)
        printf("process %s\n", fn);
    fd = open(fn, 2);
    if (fd < 0) {
        perror(fn);
        exit(3);
    }
    objsize = read(fd, &membuf[0x100], sizeof (membuf) - 0x100);
    if (objsize < 0) {
        perror(fn);
        exit(4);
    }
    if (verbose) printf("read %d bytes\n", objsize);

    for (cursor = patchbuf; *cursor; cursor++) {
        skipwhite();
        if (wordmatch("block")) {
            block();
        }
    }
    close(fd);

    if (dirty) {
        nfn = strdup(fn);
        s = index(nfn, '.');
        if (!s || (strlen(s) != 4)) {
            fprintf(stderr, "filename must have .COM extension\n");
            exit(8);
        }
        *s = '\0';
        fd = creat(nfn, 0777);
        if (fd < 0) {
            perror(nfn);
            exit(9);
        }

        printf("file dirty: writing to %s\n", nfn);
        hdr.ident = OBJECT;
        hdr.conf = NORELOC;
        hdr.table = 0;
        hdr.text = objsize;
        hdr.data = 0;
        hdr.bss = 0;
        hdr.heap = 0;
        hdr.textoff = 0x100;
        hdr.dataoff = 0;
        write(fd, &hdr, sizeof (hdr));
        write(fd, &membuf[0x100], objsize);
        close(fd);
    }

    if (sflag) {
        s = index(fn, '.');
        if (!s || (strlen(s) != 4)) {
            fprintf(stderr, "filename must have .COM extension\n");
            exit(8);
        }
        strcpy(s + 1, "sym");
        fd = creat(fn, 0777);
        if (fd < 0) {
            perror(fn);
            exit(9);
        }
        for (v = vars; v; v = v->next) {
            dprintf(fd, "%s\tcode\t0x%04x\n", v->name, v->value);
        }
        close(fd);
    }
}

int
main(int argc, char **argv)
{
    char *s;
    int i;

    pname = *argv++;
    argc--;

    while (**argv == '-') {

        for (s = ++*argv; *s; s++) switch (*s) {
            case 'v':
                verbose++;
                break;
            case 's':
                sflag++;
                break;
            case 'p':
                patchfile = *++argv;
                argc--;
                break;
            case 'h':
                usage(0);
                break;
            default:
                usage(*s);
                break;
            }
        argv++;
        argc--;
    }

    if (verbose > 1)
        printf("using patch: %s\n", patchfile);

    pf = open(patchfile, 0);
    if (pf < 0) {
        perror(patchfile);
        exit(2);
    }
    i = read(pf, patchbuf, sizeof (patchbuf));
    if (i < 0) {
        perror(patchfile);
        exit(5);
    }
    patchbuf[i] = '\0';

    while (argc--) {
        process(*argv++);
    }
}
