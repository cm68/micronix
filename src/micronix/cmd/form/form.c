/*
 * form - format text
 *
 * micronix/cmd/form/form.c
 *
 * Reconstructed from /bin/form, which we have no source for.  See
 * NOTES for how the binary was read and which facts below are traced
 * out of it rather than taken from the manual page (man1/form.1).
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <stdio.h>

#define MAXLINE     512     /* an input line */
#define MAXOUT      256     /* a built output line */
#define MAXWORD     128
#define MAXTEXT     128     /* .he and .fo text */

/*
 * The defaults are the ones form.1 states: fill on, no indent, right
 * margin 60, page length 66.  Fill lives at 0x4000 in the binary and
 * .fi/.nf are the whole of its handling.
 */
int fill = 1;               /* .fi / .nf */
int indent = 0;             /* .in */
int tmpindent = 0;          /* .ti, one line only */
int tmpset = 0;
int rmargin = 60;           /* .rm */
int pagelen = 66;           /* .pl */
int spacing = 1;            /* .ls */
int tabstop = 8;            /* .ta */

int boldcnt = 0;            /* .bd n */
int ulcnt = 0;              /* .ul n */
int cecnt = 0;              /* .ce n */

char header[MAXTEXT];       /* .he */
char footer[MAXTEXT];       /* .fo */

int pageno = 1;
int line = 0;               /* lines already put on this page */
int onpage = 0;             /* have we written the header yet */

int ttyout = 0;             /* -t: no long runs of blank lines */
int pending = 0;            /* blank lines held back under -t */

FILE *out;

char obuf[MAXOUT];          /* the line being filled */
int ocol = 0;               /* text columns used in obuf */
int owords = 0;

char *progname = "form";

char *skipsp();

/*
 * The request table.  The binary looks its requests up in a table of
 * {name, code} pairs and switches on the code; the names are exactly
 * these sixteen.  Order here is the binary's own.
 */
struct req {
    char *name;
    int code;
};

#define R_BP 1
#define R_BR 2
#define R_CE 3
#define R_FI 4
#define R_FO 5
#define R_HE 6
#define R_IN 7
#define R_LS 8
#define R_NF 9
#define R_PL 10
#define R_RM 11
#define R_SP 12
#define R_TI 13
#define R_UL 14
#define R_BD 15
#define R_TA 16

struct req reqs[] = {
    "bp", R_BP,
    "fi", R_FI,
    "nf", R_NF,
    "br", R_BR,
    "ce", R_CE,
    "fo", R_FO,
    "he", R_HE,
    "in", R_IN,
    "ls", R_LS,
    "pl", R_PL,
    "rm", R_RM,
    "sp", R_SP,
    "ti", R_TI,
    "ul", R_UL,
    "bd", R_BD,
    "ta", R_TA,
    0, 0
};

/*
 * Emit one physical line, doing the page furniture around it.  A page
 * is: header, body, footer - and '#' anywhere in either is replaced by
 * the page number, which is what every page of the manual relies on
 * for its "-#-".
 */
puttext(s)
char *s;
{
    if (!onpage)
        topofpage();
    fputs(s, out);
    putc('\n', out);
    line++;
    if (pagelen && line >= pagelen - 6)
        endpage();
}

/*
 * Header and footer text with # standing for the page number.
 */
putfurniture(s)
char *s;
{
    while (*s) {
        if (*s == '#')
            fprintf(out, "%d", pageno);
        else
            putc(*s, out);
        s++;
    }
    putc('\n', out);
}

topofpage()
{
    int i;

    onpage = 1;
    line = 0;
    for (i = 0; i < 3; i++) {
        putc('\n', out);
        line++;
    }
    if (header[0]) {
        putfurniture(header);
        line++;
    }
    putc('\n', out);
    line++;
}

endpage()
{
    if (!onpage)
        return;
    while (line < pagelen - 3) {
        putc('\n', out);
        line++;
    }
    if (footer[0])
        putfurniture(footer);
    while (line < pagelen) {
        putc('\n', out);
        line++;
    }
    onpage = 0;
    line = 0;
    pageno++;
    pending = 0;
}

/*
 * A blank line.  Under -t a run of them is collapsed, which is all the
 * flag does: "the output is made more suitable for a glass tty, i.e.
 * long sequences of blank lines are avoided."
 */
putblank()
{
    if (ttyout) {
        if (pending)
            return;
        pending = 1;
    }
    puttext("");
}

/*
 * Overstrike.  Bold is the character struck twice, underline is the
 * character struck over an underscore - form.1 says the printer must
 * have double-strike capability for either.
 */
