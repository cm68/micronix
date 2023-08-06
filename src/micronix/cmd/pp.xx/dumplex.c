/*
 * dump out lex symbols as produced by pp
 *
 * pp/dumplex.c
 *
 * Changed: <2023-06-27 20:21:41 curt>
 */

#include <stdio.h>
#include "int01.h"
#include "int012.h"

char strbuf[100];

char *
getstring()
{
    int i;
    char *s = strbuf;

    i = getchar();
    while (i--) {
        *s++ = getchar();
    }
    *s = '\0';
    return (strbuf);
}

unsigned short 
get16()
{
    return getchar() + (getchar() * 256);
}

unsigned int 
get32()
{
    return 
        (getchar() * (256 * 256 * 256)) + 
        (getchar() * (256 * 256)) + 
        (getchar()) + 
        (getchar() * (256));
}

usage(char *progname, char c)
{
    fprintf(stderr, "%s: usage\n", progname);
    fprintf(stderr, "-c\tdon't drop comments, continues\n");
    fprintf(stderr, "-d name\tdefine name as 1\n");
    fprintf(stderr, "-i name\tadd name to include path\n");
    fprintf(stderr, "-o name\toutput to name\n");
    fprintf(stderr, "-px\tuse x instead of #\n");
    fprintf(stderr, "-sx\tuse x in addition to px\n");
    fprintf(stderr, "-x\toutput lexemes\n");
    fprintf(stderr, "-6\tput SOH, extra newlines for v6 compiler\n");
    fprintf(stderr, "-Dname[=value]\n");
    fprintf(stderr, "-I<includepath>\n");
    if (c)
        fprintf(stderr, "unknown option %c\n", c);
    exit(1);
}

int cflag;
int v6flag;
int xflag;
int pchar;
int schar;
char *iprefix;
char *ofile;

struct incpath {
    struct incpath *next;
    char *path;
} *includes;

add_def(char *s, char *v)
{
    fprintf(stderr, "define %s as %s\n", s, v);
}

/*
 * chase down the chain, leaving p pointing at the tail
 */
add_incpath(char *s)
{
    struct incpath **p = &includes;
    while (*p) {
        p = &((*p)->next);
    }
    *p = malloc(sizeof(struct incpath));
    (*p)->path = strdup(s);
    (*p)->next = 0;

}

dump_includes()
{
    struct incpath *p;
    printf("includes: \n");
    for (p = includes; p; p = p->next) {
        printf("include: %s\n", p->path);
    }
}

int
main(int ac, char **av)
{
    int argc = ac - 1;
    char **argv = &av[1];
	unsigned char c;
    char *s;
    char *v;

    while (argc) {
        if ((*(s = *argv)) == '-') {
            while ((c = *++s)) switch (c) {
            case 'c':
                cflag=1;
                break;
            case 'x':
                xflag=1;
                break;
            case '6':
                v6flag=1;
                break;
            case 'p':
                pchar = *++s;
                break;
            case 's':
                schar = *++s;
                break;
            case 'd':
                add_def(*++argv, "1");
                argc--;
                s += strlen(s) - 1;
                break;
            case 'o':
                ofile = *++argv;
                argc--;
                s += strlen(s) - 1;
                break;
            case 'i':
                iprefix = *++argv;
                argc--;
                s += strlen(s) - 1;
                break;
            case 'D':
                v = index(++s,'=');
                if (v) {
                    *v++ = '\0';
                    add_def(s, v);
                } else {
                    add_def(s, "1");
                }                
                s += strlen(s) - 1;
                break;
            case 'I':
                add_incpath(++s);
                s += strlen(s) - 1;
                break;
            default:
                usage(*av, c);
                break;
            }
        }
        argv++;
        argc--;
    }
    fprintf(stderr, "c: %d v6: %d x: %d p: %d s: %d prefix: %s ofile: %s\n",
        cflag, v6flag, xflag, pchar, schar, iprefix, ofile);
    dump_includes();

    exit (0);

	while (!feof(stdin)) {
		c = (unsigned char)getchar();	
        switch (c) {
        case LLINENO:
            printf("line: %d\n", get16());
            break;
        case LIFILE:
            printf("file: %s\n", getstring());
            break;
        case LIDENT:
            printf("ident: %s\n", getstring());
            break;
        case LLPAREN:
            printf("(\n");
            break;
        case LRPAREN:
            printf(")\n");
            break;
        case LLCURLY:
            printf("{\n");
            break;
        case LRCURLY:
            printf("}\n");
            break;
        case LSCOLON:
            printf(";\n");
            break;
        case LINT:
            printf("int\n");
            break;
        case LGETS:
            printf("=\n");
            break;
        case LCNUM:
            printf("%d\n", get32());
            break;
        case LSNUM:
            printf("%d\n", get32());
            break;
        case LPLUS:
            printf("+\n");
            break;
        case LGPLU:
            printf("=+\n");
            break;
        case 0xff:
            break;
        default:
            printf("unknown: %03o\n", c);
            break;
        }
	}
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
