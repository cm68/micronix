/*
 * hitech C object dumper
 *
 * tools/hitech/objdump.c
 * Changed: <2023-06-25 03:57:27 curt>
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int size;

int debug;

int fd;

typedef unsigned char int8;
typedef unsigned short int16;
typedef unsigned int int32;

struct htobj {
	char stuff[9];
} obj;

struct ar_idx {
	int16 reclen;
	int16 syms;
	int16 filesize;
	int16 i[3];
} ar_idx;

struct segment {
    char *name;
    int size;
    struct segment *next;
} *segments;

unsigned char sbuf[1024];

void
increase_segment(char *name, int size)
{
    struct segment *s;

    for (s = segments; s; s = s->next) {
        if (strcmp(s->name, name) == 0) {
            s->size += size;
            return;
        }
    }
    s = malloc(sizeof(*s));
    s->name = strdup(name);
    s->size = size;
    s->next = segments;
    segments = s;
}

int
segsize(char *name)
{
    struct segment *s;

    for (s = segments; s; s = s->next) {
        if (strcmp(s->name, name) == 0) {
            return s->size;
        }
    }
    return 0;
}

/*
 * dump out the sanitized ascii
 */
static char pchars[16];
static int pcol;

static void
dp()
{
    int i;
    char c;

    for (i = 0; i < pcol; i++) {
        c = pchars[i];
        if ((c <= 0x20) || (c >= 0x7f))
            c = '.';
        printf("%c", c);
        if ((i % 4) == 3) { printf(" "); }
    }
    printf("\n");
}

void
dumpmem(char *base, unsigned short addr, int len)
{
    int i;
    char c;

    pcol = 0;
 
    while (len) {
        if (pcol == 0)
            printf("%04x: ", addr);
        c = base[addr++];
        pchars[pcol] = c;
        printf("%02x ", c & 0xff);
        if ((pcol % 4) == 3) { printf(" "); }
        len--;
        if (pcol++ == 15) {
            dp();
            pcol = 0;
        }
    }
    // fill the line with pad
    if (pcol != 0) {
        for (i = pcol; i < 16; i++)
            printf("   ");
        dp();
    }
}

/*
 * read a null-terminated string into our destination
 * and return the count of characters consumed, including
 * the null.
 */
int
getstring(char *dest)
{
	int i = 0;
    char c;

    while (1) {
        if (read(fd, &c, 1) != 1) exit(2);
        dest[i++] = c;
		if (c == 0) {
			break;
		}
	}
	if (debug) printf("string: (%d) %s\n", i, dest);
	return i;
}

int8
getint8()
{
	int8 v;

	if (read(fd, &v, 1) != 1) exit(2);
	if (debug) printf("int8: 0x%02x %d\n", v, v); 
	return v;
}

int16
getint16()
{
	int16 v;

	if (read(fd, sbuf, 2) != 2) exit(2);
	v =	sbuf[0] + (sbuf[1] << 8);
	if (debug) printf("int16: 0x%04x %d\n", v, v); 
	return v;
}

int32
getint32()
{
	int32 v;

	if (read(fd, sbuf, 4) != 4) exit(2);
	v = sbuf[0] + 
		(sbuf[1] << 8) + 
		(sbuf[2] << 16) +
		(sbuf[3] << 24);

	if (debug) printf("int32: 0x%08x %d\n", v, v);
	return v;
}

void
block(int n)
{
	if (read(fd, sbuf, n) != n) exit(2);
    if (!size)
        dumpmem(sbuf, 0, n);
}

void
reblock(int n)
{
	long off;
	off = lseek(fd, 0, 1);
	block(n);
	lseek(fd, off, 0);
}

/*
 * process a record
 * <16 bit length> <8 bit record type> <data>
 */
#define REC_TEXT    1
#define REC_PSECT   2
#define REC_RELOC   3
#define REC_SYM     4
#define REC_START   5
#define REC_END     6
#define REC_IDENT   7
#define REC_XPSECT  8
#define REC_SEGMENT 9
#define REC_XSYM    10
#define REC_SIGNAT  11
#define REC_FNINFO  12
#define REC_FNCONF  13

char *recnames[] = {
    "unknown",
    "text",
    "psect",
    "reloc",
    "sym",
    "start",
    "end",
    "ident",
    "xpsect",
    "segment",
    "xsym",
    "signat",
    "fninfo",
    "fnconf"
};

char *stype[] = {
    "", "stack ", "comm ", "regname ", 
    "lineno ", "filename ", "extern "
};

char flbuf[40];

char *
fflag(int16 secflag)
{
    int t;

    flbuf[0] = 0;
    t = secflag & 0xf;
    if (t < 7) strcat(flbuf, stype[t]);
    
    if (secflag & 020) strcat(flbuf, "GLOBAL ");
    if (secflag & 040) strcat(flbuf, "PURE ");
    if (secflag & 0100) strcat(flbuf, "OVRLD ");
    if (secflag & 0200) strcat(flbuf, "ABS ");
    if (secflag & 0400) strcat(flbuf, "BIGSEG ");
    if (secflag & 01000) strcat(flbuf, "BPAGE ");
    return flbuf;
}

