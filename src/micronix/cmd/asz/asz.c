/*
 * asz main file - arg processing and file handling
 * used to be trasm.  see README and LICENSE files
 *
 * /usr/src/cmd/asz/asz.c
 *
 * this file has interpolated the original sio.c
 * because it became almost trivial after the buffer stuff was
 * stripped out
 *
 * this file mostly rewritten because it had truly lame file name
 * handling:  output only went to a.out, and all specified files
 * were assembled into it.
 *
 * now, instead, for a file foo.s, we write foo.o as the gods intended
 *
 * on top of that, we assemble stdin to a.out
 */

#ifdef linux
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#else
#include <stdio.h>
#endif

#include "asm.h"

#ifdef DEBUG
char verbose;
#endif

char m_flag;
char n_flag;
char no_relax;  /* -8: no jp->jr relaxation (8080 mode) */

char *progname;

int lineNum;

FILE *outfp;
FILE *tmpfp;
FILE *relfp;            /* relocation scratch - see add_reloc in asm.c */
FILE *infp;
FILE *inbuffp;

/*
 * outputs a byte onto output file
 *
 * out = byte to output
 */
void
outbyte(out)
char out;
{

#ifdef DEBUG
    if (verbose > 4)
        printf("outbyte: 0x%x\n", out);
#endif

    fputc(out, outfp);
}

/*
 * writes a byte to the temp file
 *
 * tmp = byte to write to tmp
 */
void
outtmp(tmp)
char tmp;
{
#ifdef DEBUG
    if (verbose > 4)
        printf("outtmp: 0x%x\n", tmp);
#endif
    fputc(tmp, tmpfp);
}

/*
 * Signal handler for assembly timeout
 *
 * Catches SIGALRM to detect infinite loops or hangs during assembly.
 * The main() function sets a 5-second alarm that triggers this handler
 * if assembly doesn't complete in time.
 */
#ifdef linux
void
timeoutHdlr(int sig)
{
    fprintf(stderr, "\n\n*** TIMEOUT after 5 seconds ***\n");
    exit(1);
}
#endif

/*
 * print usage message
 */
void
usage()
{
	fprintf(stderr, "usage: %s [-vmn98l] [ -o <objectfile> ] [<sourcefile>]\n", progname);
	fprintf(stderr, "\t-v\tincrease verbosity\n");
	fprintf(stderr, "\t-l\twrite a listing (<source>.lst): address, bytes, source, symbols\n");
	fprintf(stderr, "\t-9\t9 character symbol names (default 15)\n");
	fprintf(stderr, "\t-8\t8080 mode (no jp->jr relaxation)\n");
	fprintf(stderr, "\t-n\tno timeout\n");
	exit(1);
}

char *infile;
char *outfile;
char tmpbuf[256];

#define T_BIAS  0x80
#define T_EOF   (T_BIAS + 42)
#define FILEBUFSIZE 512

unsigned char *lineptr = (unsigned char *)"";
/*
 * Long enough for the longest data line the compiler emits: the
 * string pool writes whole pooled literals as one .db, and rewrite.c
 * carries text templates that spell past 800 characters.  256 was
 * the old size, and a longer line was TRUNCATED SILENTLY - the tail
 * of pass2's long-negate template simply left the binary, and the
 * self-hosted compiler printed the next literal in the pool where
 * "ld h,a" should have been.
 */
/*
 * 512, and it was 1024.  It only ever holds a line of CODE now -
 * get_line consumes a comment where it starts rather than copying it
 * in to be cut out later - and the longest the compiler emits is 270
 * characters, a .db of rule text in c1/rules.s.  The longest in any
 * hand-written assembly in the tree is 82.
 *
 * Not smaller than that.  Overflow here is gripe("line too long"),
 * which stops the build, and a .db line is generated from tables that
 * can grow; 512 is most of twice the largest ever seen.
 */
unsigned char linebuf[512];
char l_flag;                    /* -l: write a listing file */
FILE *lstfp;
unsigned char filebuf[FILEBUFSIZE+1];
unsigned char *limit = 0;
unsigned char *inptr = 0;
char saw_eof;  /* set when CP/M EOF (^Z) encountered */

extern char pass;

void gripe();

