/*
 * patch a hitech-c compiled binary for CP/M to run on micronix
 *
 * please note that this parser is a hack - it is extremely
 *		dependent on a well-formed patch file.  
 *		forget an 'end', and you are screwed.
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
 * ; <comment>
 * block <variable name>
 *   match [<address>]
 *     <pattern bytes>	  [ ; comment ]
 *   end
 *   define  <variable name> <variable>[+|-]<offset>
 *   extract <address> <variable name>
 *   ...
 *   patch <address> [<fill length]
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
 * pattern bytes :  hexdigits, ANY, <variable>[(+|-)offset]
 *		CALL JUMP RET are syntax sugar for 0xcd, 0xc3, 0xc9 respectively
 * 		CALL foo:
 *			if foo is known, will try to match, and if unknown, create
 *			bias is not allowed for unknown symbols
 * note that integer constants are heavily hexadecimal.  the only place
 * where decimal is assumed is in <variable>+<offset>, and that can be 
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
char *patchdir;
char *pname;
int sflag;
int verbose;
int pf;
#define	V_PARSE		1
#define	V_VAR		2
#define	V_DATA		4
#define	V_MATCH		8
#define	V_LOW		16
#define	V_CHANGES	32

typedef unsigned short word;

unsigned char membuf[65536]; /* binary image */
int objsize;

char patchbuf[65536]; /* patch text */
char *cursor; /* pointer into patch text */

word matchaddr;
int dirty;
char *blockname;

/*
 * a block can have several match blocks and several patch/replace blocks.
 * a block hits only if ALL match blocks hit.
 */
int blockstart;
int blockend;
int blockhit;
int blockdirty;

/*
 * pattern array is parsed into this.
 * a pattern entry can matches exactly 1 byte.
 * if the 
 */
struct pat {
	char *name;
	unsigned char value;
	unsigned char flags;
#define	ANY		1
#define	EXTRACT	2
#define	BYTE	4
#define	WORD	8
};

#define	max(a,b) ((a) > (b) ? (a) : (b))

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

char getbuf[40];

struct var {
    char *name;
    word value;
    struct var *next;
} *vars, *varq;

char namebuf[20];

/*
 * given a variable name, return the var struct or null
 */
struct var *
getvar(char *n)
{
    struct var *v = 0;
    word vv = 0xffff;
	int i;
	
	for (i = 0; i < sizeof(namebuf) - 1; i++) {
		if (*n == '\0' || *n == '+' || *n == '-' || *n == '.')
			break;	
		namebuf[i] = *n++;
	}
	namebuf[i] = '\0';

    for (v = vars; v; v = v->next) {
        if (strcmp(namebuf, v->name) == 0) {
            return v;
        }
    }
}

