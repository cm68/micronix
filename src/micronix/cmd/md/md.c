/*
 * md - read markdown, write formatted text
 *
 * micronix/cmd/md/md.c
 *
 * A reader for the docs/ tree.  It reads CommonMark-flavoured markdown
 * from files (or stdin) and writes a plain-text rendering to stdout, so
 * "md docs/DISKLABEL.md | less" reads like a man page.
 *
 * Emphasis is overstrike, the house style (see form(1)): bold is the
 * character struck twice, underline is the character struck over an
 * underscore.  less(1) renders both.  Inline code spans (backticks) are
 * written plain, the backticks gone.
 *
 * Blocks handled: ATX and setext headings, horizontal rules, paragraphs
 * (filled to the right margin), fenced code blocks, blockquotes, nested
 * lists, and pipe tables with alignment.  Links are not used anywhere in
 * the tree's documents and are left out.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <stdio.h>
#include <string.h>
#ifdef linux
#include <stdlib.h>
#endif

#define MAXLINE  512     /* an input line */
#define MAXOUT   256     /* a built output line (plain text, before overstrike) */
#define MAXWORD  256     /* one word plus its per-character style */
#define MAXPARA  4096    /* a paragraph being accumulated */
#define MARGIN   72      /* default right margin */
#define MAXCOLS  16      /* table columns */
#define MAXCELL  96      /* one table cell */
#define MAXROWS  128     /* table data rows */
#define MAXTBL   4096    /* raw table data rows, flat and NUL-separated */

FILE *out;              /* stdout */
FILE *inf;              /* the file being read */

int rmargin = MARGIN;   /* -w N */

char *progname = "md";

/*
 * The line reader alternates two buffers and holds one line of pushback.
 * Two buffers, because a caller holds one line while reading the next:
 * a setext heading and a table separator both need the line that follows.
 */
char line[2][MAXLINE];
char pbuf[MAXLINE];
int havepeek;
int flip;

/*
 * Paragraph accumulation.  Lines are joined with single spaces; the fill
 * runs once, when the paragraph ends.
 */
char parabuf[MAXPARA];
int paralene;
int inpara;

/*
 * Inline emphasis state.  It persists across word boundaries inside one
 * paragraph, so "**bold text**" fills as a single styled run.
 */
int inbold, inul, incode;
int forcesty;           /* 0 none, 1 bold, 2 underline: overrides, for headings */

/*
 * One scanned word (or line): the plain characters and, beside each, the
 * style that overstrike will apply when it is written.
 */
char wbuf[MAXWORD];
char wsty[MAXWORD];
int wlen;

/*
 * The line being filled.  Plain text, so a column and a byte are the same;
 * overstrike is applied only when the line is flushed.
 */
char obuf[MAXOUT];
char osty[MAXOUT];
int ocol;

/*
 * List marker state, set by listmark().
 */
char marker[16];
int mlen;
int mindent;

/*
 * Table state.
 */
char cell[MAXCOLS][MAXCELL];
char hcell[MAXCOLS][MAXCELL];
int ncols;
char align[MAXCOLS];
int widths[MAXCOLS];
char tbl[MAXTBL];
int rowstart[MAXROWS];
int nrows;
int tblused;

/* htext: where the heading text starts, set by atx(). */
char *htext;

char *rdline();
unsigned char *scan();
char *listmark();
char *unquote();
dolist();
item();

/*
 * The last byte written to out.  putblank() uses it to collapse a run of
 * blank lines into one, and to avoid a leading blank before the first line.
 */
int lastc;

outc(c)
int c;
{
    c &= 0xff;              /* pass UTF-8 and other high bytes through whole */
    putc(c, out);
    lastc = c;
}

putblank()
{
    if (lastc != 0 && lastc != '\n')
        outc('\n');
}

/*
 * Emit one character with a style.  Bold is the character struck twice,
 * underline is the character struck over an underscore.
 */