int
fillbuf()
{
    int i;

    if (saw_eof)
        return 0;

    i = fread(filebuf, 1, FILEBUFSIZE, infp);
    if (i < 0) {
        gripe("io error on read");
    } else if (i == 0) {
        return 0;
    } else {
        inptr = filebuf;
        limit = &filebuf[i];
    }
    if (pass == 0 && infp == stdin) {
        fwrite(filebuf, 1, i, inbuffp);
    }
    return i;
}

/*
 * read a line into a null-terminated C string, ending with a newline.
 * comments and trailing whitespace are stripped.
 *
 * A COMMENT IS CONSUMED, NOT STORED.  It used to be copied into the
 * buffer and then cut out again afterwards, which meant the buffer
 * had to be big enough for the longest COMMENT rather than the
 * longest line of code - and c1 writes the whole AST of an expression
 * into one, so the longest the compiler emits is 2,226 characters
 * against a longest code line of 270.
 *
 * That cost a kilobyte of bss to hold text that was thrown away two
 * loops later, and it needed a special case for the line that did not
 * fit: cut it if it began with a semicolon, gripe otherwise.  Neither
 * is needed now.  The delimiter ends the line, the rest of the input
 * is read and dropped, and what "line too long" means is what it says
 * - a line of code too long to assemble.
 */
static void
skip_comment()
{
    unsigned char c;

    while (1) {
        if (inptr >= limit && fillbuf() == 0)
            return;
        c = *inptr++;
        if (c == 0x1A) {
            saw_eof = 1;
            return;
        }
        if (c == '\n')
            return;
    }
}

void
get_line()
{
    int i;
    unsigned char c;
    unsigned char prev;
    int in_string;

    list_line();		/* the line now ending gets its listing record */

    lineptr = linebuf;
    prev = 0;
    in_string = 0;

    for (i = 0; i < sizeof(linebuf) - 2; i++) {
        if (inptr >= limit) {
            if (fillbuf() == 0) {
                *lineptr++ = T_EOF;
                break;
            }
        }
        c = *inptr++;
        /* CP/M EOF marker - treat rest of file as empty */
        if (c == 0x1A) {
            saw_eof = 1;
            *lineptr++ = T_EOF;
            break;
        }

        /*
         * A quote toggles; a semicolon outside quotes ends the line.
         * The escape test looks at the character before, which is
         * what the second pass over the buffer used to do.
         */
        if (c == '"' && prev != '\\') {
            in_string = !in_string;
        } else if (c == ';' && !in_string) {
            skip_comment();
            *lineptr++ = '\n';
            break;
        }

        *lineptr++ = c;
        prev = c;
        if (c == '\n')
            break;
    }
    if (i == sizeof(linebuf) - 2)
        gripe("line too long");

    *lineptr = '\0';
    lineNum++;

    /* strip trailing whitespace before newline */
    i = strlen((char *)linebuf);
    while (i >= 2 && (linebuf[i - 2] == ' ' || linebuf[i - 2] == '\t' ||
                      linebuf[i - 2] == '\r')) {
        linebuf[i - 2] = '\n';
        linebuf[i - 1] = '\0';
        i--;
    }

    lineptr = linebuf;
    list_take();		/* hand the stripped line to the listing */
}

/*
 * get the next character that we would read, but don't advance
 */
unsigned char
peekchar()
{
    unsigned char c;

    c = *lineptr;
#ifdef DEBUG
    if (verbose > 5)
        printf("peekchar: %d \'%c\'\n", c, (c > ' ') ? c : ' ');
#endif
    return (c);
}

/*
 * returns the next character in the source, or -1 if complete
 */
unsigned char
nextchar()
{
    unsigned char c;

    if (!*lineptr)
        get_line();

    c = *lineptr;
    if (c != T_EOF) {
        lineptr++;
    }

    return c;
}

/*
 * consumes to end of line
 */
void
consume()
{
    *lineptr = '\0';
}

/*
 * reset input state for new pass
 */
void
io_reset()
{
    saw_eof = 0;
    lineptr = (unsigned char *)"";
    inptr = 0;
    limit = 0;
}

/*
 * if looking at whitespace, skip it
 */
unsigned char
skipwhite()
{
    unsigned char c;

    while(1) {
        c = peekchar();
        if ((c == '\t') || (c == ' ')) {
            c = nextchar();
        } else {
            break;
        }
    }
    return c;
}