emit(s)
char *s;
{
    char buf[MAXOUT * 3];
    char *p, *q;

    if (!boldcnt && !ulcnt) {
        puttext(s);
        return;
    }
    q = buf;
    for (p = s; *p; p++) {
        if (*p == ' ') {
            *q++ = *p;
            continue;
        }
        if (ulcnt) {
            *q++ = '_';
            *q++ = '\b';
            *q++ = *p;
        } else {
            *q++ = *p;
            *q++ = '\b';
            *q++ = *p;
        }
    }
    *q = '\0';
    puttext(buf);
    if (boldcnt)
        boldcnt--;
    if (ulcnt)
        ulcnt--;
}

/*
 * Work out where a line starts: the indent, or the temporary indent if
 * .ti set one, which lasts exactly one line.  A negative .ti is how the
 * manual's hanging paragraphs are made, so the result can be clamped at
 * zero but not folded into the indent.
 */
startcol()
{
    int n;

    n = tmpset ? indent + tmpindent : indent;
    if (n < 0)
        n = 0;
    return n;
}

/*
 * Push out whatever has been filled so far.  This is .br, and every
 * request that changes the shape of the page calls it first.
 */
brk()
{
    char buf[MAXOUT * 2];
    char *q;
    int i, n;

    if (ocol == 0) {
        tmpset = 0;
        return;
    }
    q = buf;
    n = startcol();
    for (i = 0; i < n; i++)
        *q++ = ' ';
    for (i = 0; i < ocol; i++)
        *q++ = obuf[i];
    *q = '\0';
    emit(buf);
    ocol = 0;
    owords = 0;
    tmpset = 0;
    for (i = 1; i < spacing; i++)
        putblank();
}

/*
 * Centre one line between the indent and the right margin.
 */
centre(s)
char *s;
{
    char buf[MAXOUT * 2];
    char *q;
    int i, n, len;

    for (len = 0; s[len]; len++)
        ;
    n = startcol() + (rmargin - startcol() - len) / 2;
    if (n < 0)
        n = 0;
    q = buf;
    for (i = 0; i < n; i++)
        *q++ = ' ';
    for (i = 0; s[i]; i++)
        *q++ = s[i];
    *q = '\0';
    emit(buf);
    tmpset = 0;
    if (cecnt)
        cecnt--;
}

/*
 * Add one word to the line being filled, breaking first if it will not
 * fit inside the right margin.
 */
addword(w)
char *w;
{
    int len, i;

    for (len = 0; w[len]; len++)
        ;
    if (len == 0)
        return;
    if (ocol && startcol() + ocol + 1 + len > rmargin)
        brk();
    if (ocol) {
        obuf[ocol++] = ' ';
        /* two spaces after a sentence, as the sources are typed */
        if (ocol > 1 && (obuf[ocol - 2] == '.' || obuf[ocol - 2] == '?' ||
                         obuf[ocol - 2] == '!'))
            obuf[ocol++] = ' ';
    }
    for (i = 0; i < len && ocol < MAXOUT - 2; i++)
        obuf[ocol++] = w[i];
    owords++;
}

/*
 * A text line.  In fill mode its words join whatever is already in the
 * buffer; in no-fill mode it goes out as it stands.  A blank line is a
 * break followed by a blank, which is how the sources separate their
 * paragraphs - they use one far more often than .sp.
 */
dotext(s)
char *s;
{
    char word[MAXWORD];
    char buf[MAXOUT * 2];
    char *p, *q;
    int i, n;

    if (cecnt) {
        centre(s);
        return;
    }
    for (p = s; *p == ' ' || *p == '\t'; p++)
        ;
    if (*p == '\0') {
        brk();
        putblank();
        return;
    }
    if (!fill) {
        q = buf;
        n = startcol();
        for (i = 0; i < n; i++)
            *q++ = ' ';
        for (i = 0; s[i]; i++)
            *q++ = s[i];
        *q = '\0';
        emit(buf);
        tmpset = 0;
        for (i = 1; i < spacing; i++)
            putblank();
        return;
    }
    p = s;
    while (*p) {
        while (*p == ' ' || *p == '\t')
            p++;
        if (!*p)
            break;
        q = word;
        while (*p && *p != ' ' && *p != '\t' && q < word + MAXWORD - 1)
            *q++ = *p++;
        *q = '\0';
        addword(word);
    }
}

/*
 * The argument of a request: a possibly signed number, or absent.  Each
 * request states its own default for the absent case.
 */
int hasarg;

int
getarg(s, dflt)
char *s;
int dflt;
{
    int n, sign;

    while (*s == ' ' || *s == '\t')
        s++;
    hasarg = 0;
    if (*s == '\0' || *s == '\n')
        return dflt;
    sign = 1;
    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }
    if (*s < '0' || *s > '9')
        return dflt;
    hasarg = 1;
    n = 0;
    while (*s >= '0' && *s <= '9')
        n = n * 10 + (*s++ - '0');
    return n * sign;
}

