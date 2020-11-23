/*
 * patch a hitech-c compiled binary for CP/M to run on micronix
 *
 * it turns out that the hitech compiler suite uses a unix-like libc
 * for i/o.  so, we grunge through the binaries looking for bdos calls,
 * and then go up the call tree until we find the unix api functions.
 * then, we replace them with micronix calls.
 * finally, we scribble an appropriate micronix exec struct at the
 * start of the file and presto, we have a micronix hitech-c.
 */
#include <stdio.h>
#include <sys/stat.h>

char *pbuf[80];
int verbose;
char *pname;
unsigned char *mem;
int traceflags;

typedef unsigned short addr_t;

addr_t a_getuid;
addr_t a_setuid;
addr_t a_bdoshl;
addr_t a_bdosa;

struct pattern {
	char *name;
	addr_t *var;
	int baseoffset;
	int patoff;
	int patlen;
	short p[20];	/* -1 matches any byte */
};

struct pattern pats[] = {
	{ "getuid", &a_getuid, 9, -6, 6, 
		{ 0x0e, 0x20, 0x1e, 0xff, 0xdd, 0xe5 } },
	{ "setuid", &a_setuid, 9, 3, 6, 
		{ 0xdd, 0x5e, 0x08, 0x1e, 0xff, 0xdd, 0xe5 } },
	{ "bdoshl", &a_bdoshl, 9, 3, 6, 
		{ 0xdd, 0x5e, 0x08, 0x1e, 0xff, 0xdd, 0xe5 } },
	{ "bdosa", &a_bdosa, 9, 3, 6, 
		{ 0xdd, 0x5e, 0x08, 0x1e, 0xff, 0xdd, 0xe5 } },
	{ 0 }
};

#ifdef nodef
			printf("bdos called at %x\n", i);
			if (mem[i-6] == 0xe && mem[i-5] == 0x20) {
				s = "getuser";
			}
			if (mem[i-4] == 0xe && mem[i-3] == 0x20) {
				s = "setuser";
			}
			if (mem[i+3] == 0xdd && mem[i+4] == 0xe1 && mem[i+5] == 0xc3) {
				s = "bdoshl";
			}
			if (mem[i+3] == 0xfd && mem[i+4] == 0xe1 && mem[i+5] == 0xdd) {
				s = "bdosa";
			}
#endif

/*
 * match pattern p against memory b for len
 */
int
patmatch(short *p, unsigned char *b, int len)
{
	while (len--) {
		if (*p == -1) {
			p++; b++; continue;
		}
		if (*p++ != *b++) return 0;
	}
	return 1;
}

/*
 * iterate through all the patterns checking for hits
 */
int
lookup_pat(int addr)
{
	int i;
	for (i = 0; pats[i].name; i++) {
		if (patmatch(pats[i].p, &mem[addr + pats[i].patoff], pats[i].patlen))
			return i;
	}
	return -1;
}

/*
 * gripe and die
 */
lose(char *s)
{
	fprintf(stderr, "lose: %d\n", s);
	exit(1);
}

usage(char *s)
{
	fprintf(stderr, s);
	fprintf(stderr, "usage: %s [options] filename\n", pname);
	fprintf(stderr, "\t-v\tincrement verbosity\n");
	exit (1);
}

main(int argc, char **argv)
{
	char o;

	pname = *argv++;

	while (--argc) {
		if (**argv == '-') {
			(*argv)++;
			o = **argv;
			switch (o) {
			case 'h':
				usage("");
				break;
			case 'v':
				verbose++;
				break;
			default:
				sprintf("unknown option: %c\n", o);
				usage(pbuf);
				break;
			}
		} else break;
		argv++;
	}	
	printf("verbose = %d\n", verbose);
	if (argc) {
		process(*argv);
	}
	exit (0);
}

/*
 * memdump hook
 */
unsigned char 
memget(addr_t addr)
{
	return mem[addr];
}

/*
 * given a starting address base, find the next call to addr dest
 */
addr_t
next_callto(addr_t base, addr_t dest)
{
	addr_t a;
	unsigned char lo = dest & 0xff;
	unsigned char hi = (dest >> 8) & 0xff;

	for (a = base; a < 65533; a++) {
		if (mem[a] == 0xcd && mem[a+1] == lo && mem[a+2] == hi) {
			return a;
		}
	}
	return 0xffff;
}

/*
 * first, suck the entire .com file into memory
 */
process(char *fn)
{
	struct stat sbuf;
	int size;
	int fd;
	addr_t i;
	int j;

	if (verbose) printf("do file %s\n", fn);

	if (stat(fn, &sbuf) != 0) {
		lose("cannot stat");
	}
	size = sbuf.st_size;
	
	mem = calloc(65536, 1);
	if (!mem) lose("cannot allocate memory");
	if (verbose) printf("size = %d\n", size);
	fd = open(fn, 0);
	i = read(fd, &mem[0x100], 65536);
	if (i != size) {
		lose("file length read");
	}

	/*
	 * let's find all the bdos calls and classify them
	 */
	for (i = 0x100; (i = next_callto(i, 0x0005)) != 0xffff; i += 3) {
		j = lookup_pat(i);
		if (j == -1) {
			printf("unknown function at %x\n", i);
			dumpmem(memget, i-0x20, 0x40);
		} else {
			printf("function %s at %x\n", 
				pats[j].name, i - pats[j].baseoffset);
			dumpmem(memget, i-0x20, 0x40);
		}
	}
}