/*
 * is this an alphabetic, underscore or '@'
 *
 * '@' is here for the compiler, which puts it in front of every label
 * the programmer wrote.  C keeps labels in a namespace of their own,
 * so "out:" is a good label and says nothing about any "out"
 * elsewhere; this assembler has no such namespace, and a line
 * beginning "out" is the instruction with a stray colon after it.
 * Every mnemonic was exposed that way - "out", "end", "in", "set" and
 * "cp" are all names a person reaches for at the bottom of a loop.
 *
 * A character C cannot spell is the whole point, so the prefix cannot
 * be '_': the compiler already puts one in front of every global, and
 * a label "out" and a function "out" would both arrive as "_out".  '@'
 * meant nothing here before this line, so no source that assembled
 * yesterday reads differently today - it only makes something that
 * was an error into a name.
 */
char
alpha(in)
char in;
{
    return (in >= 'A' && in <= 'Z') || (in >= 'a' && in <= 'z')
        || in == '_' || in == '@';
}

/*
 * is this a valid inner symbol character (includes '.')
 */
char
symchar(in)
char in;
{
    return alpha(in) || (in >= '0' && in <= '9') || in == '.';
}

/*
 * converts an escaped char into its value
 *
 * \[bernetv] = c escape for control chars
 * \000 (octal) =
 * \<anything else> = same
 */
char
escape()
{
    char c;
    int i = 0;

    c = nextchar();
    switch (c) {
    case 'b':
        return '\b';
    case 'e':
        return 0x1B;
    case 'r':
        return '\r';
    case 'n':
        return '\n';
    case 't':
        return '\t';
    case 'v':
        return 0x0B;
    case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7':
        i = c - '0';
        while (1) {
            c = peekchar();
            if (c > '7' || c < '0') break;
            c = nextchar();
            i = (i << 3) + c - '0';
        }
        return i;
    default:
        return c;
    }
}

/*
 * checks if a string is equal
 *
 * It used to fold a to lower case first - the comment said "string a
 * is read as lower case" - except that it compared *a and not the
 * folded character, so the fold was computed for every character of
 * every comparison and then thrown away.  Two tests and an add per
 * character, and no case-insensitivity to show for them; this is the
 * same function with the dead half removed.  If a case-blind compare
 * is ever wanted it has to be written deliberately, because nothing
 * here has been getting one.
 */
char
match(a, b)
char *a;
char *b;
{
    while (*b) {
        if (*a != *b)
            return 0;
        a++;
        b++;
    }

    return *a == *b;
}

/*
 * parse number - handles quite a few formats:
 * decimal: 20, 78
 * hex:  0x0, 0X00, 000H, 00h
 * octal: 06, 003, 05o, 06O
 * binary: 0b0001010 0B0001010 01010B (lowercase 'b' suffix reserved for local labels)
 */
unsigned long
parsenum(s)
char *s;
{
    int i = strlen(s);
    unsigned long val = 0;
    int base = 10;
    char c;

    /* Check 0x/0b/0B prefix FIRST (before trailing radix check) */
    if (*s == '0' && s[1]) {
        c = s[1] | 0x20;
        if (c == 'x') {
            base = 16;
            s += 2;
        } else if (c == 'b') {
            base = 2;
            s += 2;
        } else {
            /* Leading 0 but not 0x/0b - check for trailing radix or octal */
            c = s[i-1];
            if (c == 'h' || c == 'H') {
                base = 16;
                s[i-1] = '\0';
            } else if (c == 'o' || c == 'O') {
                base = 8;
                s[i-1] = '\0';
            } else if (c == 'B') {
                /* uppercase B only - lowercase b reserved for local labels */
                base = 2;
                s[i-1] = '\0';
            } else {
                base = 8;
                s++;
            }
        }
    } else {
        /* No leading 0 - check for trailing radix marker */
        c = s[i-1];
        if (c == 'h' || c == 'H') {
            base = 16;
            s[i-1] = '\0';
        } else if (c == 'o' || c == 'O') {
            base = 8;
            s[i-1] = '\0';
        } else if (c == 'B') {
            /* uppercase B only - lowercase b reserved for local labels */
            base = 2;
            s[i-1] = '\0';
        }
    }

    while (*s) {
        int d;
        val *= base;
        c = *s | 0x20;
        d = c - '0';
        if ((base == 16) && (d > 9))
            d -= ('a' - '0') - 10;
        if ((d >= base) || (d < 0)) {
            gripe("numeric digit out of range");
        }
        val += d;
        s++;
    }
    return val;
}


