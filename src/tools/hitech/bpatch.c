/*
 * patch a hitech-c compiled binary for CP/M to run on micronix
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
 * a block is either true or false. it must contain a match block that has either
 * a fixed address, a relative address from a variable, or no anchor at all.  if
 * it has no anchor, the entire file is searched for the match.
 *
 * if a match is found, the variable in the block name is assigned to the base address
 * of the match, and the extract and replace rules are applied.
 *
 * if no match is found, the block is ignored and the next block is processed.
 *
 * grammar:
 *
 * block <variable name>
 *   match [<address>]
 *     <pattern bytes>
 *   end
 *   extract <address> <variable name>
 *   ...
 *   replace <address>
 *     <pattern bytes>
 *   end
 *   ... more replacements
 * end
 *
 * hexdigit : [0-9a-f]
 * pattern bytes :  hexdigit hexdigit , ANY, <variable>.L, <variable>.H
 *               CALL JUMP <variable> are specially handled. expanding into
 *				 0xcd, 0xc3, and variable.L, variable.H respectively
 *               this is not an assembler. yet.  making it into one is probably a
 *				 silly idea.
 *
 * address :  <variable> , <variable> + <offset>, hexdigit...
 * offset : 0xhexdigit..., integer
 *
 * note that integer constants are heavily hexadecimal.  the only place
 * where decimal is assumed is in <address>+<offset>, and that is overridden
 * whth an explicit 0x.
 */
#include <stdio.h>

#define	MAX_PATTERN 256

char *patchfile = "hitech.pat";
char *pname;
int verbose;
int pf;

typedef unsigned short word;

unsigned char membuf[65536];	/* binary image */
int objsize;

char patch[65536];				/* patch text */
char *p;						/* pointer into patch text */

int hit;						/* state of most recent match */
word matchaddr;

usage(char c)
{
	fprintf(stderr, "usage:\n%s [<options>] objectfile ...\n", pname);
	fprintf(stderr, "\t-v\t\tincrease verbosity\n");
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
	for (v = vars; v; v = v->next) {
		if (strcmp(n, v->name) == 0) return v->value;
	}
	return 0;	
}

void
putvar(char *n, word new)
{
	struct var *v;

	for (v = vars; v; v = v->next) {
		if (strcmp(n, v->name) == 0) {
			printf("putvar: name %s already present value %x new %x\n",
				n, v->value, new);
				v->value = new;
			return;
		}
	}
	v = malloc(sizeof(*v));
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

	while (*p && (*p == ' ' || *p == '\t'))
		p++;
	*s = '\0';

	while (*p && *p != '\n' && *p != ';' && *p != ' ' && *p != '\t') {
		*s++ = *p++;
		*s = '\0';
	}
	return getbuf;
}

skiptoeol()
{
	while (*p && *p != '\n') {
		p++;
	}
}

skipwhite()
{
	for (;*p ; p++) {
		if (*p == '\n') continue;
		if (*p == ' ') continue;
		if (*p == '\t') continue;
		if (*p == ';') {
			skiptoeol();
			continue;
		}
		break;
	}
}

wordmatch(char *m)
{
	int i = strlen(m);
	if (strncmp(p, m, i) == 0) {
		p += i;
		return 1;
	}
	return 0;
}

extract()
{
	if (verbose > 1) printf("extract\n");
	skiptoeol();
}

replace()
{
	if (verbose > 1) printf("replace\n");
	while (*p) {
		skipwhite();
		if (wordmatch("end")) {
			skiptoeol();
			break;
		}
		skiptoeol();
	}
	if (verbose > 1) printf("replace end\n");
}

word
nibble(char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	c |= 0x20;
	if (c >= 'a' && c <= 'f') return (c - 'a') + 0xa;
	printf("malformed hex number %c\n", c);
	return 0xffff;
}

word
decin(char *s)
{
	word r = 0;
	while (*s >= '0' && *s <= '9') {
		r *= 10;
		r += *s++ - '0';
	}
}

unsigned int
hexin(char *s)
{
	word r = 0;
	word d;
	
	while (*s) {
		d = nibble(*s);
		if (d == 0xffff)
			return 0xffffffff;
		r <<= 4;
		r += d;
		s++;
	}
	return r;
}

/*
 * this processes 00 - ff, ANY, and <variable>.L, variable.H
 * integer constants here are ALWAYS hex.
 *
 * there's a little syntactic sugar here, doing a substitute for
 * the words CALL and JUMP, and even a little layer violation when
 * a variable name or 4 byte hex number is found.  in this case, we return 2 packed
 * match bytes which the caller needs to unpack.
 */