/*
 * The text of .he and .fo, which the pages always write in quotes.
 */
gettext(s, d)
char *s;
char *d;
{
    char *e;

    while (*s == ' ' || *s == '\t')
        s++;
    if (*s == '"') {
        s++;
        e = d + MAXTEXT - 1;
        while (*s && *s != '"' && d < e)
            *d++ = *s++;
    } else {
        e = d + MAXTEXT - 1;
        while (*s && *s != '\n' && d < e)
            *d++ = *s++;
    }
    *d = '\0';
}

docommand(s)
char *s;
{
    struct req *r;
    char name[3];
    char *a;
    int n;

    name[0] = s[1];
    name[1] = s[1] ? s[2] : '\0';
    name[2] = '\0';
    a = s + 1;
    while (*a && *a != ' ' && *a != '\t')
        a++;

    for (r = reqs; r->name; r++) {
        if (r->name[0] == name[0] && r->name[1] == name[1])
            break;
    }
    if (!r->name)
        return;                 /* unknown request: the line vanishes */

    switch (r->code) {

    case R_BR:
        brk();
        break;

    case R_FI:
        brk();
        fill = 1;
        break;

    case R_NF:
        brk();
        fill = 0;
        break;

    case R_IN:
        brk();
        n = getarg(a, 0);
        if (!hasarg)
            indent = 0;
        else if (*skipsp(a) == '+' || *skipsp(a) == '-')
            indent += n;
        else
            indent = n;
        if (indent < 0)
            indent = 0;
        break;

    case R_TI:
        n = getarg(a, 0);
        tmpindent = n;
        tmpset = 1;
        break;

    case R_RM:
        brk();
        rmargin = getarg(a, 60);
        break;

    case R_PL:
        n = getarg(a, 66);
        pagelen = n;
        break;

    case R_LS:
        spacing = getarg(a, 1);
        if (spacing < 1)
            spacing = 1;
        break;

    case R_TA:
        tabstop = getarg(a, 8);
        if (tabstop < 1)
            tabstop = 8;
        break;

    case R_SP:
        brk();
        n = getarg(a, 1);
        while (n-- > 0)
            putblank();
        break;

    case R_BP:
        brk();
        n = getarg(a, 0);
        endpage();
        if (hasarg)
            pageno = n;
        break;

    case R_CE:
        brk();
        cecnt = getarg(a, 1);
        break;

    case R_BD:
        boldcnt = getarg(a, 1);
        break;

    case R_UL:
        ulcnt = getarg(a, 1);
        break;

    case R_HE:
        gettext(a, header);
        break;

    case R_FO:
        gettext(a, footer);
        break;
    }
}

char *
skipsp(s)
char *s;
{
    while (*s == ' ' || *s == '\t')
        s++;
    return s;
}

/*
 * The main loop is the binary's: read a line, and if its first
 * character is a dot treat it as a request, otherwise as text.
 */
format(f)
FILE *f;
{
    char buf[MAXLINE];
    char *p;

    while (fgets(buf, sizeof(buf), f)) {
        for (p = buf; *p; p++) {
            if (*p == '\n') {
                *p = '\0';
                break;
            }
        }
        if (buf[0] == '.')
            docommand(buf);
        else
            dotext(buf);
    }
}

main(argc, argv)
int argc;
char **argv;
{
    int i;
    int nfiles = 0;
    FILE *f;

    out = stdout;
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 't' && argv[i][2] == '\0') {
            ttyout = 1;
        } else if (argv[i][0] == '-' && argv[i][1] == 'o' && argv[i][2] == '\0') {
            if (++i >= argc) {
                fprintf(stderr, "%s: -o needs a file\n", progname);
                exit(1);
            }
            if ((out = fopen(argv[i], "w")) == NULL) {
                perror(argv[i]);
                exit(1);
            }
        } else {
            nfiles++;
        }
    }

    if (nfiles == 0) {
        format(stdin);
    } else {
        for (i = 1; i < argc; i++) {
            if (argv[i][0] == '-' && argv[i][1] == 't' && argv[i][2] == '\0')
                continue;
            if (argv[i][0] == '-' && argv[i][1] == 'o' && argv[i][2] == '\0') {
                i++;
                continue;
            }
            if ((f = fopen(argv[i], "r")) == NULL) {
                perror(argv[i]);
                continue;
            }
            format(f);
            fclose(f);
        }
    }
    brk();
    endpage();
    fflush(out);
    exit(0);
}