/*
 * prints out an error message and exits
 */
void
gripe(msg)
char *msg;
{
	printf("%s:%d %s\n%s",
        infile, lineNum, msg, linebuf);
	exit(1);
}

void
gripe2(msg, arg)
char *msg;
char *arg;
{
	printf("%s:%d %s%s\n%s",
        infile, lineNum, msg, arg, linebuf);
	exit(1);
}

int
main(argc, argv)
int argc;
char **argv;
{
    char *s;
    int i;

    progname = *argv;
    argv++;
    argc--;

	while (argc) {
        s = *argv;

		if (*s++ != '-') {
            /* non-flag argument is input file */
            infile = *argv++;
            argc--;
            continue;
        }
        argv++;
        argc--;

	    while (*s) {
            switch (*s++) {

            case 'n':
                n_flag++;
                break;

            case '8':
                no_relax++;
                break;

			case '9':
			case 'm':
				m_flag++;
				break;

#ifdef DEBUG
			case 'v':
				verbose++;
				break;
#endif

            case 'o':
                outfile = *argv++;
                argc--;
                break;

            case 'l':
                l_flag++;
                break;

			default:
				usage();
			}
		}
	}

#ifdef DEBUG
    if (verbose) {
        printf("verbose: %d\n", verbose);
    }
#endif

    if (infile) {
        infp = fopen(infile, "rb");
		if (infp == NULL) {
            printf("cannot open source file %s\n", infile);
            exit(1);
        }
        if (!outfile) {
            outfile = malloc(strlen(infile)+2);
            strcpy(outfile, infile);
            s = strrchr(outfile, '.');
            if (!s) {
                s = &outfile[strlen(outfile)];
            }
            strcpy(s, ".o");
        }
        if (l_flag) {
            char *lst = malloc(strlen(infile) + 5);
            strcpy(lst, infile);
            s = strrchr(lst, '.');
            if (!s)
                s = &lst[strlen(lst)];
            strcpy(s, ".lst");
            lstfp = fopen(lst, "w");
            if (lstfp == NULL) {
                printf("cannot open listing file %s\n", lst);
                exit(1);
            }
        }
    } else {
        /* no filename specified - use stdin */
        infp = stdin;
        if (!outfile)
            outfile = "a.out";
        sprintf(tmpbuf, "/tmp/atmi%d", getpid());
        if ((inbuffp = fopen(tmpbuf, "w+b")) == NULL) {
            printf("cannot open tmp input buffer file %s\n", tmpbuf);
            exit(1);
        }
        unlink(tmpbuf);
    }


#ifdef linux
    if (infp != stdin && !n_flag) {
	    /* Set up timeout handler to catch infinite loops */
	    signal(SIGALRM, timeoutHdlr);
	    alarm(5);  /* 5 second timeout */
	}
#endif

	sprintf(tmpbuf, "/tmp/atm%d", getpid());
    if ((tmpfp = fopen(tmpbuf, "w+b")) == NULL) {
        printf("cannot open tmp file %s\n", tmpbuf);
        exit(1);
    }
    unlink(tmpbuf);

    sprintf(tmpbuf, "/tmp/arl%d", getpid());
    if ((relfp = fopen(tmpbuf, "w+b")) == NULL) {
        printf("cannot open tmp file %s\n", tmpbuf);
        exit(1);
    }
    unlink(tmpbuf);

    if ((outfp = fopen(outfile, "wb")) == NULL) {
        printf("cannot create output file %s\n", outfile);
        exit(1);
    }

    assemble();

    fseek(tmpfp, 0L, SEEK_SET);
    do {
        i = fread(tmpbuf, 1, sizeof(tmpbuf), tmpfp);
        if (i < 0) {
            perror("cannot read tmp file");
            exit(1);
        }
        if (i > 0 && fwrite(tmpbuf, 1, i, outfp) != i) {
            perror("cannot write object file");
            exit(1);
        }
    } while (i == sizeof(tmpbuf));
    fclose(outfp);
    if (infp != stdin)
        fclose(infp);
    fclose(tmpfp);
    fclose(relfp);
    return 0;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