int
getrec()
{
	int16 reclen;
	int8 rectype;
    int32 offset;
	int16 flag16;
	int8 flag8;
    char name1[30];
    char name2[30];

	int16 roff;

	reclen = getint16();
	rectype = getint8();

    if (!size)
        printf("\n%s record reclen %d\n", recnames[rectype], reclen);

	switch (rectype) {
		default:
		case 0:
			block(reclen);
			break;
		case REC_TEXT:
			offset = getint32();
			reclen -= getstring(name1) + 4;
            if (!size)
                printf("\n%s segment record offset: %x bytes: %d\n", 
                    name1, offset, reclen);
			block(reclen);
            /* special case hack for bss */
            if (reclen == 0) {
                if (strcmp(name1, "bss") != 0) {
                    printf("fung wha reclen 0\n");
                }
                increase_segment(name1, offset);
            } else {
                increase_segment(name1, reclen);
            }
            reclen -= reclen;
			break;
		case REC_PSECT:
			flag16 = getint16();
			reclen -= getstring(name1) + 2;
            if (!size)
                printf("\npsect: %s flags %o %s len: %d\n", 
                    name1[0] ? name1 : "(no name)", flag16, 
                    fflag(flag16), reclen);
			break;
		case REC_RELOC:
			while (reclen) {
                char *subtype = 0;

				roff = getint16();
				flag8 = getint8();
                reclen -= 3;
                switch ((flag8 & 0xf0) >> 4) {
                case 0:
                    reclen -= getstring(name1);
                    subtype = "absolute";
                    break;
                case 1:
                    reclen -= getstring(name1);
                    subtype = "segoff";
                    break;
                case 2:
                    reclen -= getstring(name1);
                    subtype = "symoff";
                    break;
                case 5:
                    reclen -= getstring(name1);
                    subtype = "pcrel";
                    break;
                case 6:
                    reclen -= getstring(name1);
                    subtype = "symrel";
                    break;
                case 9:
                    reclen -= getstring(name1);
                    subtype = "relsegsec";
                    break;
                case 10:
                    reclen -= getstring(name1);
                    subtype = "relsegsym";
                    break;
                default:
                    subtype = "fungwha";
                    printf("reloc fung wha %x ", flag8);
                    break;
                }
                if (!size)
                    printf("%x %s %d %s\n", roff, subtype, flag8 & 0xf, name1);
			}
			break;	
		case REC_SYM:
			while (reclen) {
				offset = getint32();
                flag16 = getint16();
				reclen -= getstring(name1) + 6;
				reclen -= getstring(name2);
				if (!size) 
                    printf("%-10s %-10s %8x %6o %s\n", 
                        name2, name1, offset, flag16, fflag(flag16));
			}
			break;	
        case REC_START:
			offset = getint32();
		    reclen -= getstring(name1) + 4;
			if (!size)
                printf("\nstart record offset %d name %s\n", offset, name1);
            break;

		case REC_END:
			flag16 = getint16(0);
		    reclen -= 2;
			if (!size)
                printf("\nflags 0x%04x\n", flag16);
			break;

        case REC_IDENT:
	        offset = getint32();
	        flag16 = getint16();
            reclen -= getstring(name1) + 6;
            if (reclen) {
                roff = getint16();
                reclen -= 2;
            } else {
                roff = 0;
            }
            if (!size)
                printf("%s %x %x %x\n", name1, offset, flag16, roff);
            break; 
	}
    if (reclen != 0) {
        printf("runt? %d\n", reclen);
    }
	return rectype;
}

process(name)
char *name;
{
	fd = open(name, 0);
	doobject(fd);
	close(fd);
}

char **filename;

doobject(fd)
int fd;
{
	int magic;
	int i;
	int files;
	int j;
	int flag;

#ifdef notdef
	magic = getint16();

	/*
	 * hitech archives don't have a magic number.  the first word is
	 * the size of the dictionary. 
	 * so, assume the object file is a library
	 */
	if (magic != 0xa) {
		files = getint16(0);
		reblock(magic);
		filename = malloc(sizeof(char *) * files);
		printf("archive of %d files\n", files);
		for (i = 0; i < files; i++) {
			read(fd, &ar_idx, sizeof(ar_idx));
			getstring(0);
			filename[i] = strdup(sbuf);
			printf("\t%s: size: %d syms: %d\n",
				sbuf, ar_idx.filesize, ar_idx.syms);
			for (j = 0; j < ar_idx.syms; j++) {
				flag = getint8(0);
				getstring(0);
				printf("\t\t%10s: %d\n", sbuf, flag);
			}
		}
		for (i = 0; i < files; i++) {
			printf("\narchive member %d: %s\n", i, filename[i]);
			doobject(fd);
		}
		return;
	}
#endif

	while (getrec() != REC_END)
		;
}

void
main(argc, argv)
int argc;
char **argv;
{
    char *s;
    s = rindex(argv[0], '/');
    if (s) {
        argv[0] = s+1;
    }

    if (strcmp(argv[0], "size") == 0) {
        size++;
        printf("text\tdata\tbss\tdec\thex\tfilename\n");
    }

	if (argc < 2) {
		exit(1);
	}
	while (--argc) {
		process(*++argv);
        if (1) {
            int total;
            int i;
            struct segment *sp;

            printf("%d\t", i = segsize("text")); total = i;
            printf("%d\t", i = segsize("data")); total += i;
            printf("%d\t", i = segsize("bss")); total += i;
            printf("%d\t%x\t%s\n", total, total, *argv);
            for (sp = segments; sp; sp = sp->next)
                sp->size = 0;
        }
	}
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
