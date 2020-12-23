
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

unsigned char sbuf[1024];

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

int
getstring(int o)
{
	int i = 0;

	while (1) {
		if (read(fd, &sbuf[i], 1) != 1) exit(2);
		if (sbuf[i++] == 0) {
			break;
		}
	}
	if (o) printf("string: (%d) %s\n", i, sbuf);
	return i;
}

int8
getint8(int o)
{
	int8 v;

	if (read(fd, sbuf, 1) != 1) exit(2);
	v =	sbuf[0];
	if (o) printf("int8: 0x%02x %d\n", v, v); 
	return v;
}

int16
getint16(int o)
{
	int16 v;

	if (read(fd, sbuf, 2) != 2) exit(2);
	v =	sbuf[0] + (sbuf[1] << 8);
	if (o) printf("int16: 0x%04x %d\n", v, v); 
	return v;
}

int32
getint32(int o)
{
	int32 v;

	if (read(fd, sbuf, 4) != 4) exit(2);
	v = sbuf[0] + 
		(sbuf[1] << 8) + 
		(sbuf[2] << 16) +
		(sbuf[3] << 24);

	if (o) printf("int32: 0x%08x %d\n", v, v);
	return v;
}

reblock(int n)
{
	long off;
	off = lseek(fd, 0, 1);
	block(n);
	lseek(fd, off, 0);
}

block(int n)
{
	if (read(fd, sbuf, n) != n) exit(2);
	dumpmem(sbuf, 0, n);
}

int
getrec()
{
	int l;
	int t;
	int off;
	int fl;
	int addr;

	int i0;
	int i1;
	char *seg;

	l = getint16(0);			// rec length
	t = getint8(0);				// rec code

	switch (t) {
		default:
		case 0:
			printf("\nrecord type unknown %d len: %d\n", t, l);
			block(l);
			break;
		case 1:				// code
			i0 = getint16(0);
			i1 = getint16(0);
			l -= getstring(0) + 4;
			printf("\n%s segment record  bytes: %d i0: 0x%04x i1: 0x%04x\n", 
				sbuf, l, i0, i1);
			block(l);
			break;
		case 2:
			off = getint16(0);
			l -= getstring(0) + 2;
			printf("\nmystery type 2 record 0x%04x %s len: %d\n", 
				off, sbuf[0] ? sbuf : "(no name)", l);
			break;
		case 3:
			printf("\nrelocations record len: %d\n", l);
			while (l) {
				off = getint16(0);
				fl = getint8(0);
				l -= getstring(0) + 3;
				printf("0x%04x type 0x%02x %s\n", off, fl, sbuf);
			}
			break;	
		case 4:
			printf("\nsymbols record len: %d\n", l);
			reblock(l);

			while (l) {
				addr = getint16(0);
				i0 = getint16(0);
				i1 = getint16(0);
				l -= 6;
				l -= getstring(0);

				if (strcmp(sbuf, "bss") == 0) seg = "bss";
				else if (strcmp(sbuf, "data") == 0) seg = "data";
				else if (strcmp(sbuf, "text") == 0) seg = "text";
				else if (sbuf[0] == 0) seg = "";
				else seg = strdup(sbuf);

				l -= getstring(0);
				printf("%04x %04x %04x %4s %s\n", addr, i0, i1, seg, sbuf);
			}
			break;	
		case 6:
			off = getint16(0);
			printf("\nend record 0x%04x len: %d\n", off, l);
			break;
	}
	return t;
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

	magic = getint16(0);

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

	if (getint8(0) != 7) {
		printf("bad header length\n");
		return;
	}
	if (getint32(0) != 0x03020100L) {
		printf("bad byte sex\n");
		return;
	}
	if (getint16(0) != 0x100) {
		printf("bad block alignment\n");
		return;
	}
	if ((getstring(0) != 4) || (strcmp(sbuf, "Z80") != 0)) {
		printf("bad cpu: %s\n", sbuf);
		return;
	}	

	printf("hitech Z80, little-endian, 256 byte page\n");

	while (getrec() != 6)
		;
}

void
main(argc, argv)
int argc;
char **argv;
{
	if (argc < 2) {
		exit(1);
	}
	while (--argc) {
		process(*++argv);
	}
}

