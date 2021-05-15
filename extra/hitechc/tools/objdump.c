
int fd;

typedef unsigned char int8;
typedef unsigned short int16;
typedef unsigned int int32;

struct htobj {
	char stuff[9];
} obj;

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
			printf("\nrecord type unknown %d\n", t);
			block(l);
			break;
		case 1:				// code
			i0 = getint16(0);
			i1 = getint16(0);
			l -= getstring(0) + 4;
			printf("\n%s segment record  bytes: %d i0: 0x%04x i1: 0x%04x\n", sbuf, l, i0, i1);
			block(l);
			break;
		case 2:
			off = getint16(0);
			l -= getstring(0) + 2;
			printf("\nmystery type 2 record 0x%04x %s\n", off, sbuf[0] ? sbuf : "(no name)");
			break;
		case 3:
			printf("\nrelocations record\n");
			while (l) {
				off = getint16(0);
				fl = getint8(0);
				l -= getstring(0) + 3;
				printf("0x%04x type 0x%02x %s\n", off, fl, sbuf);
			}
			break;	
		case 4:
			printf("\nsymbols record %d %x\n", l, l);
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
				printf("%04x %04x %04x %4s %s\n", i0, i1, addr, seg, sbuf);
			}
			break;	
		case 6:
			off = getint16(0);
			printf("\nend record 0x%04x\n", off);
			break;
	}
	return t;
}

process(name)
char *name;
{
	int magic;

	fd = open(name, 0);

	if (getint16(0) != 0xa) {
		printf("bad magic\n");
		goto lose;
	}
	if (getint8(0) != 7) {
		printf("bad header length\n");
		goto lose;
	}
	if (getint32(0) != 0x03020100L) {
		printf("bad byte sex\n");
		goto lose;
	}
	if (getint16(0) != 0x100) {
		printf("bad block alignment\n");
		goto lose;
	}
	if ((getstring(0) != 4) || (strcmp(sbuf, "Z80") != 0)) {
		printf("bad cpu: %s\n", sbuf);
		goto lose;
	}	

	printf("hitech Z80, little-endian, 256 byte page\n");

	while (getrec() != 6)
		;

lose:
	close(fd);
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