emitchar(c, st)
int c, st;
{
    if (st == 1) {
        outc(c); outc('\b'); outc(c);
    } else if (st == 2) {
        outc('_'); outc('\b'); outc(c);
    } else
        outc(c);
}

/*
 * The style of the character being scanned: forced, code (plain), bold,
 * underline, or plain.
 */
cursty()
{
    if (forcesty)
        return forcesty;
    if (incode)
        return 0;
    if (inbold)
        return 1;
    if (inul)
        return 2;
    return 0;
}

addw(c, st)
int c, st;
{
    if (wlen < MAXWORD - 1) {
        wbuf[wlen] = c;
        wsty[wlen] = st;
        wlen++;
    }
}

/*
 * The inline scanner.  Walk from *p, resolving ** _ * and backticks to a
 * run of styled characters in wbuf.  When stopws is set, whitespace ends
 * the scan (a word); it does not end the scan inside a code span, so a
 * code span reads as one word.  Returns the first unconsumed character.
 */
unsigned char *
scan(p, stopws)
unsigned char *p;
int stopws;
{
    int c;

    wlen = 0;
    for (;;) {
        c = *p;
        if (c == '\0')
            break;
        if (stopws && (c == ' ' || c == '\t') && !incode)
            break;
        if (c == '\\' && p[1] != '\0') {
            addw(p[1], cursty());
            p += 2;
            continue;
        }
        if (c == '`') {
            incode = !incode;
            p++;
            continue;
        }
        if (c == '*') {
            if (p[1] == '*') {
                inbold = !inbold;
                p += 2;
                continue;
            }
            inul = !inul;
            p++;
            continue;
        }
        if (c == '_') {
            if (p[1] == '_') {
                inbold = !inbold;
                p += 2;
                continue;
            }
            inul = !inul;
            p++;
            continue;
        }
        addw(c, cursty());
        p++;
    }
    return p;
}

emitw()
{
    int i;

    for (i = 0; i < wlen; i++)
        emitchar(wbuf[i], wsty[i]);
}

/*
 * Render a whole string with inline markup resolved, no filling.  The
 * caller sets forcesty first for a forced style (headings, table heads).
 */
renderinline(s)
unsigned char *s;
{
    inbold = inul = incode = 0;
    scan(s, 0);
    emitw();
}

/*
 * The display width of a string after markup is resolved: what the scan
 * produced, counted as characters.
 */
cellwidth(s)
unsigned char *s;
{
    inbold = inul = incode = 0;
    scan(s, 0);
    return wlen;
}

/*
 * Fill a paragraph to the right margin.  base is the left margin for every
 * line: the first line's text starts there (the caller has written a list
 * marker into that space) and later lines are indented to it.
 */
fill(s, base)
unsigned char *s;
int base;
{
    unsigned char *p = s;
    int avail;
    int nlines = 0;
    int i;

    avail = rmargin - base;
    if (avail < 8)
        avail = 8;
    ocol = 0;
    inbold = inul = incode = 0;
    forcesty = 0;
    for (;;) {
        while (*p == ' ' || *p == '\t')
            p++;
        if (*p == '\0')
            break;
        p = scan(p, 1);
        if (wlen == 0)
            continue;
        if (ocol > 0 && ocol + 1 + wlen > avail)
            flushline(base, &nlines);
        if (ocol > 0 && ocol < MAXOUT - 1) {
            obuf[ocol] = ' ';
            osty[ocol] = 0;
            ocol++;
        }
        for (i = 0; i < wlen && ocol < MAXOUT - 1; i++) {
            obuf[ocol] = wbuf[i];
            osty[ocol] = wsty[i];
            ocol++;
        }
    }
    flushline(base, &nlines);
}

flushline(base, pn)
int base;
int *pn;
{
    int i;

    if (ocol == 0)
        return;
    if (*pn > 0)
        for (i = 0; i < base; i++)
            outc(' ');
    for (i = 0; i < ocol; i++)
        emitchar(obuf[i], osty[i]);
    outc('\n');
    ocol = 0;
    (*pn)++;
}