void
putvar(char *n, word new)
{
    struct var *v;

    if (verbose & V_VAR) printf("putvar: %s %04x\n", n, new);

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

void
queuevar(char *n, word new)
{
    struct var *v;

    if (verbose & V_VAR) printf("queuevar: %s %04x\n", n, new);

    for (v = varq; v; v = v->next) {
        if (strcmp(n, v->name) == 0) {
            if (v->value != new) {
                printf("queuevar: name %s already present value %x new %x\n",
                        n, v->value, new);
                v->value = new;
            }
            return;
        }
    }
    v = malloc(sizeof (*v));
    v->name = strdup(n);
    v->value = new;
    v->next = varq;
    varq = v;
}

void
do_queue(int hit)
{
	struct var *v;

	while (v = varq) {
		varq = v->next;
		if (hit) {
			putvar(v->name, v->value);
		}
		free(v->name);
		free(v);
	}
}


word
wordat(word addr)
{
    return membuf[addr] + (membuf[addr + 1] << 8);
}

/*
 * copy the next token to getbuf, returning the pointer.
 * leave the cursor after the next hunk of horizontal white space
 */
char *
get()
{
    char *s = getbuf;

    while (*cursor && (*cursor == ' ' || *cursor == '\t'))
        cursor++;

    *s = '\0';

    while (*cursor && *cursor != '\n' && 
		*cursor != ';' && *cursor != ' ' && *cursor != '\t') {
        *s++ = *cursor++;
        *s = '\0';
    }

    while (*cursor && (*cursor == ' ' || *cursor == '\t'))
        cursor++;

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
 * the upper word is the number of hex digits, which can be zero.
 */
#define	HEXDIGITS(k)	((k) >> 16)

unsigned int
hexin(char *s)
{
    word r = 0;
    word d;
    int i = 0;

    while (*s) {
		switch(*s) {
		case '0': case '1': case '2': case '3': case '4': 
		case '5': case '6': case '7': case '8': case '9':
			d = *s - '0';
			break;
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
			d = *s - 'a' + 0xa;
			break;
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
			d = *s - 'A' + 0xa;
			break;
		default:
			return 0;
		}
        r <<= 4;
        r += d;
        i++;
		s++;
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
	struct var *varp;

    if (verbose & V_VAR) printf("getaddr %s", in);

    if ((s = index(in, '+')) != 0) {
        *s++ = '\0';
        sign = 1;
    } else if ((s = index(in, '-')) != 0) {
        *s++ = '\0';
        sign = -1;
    }

    v = hexin(in);
    if (HEXDIGITS(v) != 4) {
        varp = getvar(in);
		if (varp) {
			v = varp->value;
		} else {
			if (verbose & V_VAR) {
				printf("variable %s not set\n", in);
			}
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
    if (verbose & V_VAR) printf(" = %04x\n", v + o);
    return v + o;
}

/*
 * if a pattern exactly matches at base, then return a hit
 * variables
 */
int
patmatch(struct pat *pat, int pl, word base)
{
    int i;

    if (!pl) return 0;

    for (i = 0; i < pl; i++) {
        if (pat[i].flags & ANY) continue;
        if (pat[i].value == membuf[base + i]) continue;
        if ((verbose & V_MATCH) && (i > 4)) {
            printf("miscompare at index %d: %04x wanted %02x got %02x\n",
                    i, base + i, pat[i].value, membuf[base + i]);
        }
        return 0;
    }
    matchaddr = base;

	if (verbose & V_MATCH) {
		printf("hit at %04x", base);
    }

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

    if (!blockhit) {
        skiptoeol();
        return;
    }
    if (verbose) printf("extract\n");

    a = get();
    aa = strdup(a);

    ad = getaddr(a);

    v = wordat(ad);

    a = get();
    printf("\t%10s %04x (%s:%04x)\n", a, v, aa, ad);
    putvar(a, v);
    skiptoeol();
}

/*
 * define name expression
 */
void
defvar()
{
    char *a;
    word ad;
    word v;
    char *aa;
	char *varname;

    if (!blockhit) {
        skiptoeol();
        return;
    }
	varname = strdup(get());

    ad = getaddr(get());

    if (verbose) printf("define %10s %04x\n", varname, ad);
    putvar(varname, ad);
    skiptoeol();
}

/*
 * parse a pattern, returning the number of entries
 */
int
parsepat(struct pat *p)
{
	int pl;
	char *t;
	unsigned int i;
	int val;
	struct var *sym;
	int f;
	char *n;
	
	pl = 0;

    while (*cursor) {
        skipwhite();
        if (wordmatch("end")) {
            skiptoeol();
            break;
        }

    	t = get();
		val = hexin(t);
		n = 0;
		f = 0;
		if (verbose & V_LOW) {
			printf("%s\n", t);
		}

		if (!strcmp(t, "ANY")) {
			val = 0;
			f = ANY;
		} else if (!strcmp(t, "RET")) {
			val = 0xc9;
		} else if (!strcmp(t, "CALL")) {
			val = 0xcd;
		} else if (!strcmp(t, "JUMP")) {
			val = 0xc3;
		} else if (HEXDIGITS(val) > 2) {
			f = WORD;
			val = val & 0xffff;
		} else if (HEXDIGITS(val) > 0) {
			val = val & 0xff;
		} else {
			namebuf[0] = '\0';
			sym = getvar(t);
			i = strlen(namebuf);
			if (sym) {
				val = sym->value;
				int sign = 0;
				if (t[i] == '+') {
					sign = 1;
				} else if (t[i] == '-') {
					sign = -1;
				}
				val = val + (sign * numin(&t[i+1]));
				f = WORD;
			} else if (i) {
				if (t[i] != '\0') {
					printf("no bias on unknown symbol");
				}
				n = strdup(namebuf);
				f = ANY | WORD | EXTRACT;
			} else {
				printf("no name\n");
			}
		}
		p[pl].value = val;
		p[pl].name = n;
		p[pl].flags = f | BYTE;
		pl++;

		if ((f & WORD) || (f & EXTRACT)) {
			p[pl].value = (val >> 8) & 0xff;
			p[pl].name = n;
			p[pl].flags = f;
			pl++;
		}
    }
	return (pl);
}

void
replace()
{
    struct pat *repl = malloc(sizeof (*repl) * MAX_PATTERN);
    int rl;
    word ad;
    char *a;
    int i;

    a = get();
    if (blockhit) {
        ad = getaddr(strdup(a));
    } else {
        ad = 0;
    }

    if (verbose) printf("replace %s(%04x)\n", a, ad);

	rl = parsepat(repl);

    if (blockhit) {
        printf("\t%10s %04x %d\n", "replace", ad, rl);
    } else {
        return;
    }
	if (blockend < ad + rl) {
		blockend = ad + rl;
	}

    for (i = 0; i < rl; i++) {
        membuf[ad + i] = repl[i].value;
		blockdirty = dirty = 1;
    }
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
	int fill = 0;

    if (verbose) printf("patch\n");

    asmtext = optr = malloc(MAX_PATCH);

    a = get();
    if (blockhit) {
        ad = getaddr(strdup(a));
    } else {
        ad = 0;
    }
	if (*cursor != '\n') {
		fill = atoi(get());
	}
	
    /*
     * build the assembly prologue:
     * send all our defined variables to the assembly as equates
     */
    if (blockhit) {
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

		if (wordmatch("missing")) {
            fprintf(stderr, "patch for block %s missing\n", blockname);
			exit(2);
		}

        /* process included source: include[\t ]+ ["']filename["'] \n */
        if (wordmatch("include")) {
            char *incfile = malloc(100);
            char *ifn = incfile;
            int fd;

			if (patchdir) {
				ifn += sprintf(incfile, "%s/", patchdir);
			}

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
            if (*cursor == ' ' || *cursor == '\t') *cursor = ' ';
            *optr++ = *cursor++;
            if (optr[-1] == '\t') {
                while ((*cursor == '\t') || (*cursor == ' ')) cursor++;
            }
        }
        *optr++ = '\n';
        *optr = '\0';

        if (strncmp(s, "end", 3) == 0 && rindex(" \t\n\0", s[3]))
            break;
    }

    if (verbose & V_DATA) printf("asm text:\n%s\n", asmtext);
    if (blockhit) {
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
        if (verbose) {
            unlink("asmtext.cim");
            unlink("asmtext.asm");
        }
    }

    if (blockhit) {
		if (blockend < ad + fill) {
			blockend = ad + fill;
		}
        printf("patch %04x %d fill %d\n", ad, rl, fill);
        blockdirty = dirty = 1;

		for (i = 0 ; i < fill; i++) {
			membuf[ad + i] = 0;
		}
		for (i = 0; i < rl; i++) {
			membuf[ad + i] = repl[i];
		}
	}
    if (verbose) printf("patch end\n");
}

/*
 * this function is really the engine of the patcher.
 * it does all the searching in the object file, and is where we spend the 
 * vast majority of our time. first, we snarf the entire match text into a 
 * buffer, where we do value substititions as needed.
 */

int
match()
{
    char *anchor;
    struct pat *pat = malloc(sizeof (*pat) * MAX_PATTERN);
    int pl;
    int i;
    word base;
    char *msg = "unanchored";
	int hit = 0;

    anchor = get();
    if (anchor[0]) {
        base = getaddr(anchor);
        msg = anchor;
    } else {
        anchor = 0;
    }

    if (verbose) printf("match %s\n", msg);

	pl = parsepat(pat);

    if (verbose & V_DATA) {
        printf("checking match patlen %d\n", pl);
        for (i = 0; i < pl; i++) {
			if (pat[i].flags & ANY) {
				printf("?? ");
			} else {
				printf("%02x ", pat[i].value);
			}
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

	if (hit) {
		if (blockstart == 0) {
			blockstart = matchaddr;
		}
		blockend = matchaddr + pl;

		for (i = 0; i < pl; i++) {
			int ext;
			if (pat[i].flags & EXTRACT) {
				if (verbose & V_VAR) {
					printf("extract [%d]: %s %d %d\n", 
						i, pat[i].name, pat[i].value, pat[i].flags);
				}
				if (pat[i].flags & BYTE) {
					ext = membuf[base + i];
				} else {
					ext += membuf[base + i] << 8;
					queuevar(pat[i].name, ext);
				}
			}
		}
	}
	return hit;
}

void
block()
{
    blockname = get();
	struct var *vp;
	int matchcount;

    if (!blockname[0]) {
        fprintf(stderr, "block must have a name\n");
        exit(6);
    }
    if (verbose) printf("\nblock %s\n", blockname);
    blockname = strdup(blockname);

	blockstart = 0;
	blockend = 0;
    blockhit = 0;
	blockdirty = 0;

	varq = 0;
	matchcount = 0;

    skiptoeol();
    while (*cursor) {
        skipwhite();
        if (wordmatch("match")) {
			matchcount++;
            if (match()) {
				if (matchcount == 1) {
					vp = getvar(blockname);
					if (!vp) {
						putvar(blockname, matchaddr);
					} else if (vp->value != matchaddr) {
						printf("block %s matchaddr %04x already %04x\n",
								blockname, matchaddr, vp->value);
					}
				}
				blockhit = 1;
            } else {
                if (verbose & V_MATCH)
                    printf("block %s miss\n", blockname);
				blockhit = 0;
            }
        } else {
			if (verbose && blockhit) {
                    printf("block %s hit at 0x%04x\n", blockname, blockstart);
			}
			do_queue(blockhit);
		}
		if (wordmatch("define")) {
			defvar();
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
	if (blockhit && blockdirty && (verbose & V_CHANGES)) {
		int i;

		printf("block data from %x to %x (%d)\n", 
			blockstart, blockend, blockend - blockstart);

        for (i = 0; i < blockend - blockstart; i++) {
			printf("%02x ", membuf[blockstart + i]);
            if ((i % 8) == 7) printf("\n");
        }
        printf("\n");
	}
    if (verbose) printf("block end\n");
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
				if (s[1]) {
					verbose = strtol(&s[1], 0, 0);
					s = "v";
				} else {
					verbose++;
				}
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

    if (verbose)
        printf("using patch: %s verbose: %x\n", patchfile, verbose);

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

	if (rindex(patchfile, '/')) {
		patchdir = strdup(patchfile);
		*(char *)rindex(patchdir, '/') = '\0';
	}
    while (argc--) {
        process(*argv++);
    }
}
