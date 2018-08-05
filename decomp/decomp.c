/*
 * vim: set tabstop=4 shiftwidth=4 expandtab:
 */
#include <fcntl.h>

#include "z80test.h"

#define	V_DATA	0x01
char           *vopts[] = {"V_DATA", 0};
int		verbose;

char		instbuf   [100];

char		mem       [64 * 1024];

unsigned char	pchars[16];
int		pcol;
char *progname;

unsigned char
getmem(unsigned short addr)
{
    return mem[addr];
}
void
usage(char *complaint, char *p)
{
    int		    i;

    printf("%s", complaint);
    printf("usage: %s [<options>] [program [<program options>]]\n", p);
    printf("\t-v <verbosity>\n");
    printf("\t-c call=bytes\n");
    for (i = 0; vopts[i]; i++) {
        printf("\t%x %s\n", 1 << i, vopts[i]);
    }
    exit(1);
}

struct special {
	unsigned short addr;
	unsigned char extra;
	struct special *next;
} *specials;

unsigned char 
is_special(unsigned short dest)
{
    struct special *sp;

    for (sp = specials ; sp; sp = sp->next) {
        if (sp->addr == dest) {
            return sp->extra;
        }
    }
    return 0;
}

main(int argc, char **argv)
{
    int		    fd;
    int		    ret;
    int		    pc;
    int		    bytes;
    int		    i;
    struct special *sp;
    char *s;
    int pass;

    progname = *argv++;
    argc--;

    while (argc) {
		s = *argv;

		/* end of flagged options */
		if (*s++ != '-')
			break;

		argv++;
		argc--;

		/* s is the flagged arg string */
		while (*s) {
			switch (*s++) {
			case 'h':
				usage("", progname);
				break;
			case 'v':
				if (!argc--) {
					usage("verbosity not specified \n", progname);
				}
				verbose = strtol(*argv++, 0, 0);
				break;
			case 'c':
				if (!argc--) {
					usage("special call not specified \n",
						progname);
				}
				s = *argv++;
				sp = malloc(sizeof(*sp));
				sp->addr = strtol(s, &s, 0);
				if (*s == '=') {
					s++;
					sp->extra = strtol(s, &s, 0);
				} else {
					sp->extra = 2;
                }
				sp->next = specials;
				specials = sp;
				break;
			default:
				printf("bad flag %c\n", (*s));
				break;
			}
		}
    }

    if (argc) {
        s = *argv;
		fd = open(s, O_RDONLY);
		if (fd < 0) {
			perror(s);
			exit(1);
		}
		ret = read(fd, &mem[0x100], sizeof(mem) - 0x100);
		if (ret < 0) {
			perror("read binary");
			exit(1);
		}
		printf("read %d from %s\n", ret, s);
    } else {
		ret = sizeof(testvec);
		memcpy(&mem[0x100], testvec, ret);
    }

    for (sp = specials ; sp; sp = sp->next) {
        printf("special %x %d\n", sp->addr, sp->extra); 
    }

    makework(0x100);

    /* start at entry */
    bytes = do_instr(instbuf, pc);
            for (i = 0; i < 6; i++) {
                if (i < bytes) {
                    printf("%02x ", getmem(pc + i));
                } else {
                    printf("   ");
                }
            }
            printf("%s\n", instbuf);
            pc += bytes;
        }

}

char		cbuf      [20];
char		cbufi;

char           *
ncbuf()
{
    cbufi ^= 1;
    if (cbufi) {
	return &cbuf[10];
    } else {
	return &cbuf[0];
    }
}

char           *
const16(unsigned short a)
{
    char           *s = ncbuf();

    sprintf(s, "0x%04x", a);
    return s;
}

char           *
const8(unsigned char a)
{
    char           *s = ncbuf();

    sprintf(s, "0x%02x", a);
    return s;
}

char           *
addr(unsigned short a)
{
    return const16(a);
}

char		ibuf      [20];

char           *
signedoff(unsigned char i)
{
    char	    off;

    off = i;
    if (off < 0) {
	sprintf(ibuf, "-%d", -off);
    } else {
	sprintf(ibuf, "+%d", off);
    }
    return ibuf;
}

char           *
reladdr(unsigned short pc, unsigned char a)
{
    char	    off;
    off = a;

    pc = pc + 2 + off;
    sprintf(ibuf, "%x", pc);

    return ibuf;
}