/*
 * Paragraph accumulation: skip leading spaces, join lines with one space.
 */
addpara(s)
char *s;
{
    char *p = s;
    int l;

    while (*p == ' ' || *p == '\t')
        p++;
    l = strlen(p);
    if (inpara && paralene + 1 + l >= MAXPARA - 1)
        endpara();
    if (inpara && paralene > 0)
        parabuf[paralene++] = ' ';
    strcpy(parabuf + paralene, p);
    paralene += l;
    inpara = 1;
}

endpara()
{
    if (!inpara)
        return;
    parabuf[paralene] = '\0';
    fill(parabuf, 0);
    paralene = 0;
    inpara = 0;
}

/*
 * The line reader, with one line of pushback.
 */
char *
rdline()
{
    char *r;

    if (havepeek) {
        havepeek = 0;
        return pbuf;
    }
    flip = !flip;
    r = fgets(line[flip], MAXLINE, inf);
    if (r == 0)
        return 0;
    stripnl(line[flip]);
    return line[flip];
}

stripnl(s)
char *s;
{
    char *p = s;

    while (*p)
        p++;
    while (p > s && (p[-1] == '\n' || p[-1] == '\r'))
        *--p = '\0';
}

pushback(s)
char *s;
{
    strcpy(pbuf, s);
    havepeek = 1;
}

isblankline(s)
char *s;
{
    char *p = s;

    while (*p == ' ' || *p == '\t')
        p++;
    return *p == '\0';
}

/*
 * Block predicates.  All are read-only on the line, so a line may be
 * classified and pushed back unchanged.
 */

/* ATX heading level, 1-6, else 0; leaves htext pointing at the text. */
atx(s)
char *s;
{
    char *p = s;
    int lvl = 0;

    while (*p == '#') {
        p++;
        lvl++;
    }
    if (lvl == 0 || lvl > 6)
        return 0;
    if (*p != ' ' && *p != '\t' && *p != '\0')
        return 0;
    while (*p == ' ' || *p == '\t')
        p++;
    htext = p;
    return lvl;
}

/* Setext underline: 1 for ===, 2 for ---, else 0. */
setext(s)
char *s;
{
    char *p = s;
    char c;

    while (*p == ' ' || *p == '\t')
        p++;
    if (*p == '\0')
        return 0;
    c = *p;
    if (c != '=' && c != '-')
        return 0;
    while (*p == c)
        p++;
    while (*p == ' ' || *p == '\t')
        p++;
    if (*p != '\0')
        return 0;
    return (c == '=') ? 1 : 2;
}

ishr(s)
char *s;
{
    char *p = s;
    char c;
    int n = 0;

    while (*p == ' ' || *p == '\t')
        p++;
    c = *p;
    if (c != '-' && c != '*' && c != '_')
        return 0;
    while (*p) {
        if (*p == ' ' || *p == '\t') {
            p++;
            continue;
        }
        if (*p != c)
            return 0;
        n++;
        p++;
    }
    return n >= 3;
}

isfence(s)
char *s;
{
    char *p = s;

    while (*p == ' ' || *p == '\t')
        p++;
    return (p[0] == '`' && p[1] == '`' && p[2] == '`');
}

istablesep(s)
char *s;
{
    char *p = s;
    int pipe = 0;

    while (*p == ' ' || *p == '\t')
        p++;
    while (*p) {
        if (*p == '|')
            pipe = 1;
        else if (*p != '-' && *p != ':' && *p != ' ' && *p != '\t')
            return 0;
        p++;
    }
    return pipe;
}

haspipe(s)
char *s;
{
    return strchr(s, '|') != 0;
}

isquote(s)
char *s;
{
    return s[0] == '>';
}

char *
unquote(s)
char *s;
{
    char *p = s + 1;

    if (*p == ' ')
        p++;
    return p;
}

isblockstart(s)
char *s;
{
    return isfence(s) || atx(s) > 0 || ishr(s) || isquote(s);
}

/*
 * List items.  listmark() records the marker and returns the item text.
 */