unsigned int
patword()
{
	char *t = get();
	word r;
	int i;

	if (strcmp(t, "ANY") == 0) return 0xffff;
	if (strcmp(t, "CALL") == 0) return 0xcd;
	if (strcmp(t, "JUMP") == 0) return 0xc3;

	i = strlen(t);

	if (i == 2 && ((r = hexin(t)) != 0xffffffff)) {
		r = hexin(t);
		return r;
	}

	if (i == 4 && ((r = hexin(t)) != 0xffffffff)) {
		return r | 0xffff0000;
	}

	if (t[i-2] == "." && (t[i-1] == 'L' || t[i-1] == 'H')) {
		t[i-2] = '\0';
		r = getvar(t);
		if (r == 0xffff) {
			printf("variable %s not set\n", t);
		}
		if (t[i-1] == 'H') {
			r >>= 8;
		}
		r &= 0xff;
		return (r);
	}
	r = getvar(t);
	if (r == 0xffff) {
		printf("variable %s not set\n", t);
	}
	return 0xffff0000 | r;
}

int
patmatch(word *pat, int pl, word base)
{
	int i;
	for (i = 0; i < pl; i++) {
		if (pat[i] == 0xffff) continue;
		if (pat[i] == membuf[base + i]) continue;
		return 0;
	}
	matchaddr = base;
	return 1;
}

/*
 * given a string, return the address.
 * grammar: 
 * address :  <variable> , <variable> + <offset>, hexdigit...
 * 
 * LEXICAL NOTE: spaces are not tolerated anywhere in addresses.  there is no
 * space allowed before or after the '+'.  this is NOT a generic expression parser.
 */
word
getaddr(char *in)
{
	char *s;
	word v;
	word o = 0;

	v = hexin(in);
	if (v != 0xffffffff) return v;

	s = index(in, '+');
	if (s) {
		*s++ = '\0';
	}
	v = getvar(in);
	if (v == 0xffff) {
		printf("variable %s not set\n", in);
	}
	if (s) {
		if (s[0] == '0' && s[1] == 'x') {
			o = hexin(s+2);
		} else {
			o = decin(s);
		}
		if (o == 0xffffffff) {
			printf("malformed offset %s\n", s);
			o = 0;
		}
	}
}

/*
 * this function is really the engine of the patcher.
 * it does all the searching in the object file, and is where we spend the vast majority
 * of our time. first, we snarf the entire match text into a buffer, where we do value
 * substititions as needed.
 */
match()
{
	char *anchor;
	word *pat = malloc(MAX_PATTERN * 2);
	int pl = 0;
	int i;
	word base;

	anchor = get();
	hit = 0;
	if (anchor[0]) {
		base = getaddr(anchor);
	} else {
		anchor = 0;
	}

	if (verbose > 1) printf("match %s\n", anchor ? anchor : "unanchored");

	while (*p) {
		skipwhite();
		if (wordmatch("end")) {
			skiptoeol();
			break;
		}
		i = patword();
		if ((i & 0xffff0000) == 0xffff0000) {
			pat[pl++] = i & 0xff;
			i = (i >> 8) & 0xff;
		}
		pat[pl++] = i;
	}

	if (verbose > 1) {
		printf("checking match patlen %d\n", pl);
		for (i = 0; i < pl; i++) {
			printf("%04x ", pat[i]);
			if ((i % 8) == 7) printf("\n");
		}
		printf("\n");
	}

	if (anchor) {
		if (patmatch(pat, pl, base)) {
			hit = 1;
		}
	} else {
		for (base = 0; base < objsize; base++) {
			if (patmatch(pat, pl, base)) {
				if (hit) {
					printf("multiple hit\n");
				}
				hit = 1;
			}
		}
	}
	if (verbose > 1) {
		if (hit) {
			printf("hit at %04x\n", base);
		} else {
			printf("miss\n", base);
		}
		printf("match end\n");
	}
}

block()
{
	char *blockname;
	word addr;
	blockname = get();

	printf("block %s\n", blockname);

	skiptoeol();
	while (*p) {
		skipwhite();
		if (wordmatch("match")) {
			match();
			if (hit) {
				printf("block hit at %x\n", matchaddr);
			}
		}
		if (wordmatch("extract")) {
			extract();
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

process(char *fn)
{
	int fd;
	
	if (verbose) 
		printf("process %s\n", fn);
	fd = open(fn, 2);
	if (fd < 0) {
		perror(fn);
		exit(3);
	}
	objsize = read(fd, &membuf[0x100], sizeof(membuf) - 0x100);
	if (objsize < 0) {
		perror(fn);
		exit(4);
	}
	printf("read %d bytes\n", objsize);

	objsize += 0x100;

	for (p = patch; *p; p++) {
		skipwhite();
		if (wordmatch("block")) {
			block();
		}
	}
	close(fd);
}

int
main(int argc, char **argv)
{
	char *s;
	int i;

	pname = *argv++;
	argc--;

	while (**argv == '-') {
		
		for (s = ++*argv; *s; s++) switch(*s) {
		case 'v':
			verbose++;
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
		exit (2);
	}
	i = read(pf, patch, sizeof(patch));
	if (i < 0) {
		perror(pf);
		exit(5);
	}
	patch[i] = '\0';

	while (argc--) {
		process(*argv++);
	}
}
