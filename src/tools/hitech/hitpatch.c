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

/*
 * addresses grunged out of our object file
 */
addr_t a_bss;
addr_t a_end;
addr_t a_exit;
addr_t a_main;
addr_t a_startup;
addr_t a_getarg;
addr_t a_getuid;
addr_t a_setuid;

addr_t a_bdoshl;
addr_t a_bdos;
addr_t a_bdosa;

addr_t a_write;
addr_t a_csv;
addr_t a_ncsv;

/*
 * system call library functions
 */
addr_t a_writeaddr;

struct var {
	addr_t *varp;
	char *name;
};

struct var getarg_v = {
	&a_getarg, "getarg"
};

struct get {
	struct var v;
	addr_t where;
} crtvars[] = {
	{{ &a_bss, "bss" }, 0x105 },
	{{ &a_end, "end" }, 0x109 },
	{{ &a_startup, "startup" }, 0x12a },
	{{ &a_main, "_main" }, 0x134 },
	{{ &a_exit, "_exit" }, 0x138 },
	{0}
};

/*
 * patterns to matchg subroutines that contain a call to a known address
 */
struct pattern {
	char *name;		/* function */
	addr_t *var;	/* pointer where to store offset */
	int patoff;		/* where to start pattern match */
	short sig[40];
#define	ANY	-1
#define	END	-2
};

/*
 * zcrt
 */
short zcrtpat[] = {
	0x2a, 0x06, 0x00, 
	0xf9, 
	0x11, ANY, ANY, 
	0xb7, 
	0x21, ANY, ANY, 
	0xed, 0x52, 
	0x4d, 
	0x44, 
	0x0b, 0x6b, 0x62, 0x13, 
	0x36, 0x00, 
	0xed, 0xb0, 
	0x21, ANY, ANY, 
	0xe5, 
	0x21, 0x80, 0x00, 
	0x4e, 
	0x23, END
};

/*
 * there are only a few places that call 5.  find them all
 */
struct pattern entry_pats[] = {
	{ "bdos", &a_bdos, -16, { 
		0xcd, ANY, ANY, 
		0xdd, 0x5e, 0x08, 
		0xdd, 0x56, 0x09, 
		0xdd, 0x4e, 0x06,
		0xdd, 0xe5,
		0xfd, 0xe5,
		0xcd, 0x05, 0x00, 
		0xfd, 0xe1,
		0xdd, 0xe1,
		0x6f, 
		0x17, 
		0x9f, 
		0x67, 
		0xc3, ANY, ANY,  END } },
	{ "bdosa", &a_bdosa, -14, { 
		0xcd, ANY, ANY, 
		0xdd, 0x5e, 0x08, 
		0xdd, 0x56, 0x09, 
		0xdd, 0x4e, 0x06,
		0xdd, 0xe5,
		0xcd, 0x05, 0x00, 
		0xdd, 0xe1,
		0x6f, 
		0x17, 
		0x9f, 
		0x67, 
		0xc3, ANY, ANY,  END } },
	{ "bdoshl", &a_bdoshl, -14, {
		0xcd, ANY, ANY,
		0xdd, 0x5e, 0x08, 
		0xdd, 0x56, 0x09, 
		0xdd, 0x4e, 0x06,
		0xdd, 0xe5,
		0xcd, 0x05, 0x00, 
		0xdd, 0xe1,
		0xc3, ANY, ANY, END } },
	{ "getuid", &a_getuid, -9, { 
		0xcd, ANY, ANY, 
		0x0e, 0x20,
		0x1e, 0xff,
		0xdd, 0xe5,
		0xcd, 0x05, 0x00, END } },
	{ "setuid", &a_setuid, -10, {
		0xcd, ANY, ANY, 
		0xdd, 0x5e, 0x06,
		0x0e, 0x20,
		0xdd, 0xe5,
		0xcd, 0x05, 0x00, END } },
	{ 0 }
};

struct pattern bdos_pats[] = {
	{ "write", &a_write, -0x6c, {
		0xcd, ANY, ANY, 
		0x79, 0xff, 
		0x06, 0x08, 0xdd, 
		  0x7e, 0x06, 0xcd, ANY, ANY, 0x38, 0x06, 0x21,
		  0xff, 0xff, 0x11, 0x2a, 0x00, 0xdd, 0x6e, 0x06, END } },
	{ 0 }
};

getptr(struct var *v, addr_t where)
{
	addr_t a;
	a = mem[where] + ((mem[where+1]) << 8);
	*v->varp = a;

	if (verbose) printf("%s: %04x\n", v->name, a);
}

/*
 * match pattern p against memory b
 */
int
patmatch(short *p, unsigned char *b)
{
	while (*p != END) {
		if (*p == ANY) {
			p++; b++; continue;
		}
		if (*p++ != *b++) return 0;
	}
	return 1;
}

/*
 * iterate through all the patterns checking for hits
 */
struct pat *
lookup_pat(int addr, struct pattern *p)
{
	while (p->name) {
		if (patmatch(p->sig, &mem[addr + p->patoff]))
			return p;
		p++;
	}
	return 0;
}

/*
 * gripe and die
 */
lose(char *f, char *s)
{
	fprintf(stderr, "lose: %s %s\n", f, s);
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
	while (argc--) {
		process(*argv++);
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
	struct pattern *p;

	if (verbose) printf("do file %s\n", fn);

	if (stat(fn, &sbuf) != 0) {
		lose(fn, "cannot stat");
	}
	size = sbuf.st_size;
	
	mem = calloc(65536, 1);
	if (!mem) lose("cannot allocate memory", "");
	if (verbose) printf("size = %d\n", size);
	fd = open(fn, 0);
	if (fd < 0) {
		lose(fn, "can't open");
	}
	i = read(fd, &mem[0x100], 65536);
	if (i != size) {
		lose(fn, "file length read");
	}
	close(fd);

	/*
	 * first, make sure it's a hitech C binary, with
	 * the zcrt prolog.
	 */
	if (!patmatch(zcrtpat, &mem[0x100])) {
		fprintf(stderr, "%s: not a hitech binary\n", fn);
		return;
	}
	if (verbose) printf("%s: hitech detected\n", fn);	

	/* read addresses from crt */
	for (i = 0; crtvars[i].v.name; i++) {
		getptr(&crtvars[i].v, crtvars[i].where);
	}
	/* get address of getarg */
	getptr(&getarg_v, a_startup + 1);

	/*
	 * let's find all the bdos calls and classify them
	 */
	for (i = 0x100; (i = next_callto(i, 0x0005)) != 0xffff; i += 3) {
		p = lookup_pat(i, entry_pats);
		if (!p) {
			printf("unknown function at %x\n", i);
			dumpmem(memget, i-0x20, 0x40);
		} else {
			printf("function %s at %x\n", p->name, i + p->patoff);
			*p->var = i + p->patoff;
		}
	}

	/*
	 * now, let's find all the calls to the ones we just found
	 */
	if (a_bdos) {
		for (i = 0x100; (i = next_callto(i, a_bdos)) != 0xffff; i += 3) {
			printf("bdos call at %x\n", i);
		}
	}

	if (a_bdosa) {
		for (i = 0x100; (i = next_callto(i, a_bdosa)) != 0xffff; i += 3) {
			printf("bdosa call at %x\n", i);
		}
	}

	if (a_bdoshl) {
		for (i = 0x100; (i = next_callto(i, a_bdoshl)) != 0xffff; i += 3) {
			printf("bdoshl call at %x\n", i);
		}
	}
}