char *
listmark(s)
char *s;
{
    char *p = s;
    int i;

    mindent = 0;
    while (*p == ' ' || *p == '\t') {
        p++;
        mindent++;
    }
    if (*p == '-' || *p == '*' || *p == '+') {
        if (p[1] != ' ' && p[1] != '\t' && p[1] != '\0')
            return 0;
        marker[0] = *p;
        marker[1] = ' ';
        marker[2] = '\0';
        mlen = 2;
        if (p[1] == '\0')
            return p + 1;
        return p + 2;
    }
    if (*p >= '0' && *p <= '9') {
        i = 0;
        while (*p >= '0' && *p <= '9') {
            marker[i++] = *p++;
        }
        if (*p != '.')
            return 0;
        marker[i++] = *p++;
        if (*p == ' ' || *p == '\t') {
            marker[i++] = ' ';
            p++;
        }
        marker[i] = '\0';
        mlen = i;
        return p;
    }
    return 0;
}

leadsp(s)
char *s;
{
    int n = 0;

    while (*s == ' ' || *s == '\t') {
        s++;
        n++;
    }
    return n;
}

emitmarker(mk)
char *mk;
{
    while (*mk)
        outc(*mk++);
}

/*
 * One list item.  The marker is written, then the item text and its
 * continuation lines are filled with a hanging indent.  A line that is a
 * deeper list item is a nested list, rendered recursively.
 */
item(s)
char *s;
{
    int indent = leadsp(s);
    char *m = listmark(s);
    int textcol = mindent + mlen;
    int ind = mindent;
    char mk[16];
    char *t;
    int ti;
    int i;

    /*
     * listmark() writes the marker and indent into globals, and the loop
     * below calls it again on continuation lines.  Save them now, before
     * they are clobbered, so the marker written here is the right one.
     */
    strcpy(mk, marker);

    /* the marker goes out now; the body fills in after it on this line */
    for (i = 0; i < ind; i++)
        outc(' ');
    emitmarker(mk);

    /* body starts with the item's own text */
    paralene = 0;
    inpara = 0;
    addpara(m);

    for (;;) {
        t = rdline();
        if (t == 0)
            break;
        if (isblankline(t)) {
            pushback(t);
            break;
        }
        ti = leadsp(t);
        if (listmark(t) != 0) {
            if (ti > indent) {
                /* a nested list: flush the body, then recurse */
                if (paralene > 0) {
                    parabuf[paralene] = '\0';
                    fill(parabuf, textcol);
                    paralene = 0;
                    inpara = 0;
                }
                dolist(t);
                break;
            }
            pushback(t);
            break;
        }
        if (isblockstart(t)) {
            pushback(t);
            break;
        }
        addpara(t);
    }
    if (paralene > 0) {
        parabuf[paralene] = '\0';
        fill(parabuf, textcol);
        paralene = 0;
        inpara = 0;
    }
    /* the marker line must end, even when the body was empty */
    if (lastc != '\n')
        outc('\n');
}

/*
 * A list: a run of items at one indent.
 */
dolist(s)
char *s;
{
    int indent = leadsp(s);

    for (;;) {
        item(s);
        s = rdline();
        if (s == 0)
            return;
        if (isblankline(s)) {
            putblank();
            return;
        }
        if (listmark(s) == 0 || leadsp(s) != indent) {
            pushback(s);
            return;
        }
    }
}

/*
 * Headings and rules.
 */
heading(lvl, s)
int lvl;
char *s;
{
    char tmp[MAXLINE];
    char *q, *e;
    int i;

    q = tmp;
    while (*s)
        *q++ = *s++;
    *q = '\0';
    e = q;
    while (e > tmp && (e[-1] == ' ' || e[-1] == '\t'))
        *--e = '\0';
    while (e > tmp && e[-1] == '#')
        *--e = '\0';

    putblank();
    forcesty = 1;
    renderinline(tmp);
    forcesty = 0;
    outc('\n');
    if (lvl == 1 || lvl == 2) {
        int c = (lvl == 1) ? '=' : '-';
        for (i = 0; i < rmargin; i++)
            outc(c);
        outc('\n');
    }
    putblank();
}

dhr()
{
    int i;

    putblank();
    for (i = 0; i < rmargin; i++)
        outc('-');
    outc('\n');
    putblank();
}

/*
 * A fenced code block: everything up to the next fence, verbatim.
 */
fenced()
{
    char *s;

    putblank();
    for (;;) {
        s = rdline();
        if (s == 0)
            break;
        if (isfence(s))
            break;
        emitverb(s);
    }
    putblank();
}

emitverb(s)
char *s;
{
    outc(' ');
    outc(' ');
    while (*s)
        outc(*s++);
    outc('\n');
}

doquote(s)
char *s;
{
    for (;;) {
        outc(' ');
        outc(' ');
        outc('>');
        outc(' ');
        forcesty = 0;
        renderinline(unquote(s));
        outc('\n');
        s = rdline();
        if (s == 0 || !isquote(s))
            break;
    }
    if (s != 0)
        pushback(s);
    putblank();
}

/*
 * Tables.
 */

trim(s)
char *s;
{
    char *q = s, *e;

    while (*q == ' ' || *q == '\t')
        q++;
    e = s;
    while (*q)
        *e++ = *q++;
    *e = '\0';
    while (e > s && (e[-1] == ' ' || e[-1] == '\t'))
        *--e = '\0';
}

/*
 * Split a table row into cells, honouring a backtick span that contains a
 * pipe.  Fills cell[] and sets ncols.
 */
splitcells(s)
char *s;
{
    char *p = s;
    int code = 0;
    int ci = 0, cl;

    if (*p == '|')
        p++;
    while (*p == ' ' || *p == '\t')
        p++;
    for (;;) {
        cl = 0;
        while (*p && !(*p == '|' && !code)) {
            if (*p == '`')
                code = !code;
            if (cl < MAXCELL - 1)
                cell[ci][cl++] = *p;
            p++;
        }
        cell[ci][cl] = '\0';
        trim(cell[ci]);
        ci++;
        if (ci >= MAXCOLS || *p == '\0')
            break;
        p++;
        while (*p == ' ' || *p == '\t')
            p++;
    }
    if (ci > 0 && cell[ci - 1][0] == '\0')
        ci--;
    ncols = ci;
    return ncols;
}

parsealign(sep)
char *sep;
{
    int i, l;
    int hasl, hasr;

    splitcells(sep);
    for (i = 0; i < ncols; i++) {
        l = strlen(cell[i]);
        hasl = (l > 0 && cell[i][0] == ':');
        hasr = (l > 0 && cell[i][l - 1] == ':');
        if (hasl && hasr)
            align[i] = 'c';
        else if (hasr)
            align[i] = 'r';
        else
            align[i] = 'l';
    }
}

emitcell(s, w, al)
char *s;
int w;
char al;
{
    int dw = cellwidth(s);
    int pad = w - dw;
    int lpad = 0, rpad = 0;

    if (al == 'c') {
        lpad = pad / 2;
        rpad = pad - lpad;
    } else if (al == 'r')
        lpad = pad;
    else
        rpad = pad;
    while (lpad-- > 0)
        outc(' ');
    renderinline(s);
    while (rpad-- > 0)
        outc(' ');
}

table(hdr, sep)
char *hdr;
char *sep;
{
    int i, c, r;
    char *s;

    putblank();
    ncols = splitcells(hdr);
    for (i = 0; i < ncols; i++)
        strcpy(hcell[i], cell[i]);
    parsealign(sep);

    /* collect the data rows, measuring as we store them */
    nrows = 0;
    tblused = 0;
    for (;;) {
        s = rdline();
        if (s == 0)
            break;
        if (isblankline(s) || !haspipe(s)) {
            pushback(s);
            break;
        }
        if (nrows >= MAXROWS || strlen(s) >= MAXTBL - tblused - 1) {
            fprintf(stderr, "md: table too large\n");
            break;
        }
        rowstart[nrows] = tblused;
        strcpy(tbl + tblused, s);
        tblused += strlen(s) + 1;
        nrows++;
    }

    for (c = 0; c < ncols; c++)
        widths[c] = cellwidth(hcell[c]);
    for (r = 0; r < nrows; r++) {
        splitcells(tbl + rowstart[r]);
        for (c = 0; c < ncols; c++) {
            int w = cellwidth(cell[c]);
            if (w > widths[c])
                widths[c] = w;
        }
    }

    /* header, in bold */
    forcesty = 1;
    for (c = 0; c < ncols; c++) {
        if (c > 0) {
            outc(' '); outc('|'); outc(' ');
        }
        emitcell(hcell[c], widths[c], align[c]);
    }
    forcesty = 0;
    outc('\n');

    /* a dash rule under the header */
    for (c = 0; c < ncols; c++) {
        if (c > 0) {
            outc('-'); outc('-'); outc('-');
        }
        for (i = 0; i < widths[c]; i++)
            outc('-');
    }
    outc('\n');

    /* the data rows */
    for (r = 0; r < nrows; r++) {
        splitcells(tbl + rowstart[r]);
        for (c = 0; c < ncols; c++) {
            if (c > 0) {
                outc(' '); outc('|'); outc(' ');
            }
            forcesty = 0;
            emitcell(cell[c], widths[c], align[c]);
        }
        outc('\n');
    }
    putblank();
}

/*
 * The main loop.  Read a line, classify its block, and dispatch.
 */
process()
{
    char *s, *t;
    int lvl;

    for (;;) {
        s = rdline();
        if (s == 0)
            break;
        if (isblankline(s)) {
            endpara();
            putblank();
            continue;
        }
        if (isfence(s)) {
            endpara();
            fenced();
            continue;
        }
        if ((lvl = atx(s)) > 0) {
            endpara();
            heading(lvl, htext);
            continue;
        }
        if (listmark(s) != 0) {
            endpara();
            dolist(s);
            continue;
        }
        if (isquote(s)) {
            endpara();
            doquote(s);
            continue;
        }
        if (ishr(s)) {
            endpara();
            dhr();
            continue;
        }
        /* a text line: setext heading or table needs the next line */
        t = rdline();
        if (t != 0) {
            if (!inpara && (lvl = setext(t)) != 0) {
                heading(lvl, s);
                continue;
            }
            if (istablesep(t) && haspipe(s)) {
                table(s, t);
                continue;
            }
            pushback(t);
        }
        addpara(s);
    }
    endpara();
}

main(argc, argv)
int argc;
char **argv;
{
    int i, nfiles = 0;
    int n;
    FILE *f;

    out = stdout;
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'w' && argv[i][2] == '\0') {
            if (++i >= argc) {
                fprintf(stderr, "md: -w needs a number\n");
                exit(1);
            }
            n = 0;
            {
                char *p = argv[i];
                while (*p >= '0' && *p <= '9')
                    n = n * 10 + (*p++ - '0');
            }
            rmargin = n;
            if (rmargin < 20)
                rmargin = 20;
        } else if (argv[i][0] == '-' && argv[i][1] == '\0')
            nfiles++;
        else if (argv[i][0] == '-') {
            fprintf(stderr, "%s: unknown option %s\n", progname, argv[i]);
            exit(1);
        } else
            nfiles++;
    }

    if (nfiles == 0) {
        inf = stdin;
        process();
    } else {
        for (i = 1; i < argc; i++) {
            if (argv[i][0] == '-' && argv[i][1] == 'w' && argv[i][2] == '\0') {
                i++;
                continue;
            }
            if (argv[i][0] == '-' && argv[i][1] == '\0') {
                inf = stdin;
                process();
                continue;
            }
            if ((f = fopen(argv[i], "r")) == 0) {
                perror(argv[i]);
                continue;
            }
            inf = f;
            process();
            fclose(f);
        }
    }
    exit(0);
}
