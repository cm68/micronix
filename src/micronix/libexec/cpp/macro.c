/*
 * macros are done in a unified way with include file processing:
 *
 * I think this implementation is a little more restrictive than normal
 * cpp, in that the values to substitute need to be single tokens
 * this can probably be relaxed at some point, but I need to get a good
 * handle on what the actual standard says
 *
 * these are the canonical examples:
 * #define k(a,b) a##b
 * #define k(a,b) if (a) b=0
 * #define k(a,b) if (a) s=#b
 */
#include <stdlib.h>
#include <string.h>
#include "cpp.h"

char *macbuffer;
struct macro *macros;

/* C identifier character classes - used by macexpand to scan ident tokens
 * inside macro replacement text without dragging in ctype.h. */
int
is_id_start(unsigned char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

int
is_id_cont(unsigned char c)
{
    return is_id_start(c) || (c >= '0' && c <= '9');
}

/*
 * Numeric defines get a compact store.  Most of what a real header
 * defines is numbers - token codes, flags, filtenum's lowered enum
 * constants - and for pass1.c that was 304 of 322 macros: a struct,
 * a name copy, and the number spelled out as text, about twenty
 * bytes each to hold a value that fits in four.  An ndef is the
 * interned name (shared with the identifier pool the lexer already
 * maintains), the value, and a link; expansion synthesizes digits
 * back into the shared buffer.  Anything that is not a bare number
 * - suffixed constants, expressions, anything function-like - still
 * takes the full macro path.
 */
/*
 * The store packs into 256-byte slabs: no per-entry link, no NUL,
 * no alignment, and the value in as few bytes as it needs.
 *
 *	header	1 byte: bit 7 dead, bits 5-6 value width code
 *		(0: 1 byte, 1: 2 bytes, 2: 4 bytes), bits 0-4 name length
 *	value	that many bytes, little-endian, sign-extended on read
 *	name	length bytes, no terminator
 *
 * permalloc zeroes, so a zero header ends a slab's used region; the
 * appender always leaves at least one spare byte so the terminator
 * exists even in a full slab.  A slab's first two bytes link to the
 * older slab.
 */
#define NSLAB 256
static unsigned char *nslabs;	/* newest slab; head links older */
static unsigned char *nfree;	/* append cursor in newest slab */
static unsigned char *nend;	/* its end */

/*
 * Width codes - everything numval admits fits in two bytes, so the
 * codecs have no long arithmetic in them at all:
 *	0: 1 byte, signed	(-128..127)
 *	1: 2 bytes, unsigned	(128..65535)
 *	2: 2 bytes, signed	(-32768..-129)
 */
long
ndget(unsigned char *p, unsigned char w)
{
    unsigned v;

    if (w == 0)
        return (char)p[0];
    v = p[0] | (p[1] << 8);
    if (w == 1)
        return (long)v;
    return (short)v;	/* (int) would not sign-extend on the host */
}

void
ndput(unsigned char *p, unsigned char w, int val)
{
    *p++ = val;
    if (w)
        *p = val >> 8;
}

#define NDWIDTH(v) ((v) >= -128 && (v) < 128 ? 0 : (v) > 0 ? 1 : 2)
#define NDLEN(w) ((w) ? 2 : 1)

/*
 * One bit per hash of a stored name.  macexpand asks the numeric store
 * about every identifier it might expand and the answer is almost
 * always no, which used to cost a walk of every entry in every slab -
 * a quarter of the preprocessor on our worst source.
 *
 * A clear bit is a definite no.  A set bit means only "maybe", so a
 * collision costs the old walk and nothing more; ndefundef leaves the
 * bit standing rather than rescan to see whether some other live name
 * still needs it.  Nothing here can produce a wrong answer, only a
 * slow one.
 *
 * Thirty-two bytes.  This file packs its entries into slabs to save
 * room on a machine that has none to spare, so the index has to be
 * cheaper than what it indexes.
 */
/*
 * An index over the slabs, because the walk does not scale.  nm.c
 * stores seventy names and rules.c a hundred and forty two - it takes
 * opcodes.h and lexops.h, which are tables of them - and a bitmap
 * sized for the first is saturated by the second and stops rejecting
 * anything.  A slot per name answers both.
 *
 * Open addressed, linear probe, holding pointers to the packed
 * entries rather than copies of them.  An empty slot ends a probe and
 * is the common answer.  Dead entries - ndefundef marks the header,
 * it does not remove it - stay indexed and are stepped over, which
 * keeps the probe chains intact.
 *
 * If a source ever defines more names than the table will hold,
 * ndifull says so and the old walk answers instead, so the fallback
 * is correctness and not an error.
 */
#define NDIDX 512
static unsigned char *ndidx[NDIDX];
static unsigned short ndicount;
static char ndifull;

/*
 * Length and hash in one walk, because the walk was already being made
 * for the length.  Two bits are set per name rather than one: with
 * seventy names in two hundred and fifty six bits a single bit leaves
 * a quarter of the table standing even with a perfect hash, and two
 * independent bits take the false-yes rate below a tenth without
 * costing another byte of store.
 *
 * The rotate is what makes the two bytes independent - a plain sum
 * loses the order of the characters, and identifiers here differ
 * mostly in their order.  Measured over the real traffic on our worst
 * source, 5014 lookups against 70 stored names: first-and-last-and-
 * length let 45.8% through, a byte sum 36.1%, this 8.1%.
 */
static unsigned short
ndhash(char *s, unsigned char *lenp)
{
    unsigned short a = 0;
    unsigned char l = 0;

    while (*s) {
        a = ((a << 1) | (a >> 15)) ^ (unsigned char)*s++;
        l++;
    }
    *lenp = l;
    return a;
}


/*
 * Find a live entry.  Returns the header pointer, or 0.
 */
unsigned char *
ndeffind(char *name)
{
    unsigned char *slab;
    register unsigned char *p;
    unsigned char h, len, hlen;
    unsigned char nl;
    unsigned short nh = ndhash(name, &nl);

    if (nl == 0)
        return 0;

    if (!ndifull) {
        unsigned short i = nh & (NDIDX - 1);

        while ((p = ndidx[i]) != 0) {
            h = *p;
            if (!(h & 0x80) && (h & 0x1f) == nl &&
                memcmp((char *)p + 1 + NDLEN((h >> 5) & 3), name, nl) == 0)
                return p;
            i = (i + 1) & (NDIDX - 1);
        }
        return 0;
    }

    for (slab = nslabs; slab; slab = *(unsigned char **)slab) {
        p = slab + sizeof(char *);
        while ((h = *p)) {
            len = h & 0x1f;
            hlen = 1 + NDLEN((h >> 5) & 3);
            if (!(h & 0x80) && len == nl &&
                memcmp((char *)p + hlen, name, len) == 0)
                return p;
            p += hlen + len;
        }
    }
    return 0;
}

/*
 * A pure numeric body: [-]digits or [-]0x hex, nothing else - no
 * suffixes, no expressions, and nothing outside -32768..65535.
 * Everything else keeps its text: a 32-bit value respelled through
 * a long would print differently on the host and the Z80 (and a
 * negative respelling splits one NUMBER token into MINUS NUMBER,
 * which the original spelling of a big hex constant does not).
 */
char
numval(char *s, long *out)
{
    long v = 0;
    char neg = 0;
    unsigned char c;

    if (*s == '-') {
        neg = 1;
        s++;
    }
    if (!*s)
        return 0;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
        if (!*s)
            return 0;
        while ((c = *s++)) {
            if (c >= '0' && c <= '9')
                c -= '0';
            else if (c >= 'a' && c <= 'f')
                c -= 'a' - 10;
            else if (c >= 'A' && c <= 'F')
                c -= 'A' - 10;
            else
                return 0;
            if (v > 4095L)
                return 0;
            v = (v << 4) | c;
        }
    } else if (s[0] == '0' && s[1]) {
        /*
         * A leading zero is octal, and it matters that it is: this
         * body is respelled in decimal at expansion, so the base
         * has to be honoured here or it is lost for good.  "0200"
         * read as two hundred is how every stdio flag past 010
         * came out wrong - _IOBINARY as 200 is _IOSTRG|_IOMYBUF
         * with the real bit, and fgetc took every binary stream
         * for a string it had run off the end of.
         */
        s++;
        while ((c = *s++)) {
            if (c < '0' || c > '7')
                return 0;
            if (v > 8191L)
                return 0;
            v = (v << 3) | (c - '0');
        }
    } else {
        while ((c = *s++)) {
            if (c < '0' || c > '9')
                return 0;
            if (v > 6553L)
                return 0;
            v = v * 10 + (c - '0');
        }
    }
    if (v > 65535L || (neg && v > 32768L))
        return 0;
    *out = neg ? -v : v;
    return 1;
}

/* found + value out, for expansion and defined-ness */
char
ndefval(char *name, long *out)
{
    unsigned char *p = ndeffind(name);

    if (!p)
        return 0;
    *out = ndget(p + 1, (*p >> 5) & 3);
    return 1;
}

void
ndefadd(char *name, long lval)
{
    register unsigned char *p = ndeffind(name);
    int val = (int)lval;
    unsigned char w;
    unsigned char len;
    unsigned char hlen;

    w = lval >= -128 && lval < 128 ? 0 : lval > 0 ? 1 : 2;

    if (p) {
        unsigned char ow = (*p >> 5) & 3;
        if (w == ow || (w == 0 && ow == 2)) {	/* re-encodable in place */
            ndput(p + 1, ow, val);
            return;
        }
        *p |= 0x80;		/* changed sign class: dead, re-append */
    }
    len = strlen(name);
    hlen = 1 + NDLEN(w);
    if (!nslabs || nfree + hlen + len >= nend) {
        unsigned char *s = (unsigned char *)permalloc(NSLAB);
        *(unsigned char **)s = nslabs;
        nslabs = s;
        nfree = s + sizeof(char *);
        nend = s + NSLAB;
    }
    p = nfree;
    *p = (w << 5) | len;
    ndput(p + 1, w, val);
    memcpy((char *)p + hlen, name, len);
    nfree = p + hlen + len;
    if (!ndifull) {
        unsigned char nl;
        unsigned short i = ndhash(name, &nl) & (NDIDX - 1);

        if (ndicount >= NDIDX - (NDIDX / 4)) {
            ndifull = 1;    /* too full to probe cheaply - walk instead */
        } else {
            while (ndidx[i])
                i = (i + 1) & (NDIDX - 1);
            ndidx[i] = p;
            ndicount++;
        }
    }
}

/* remove from the numeric store, if present (#undef, redefinition) */
void
ndefundef(char *name)
{
    unsigned char *p = ndeffind(name);

    if (p)
        *p |= 0x80;
}

#ifdef DEBUG
int
ndefstat(int *bytes)
{
    unsigned char *slab, *p;
    unsigned char h;
    int c = 0;
    *bytes = 0;
    for (slab = nslabs; slab; slab = *(unsigned char **)slab) {
        p = slab + sizeof(char *);
        while ((h = *p)) {
            if (!(h & 0x80))
                c++;
            *bytes += 1 + NDLEN((h >> 5) & 3) + (h & 0x1f);
            p += 1 + NDLEN((h >> 5) & 3) + (h & 0x1f);
        }
    }
    return c;
}
#endif

/* defined-ness, for #ifdef and defined(): either store counts */
char
mdefined(char *s)
{
    long v;

    return maclookup(s) != 0 || ndefval(s, &v);
}

#ifdef DEBUG
int macpeak;	/* macbuffer high-water */
#endif

/* Lazily allocate the shared expansion buffer (used by macdefine + macexpand). */
void
macbuf_init(void)
{
    if (!macbuffer)
        macbuffer = xalloc(MACBUF);
}

/*
 * Add a macro definition from command-line argument
 *
 * Processes -D command-line arguments to define preprocessor macros before
 * parsing the source file. Supports both simple and value-defined macros.
 *
 * Formats accepted:
 *   - "NAME"       -> defines NAME as 1
 *   - "NAME=value" -> defines NAME as value
 *
 * The macro is added to the front of the macro list and becomes immediately
 * available for expansion during preprocessing.
 *
 * Object-like macros only:
 *   - No parameters (parmcount=0)
 *   - Direct text replacement
 *   - Cannot define function-like macros from command line
 *
 * Examples:
 *   -DDEBUG         -> DEBUG=1
 *   -DVERSION=2     -> VERSION=2
 *   -DMAX=100       -> MAX=100
 *
 * Parameters:
 *   s - Definition string in "NAME" or "NAME=value" format
 */
void
addDefine(char *s)
{
    struct macro *m;
    char *eq;
    unsigned char namelen;
    char nbuf[MAXSYMLEN];
    char *text;
    long v;

    if (!*s) {
        return;
    }

    /* Find '=' to separate name from value */
    eq = strchr(s, '=');

    if (eq) {
        namelen = eq - s;
        if (namelen > MAXSYMLEN - 1)
            namelen = MAXSYMLEN - 1;
        memcpy(nbuf, s, namelen);
        nbuf[namelen] = '\0';
        text = eq + 1;
    } else {
        strncpy(nbuf, s, MAXSYMLEN - 1);
        nbuf[MAXSYMLEN - 1] = '\0';
        text = "1";  /* default (like -DDEBUG) */
    }

    if (numval(text, &v)) {
        ndefadd(nbuf, v);
        return;
    }

    m = (struct macro *)permalloc(sizeof(*m));
    m->name = permdup(nbuf);
    m->mactext = permdup(text);
    m->parmcount = 0;
    m->parms = 0;
    m->next = macros;
    macros = m;
}

/*
 * Look up a macro by name
 *
 * Searches the macro list for a macro with the specified name. The macro
 * list is a simple linked list with most recently defined macros at the
 * front.
 *
 * Search order:
 *   - Linear search from front to back
 *   - First match wins (allows redefinition by prepending)
 *   - Case-sensitive name matching
 *
 * Used by:
 *   - macexpand() to check if identifier is a macro
 *   - #ifdef/#ifndef to test macro existence
 *   - #undef to find macro for removal
 *
 * Parameters:
 *   name - Macro name to search for
 *
 * Returns:
 *   Pointer to macro structure if found, NULL if not found
 */
struct macro *
maclookup(char *name)
{
    struct macro *m;


    /*
     * The first character before the call: this list is walked for
     * every identifier that might expand, and on our worst source
     * that was ninety-two thousand strcmps to find at most one match.
     * Nearly all of them disagree in the first byte, and on the Z80
     * strcmp is a byte loop with a call in front of it, not one
     * instruction.
     */
    for (m = macros; m; m = m->next) {
        if (m->name[0] == name[0] && strcmp(m->name, name) == 0) {
            return m;
        }
    }
    return 0;
}

/*
 * Remove a macro definition (#undef)
 *
 * Unlinks the macro from the list.  Definitions live in the permanent
 * arena, so the storage is simply abandoned - #undef is rare and the
 * waste is bounded by the number of #undefs in a run.
 *
 * Silently succeeds if macro not found (standard C preprocessor behavior).
 *
 * Parameters:
 *   s - Name of macro to undefine
 */
void
macundefine(char *s)
{
    struct macro *m, *p;

    ndefundef(s);
    m = maclookup(s);
    if (!m) {
        return;
    }
    /* Unlink from list */
    if (m == macros) {
        macros = m->next;
    } else {
        for (p = macros; p->next != m; p = p->next) ;
        p->next = m->next;
    }
}

/*
 * Parse and define a macro from #define directive
 *
 * Reads a macro definition from the input stream and creates a macro
 * structure. Handles both object-like and function-like macros with
 * parameter parsing.
 *
 * Macro forms:
 *   - Object-like:     #define NAME replacement_text
 *   - Function-like:   #define NAME(a,b) replacement_text
 *
 * Function-like detection:
 *   - '(' must IMMEDIATELY follow name (no whitespace) per C standard
 *   - Whitespace before '(' means object-like macro with '(' in text
 *
 * Parameter parsing:
 *   - Comma-separated identifiers: NAME(a,b,c)
 *   - Whitespace allowed around commas
 *   - Closing ')' terminates parameter list
 *   - Parameters stored in macro->parms array
 *
 * Replacement text processing:
 *   - Everything after parameters/whitespace until newline
 *   - Backslash-newline: Continues macro to next line (replaced with space)
 *   - C++ comments (//): Terminates macro text (not included)
 *   - C block comments: Removed, replaced with single space
 *   - Trailing whitespace: Trimmed
 *
 * Special operators in replacement text:
 *   - Hash (stringify): Must precede parameter name
 *   - Double-hash (token paste): Glues adjacent tokens together
 *   - Both handled during expansion, not definition
 *
 * Parameters:
 *   s - Macro name (already parsed from input stream)
 *
 * Side effects:
 *   - Consumes input stream through newline
 *   - Adds macro to front of macro list
 *   - Macro immediately available for expansion
 */
void
macdefine(char *s)
{
    unsigned char i;
    struct macro *m;
    char *parms[MAXPARMS];
    unsigned char parmcount = 0;
    char fnlike = 0;
    char nbuf[MAXSYMLEN];
    long v;

    macbuf_init();
    /* s is the shared symbol buffer and the parameter scan below
     * refills it - take the name before anything advances */
    strncpy(nbuf, s, MAXSYMLEN - 1);
    nbuf[MAXSYMLEN - 1] = 0;

    /*
     * Check for function-like macro: '(' must be IMMEDIATELY after name
     * with NO whitespace (C standard requirement)
     * If there's whitespace before '(', it's part of the replacement text
     */
    if (curchar == '(') {
        fnlike = 1;
        advance();
        while (1) {
            skipws1();
            if (issym()) {
                advance();
                parms[parmcount++] = permdup(s);
                skipws1();
                if (curchar == ',') {
                    advance();
                    continue;
                }
            }
            if (curchar == ')') {
                break;
            }
            gripe(ER_C_DP);
            break;  /* Exit loop on error to avoid infinite loop */
        }
        advance();
        skipws1();
    } else {
        /* Object-like macro: skip whitespace before replacement text */
        skipws1();
    }
    s = macbuffer;
    /* we copy to the macbuffer the entire logical line,
     * spaces and tabs included, but stop at // comments */
    while (curchar != '\n') {
        /* Check for C++ style comment */
        if (curchar == '/' && nextchar == '/') {
            /* Skip rest of line - treat // as end of macro text */
            while (curchar != '\n') {
                advance();
            }
            break;
        }
        /* Check for C-style block comment */
        if (curchar == '/' && nextchar == '*') {
            /* Skip comment - do not include in macro text */
            advance();  /* skip '/' */
            advance();  /* skip '*' */
            while (1) {
                if (curchar == '*' && nextchar == '/') {
                    advance();  /* skip '*' */
                    advance();  /* skip '/' */
                    break;
                }
                if (curchar == 0) {
                    /* Unterminated comment - EOF */
                    break;
                }
                /* Continue skipping through newlines in comment */
                advance();
            }
            /* Replace comment with single space (to separate tokens) */
            if (s > macbuffer && s[-1] != ' ' && s[-1] != '\t') {
                *s++ = ' ';
            }
            continue;
        }
        if ((curchar == '\\') && (nextchar == '\n')) {
            advance();
            curchar = ' ';
        }
        if (s >= macbuffer + MACBUF - 2) {
            gripe(ER_C_TL);
            break;
        }
        *s++ = curchar;
        advance();
    }
    *s = 0;

    /* Trim trailing whitespace from macro text */
    while (s > macbuffer && (s[-1] == ' ' || s[-1] == '\t')) {
        s--;
        *s = 0;
    }

    advance();  /* eat the newline */

    /* an object-like macro whose body is a bare number is an ndef */
    if (!fnlike && numval(macbuffer, &v)) {
        ndefadd(nbuf, v);
        return;
    }

    m = (struct macro *)permalloc(sizeof(*m));
    m->name = permdup(nbuf);
    m->parmcount = parmcount;
    m->parms = NULL;  /* NULL means object-like macro */
    if (fnlike) {
        if (parmcount) {
            m->parms = (char **)permalloc(sizeof(char *) * parmcount);
            for (i = 0; i < parmcount; i++) {
                m->parms[i] = parms[i];
            }
        } else {
            /* Function-like macro with 0 params: use sentinel */
            m->parms = (char **)1;  /* Non-NULL to indicate function-like */
        }
    }
    ndefundef(nbuf);	/* a text body shadows an old numeric one */
    m->mactext = permdup(macbuffer);
    m->next = macros;
    macros = m;
}

/*
 * Expand a macro invocation
 *
 * Checks if an identifier is a macro and expands it if so. Handles both
 * object-like and function-like macros with parameter substitution,
 * stringify (#), and token pasting (##) operators.
 *
 * Expansion process:
 *   1. Look up macro by name
 *   2. For function-like macros: parse argument list from input
 *   3. Build expansion text with parameter substitution
 *   4. Insert expansion into input stream via insertmacro()
 *   5. Recursive expansion happens when inserted text is processed
 *
 * Object-like macros:
 *   - Direct text replacement
 *   - No arguments parsed
 *   - Example: #define MAX 100
 *
 * Function-like macros:
 *   - Arguments parsed from (arg1, arg2, ...) in input
 *   - Parentheses levels tracked (nested calls handled)
 *   - String/character literals copied verbatim
 *   - Arguments matched to parameters by position
 *   - Parameter mismatch generates error
 *
 * Parameter substitution:
 *   - Formal parameters in macro text replaced with actual arguments
 *   - Identifiers in macro text matched against parameter names
 *   - Non-matching identifiers passed through unchanged
 *
 * Stringify operator (single hash):
 *   - Converts parameter to string literal
 *   - Adds quotes around parameter value in expansion
 *   - Example: STR(x) with "x" -> STR(foo) becomes "foo"
 *
 * Token paste operator (double hash):
 *   - Concatenates adjacent tokens
 *   - Simply removed during expansion (tokens already adjacent)
 *   - Example: CONCAT(a,b) with "a" and "b" -> CONCAT(foo,bar) becomes foobar
 *
 * Nested macro calls:
 *   - Arguments can contain macro invocations
 *   - Processed outside-in (outer expanded first)
 *   - Example: foo(bar(x)) expands foo first with bar(x) as argument
 *
 * Special asm block handling:
 *   - If newline seen during argument parsing in asm block
 *   - Appends semicolon to expansion text
 *   - Maintains asm statement separation
 *
 * Parameters:
 *   s - Identifier name to check and potentially expand
 *
 * Returns:
 *   1 if macro found and expanded, 0 if not a macro
 *
 * Side effects:
 *   - Consumes macro arguments from input stream
 *   - Inserts expansion text into input stream
 *   - Advances curchar/nextchar to start of expansion
 */
char
macexpand(char *s)	/* the symbol we are looking up as a macro */
{
    struct macro *m;
    unsigned char plevel;
    char *d;
    unsigned char args;
    char *parms[MAXPARMS];
    unsigned char c;
    char *n;
    unsigned char i;
    char stringify = 0;
    char *lim;			/* set below: macbuffer is malloc'd by
				 * macbuf_init, so the end of it is not
				 * known until that has run */

    long ndv;

    macbuf_init();
    if (ndefval(s, &ndv)) {
        fmtstr(fmtlong(macbuffer, ndv), " ");
        insertmacro(s, macbuffer);	/* insertmacro interns the name */
        return 1;
    }
    m = maclookup(s);
    if (!m) {
        return 0;
    }

    lim = macbuffer + MACBUF - 4;

    args = 0;
    d = macbuffer;
    plevel = 0;

    /*
     * Only parse arguments for function-like macros (m->parms != NULL).
     * Object-like macros should not consume following parentheses.
     */
    if (m->parms != NULL) {
        /* this will stop after nextchar is not white space */
        while (iswhite(nextchar)) {
            advance();
        }
        if (nextchar != '(') {
            /* Function-like macro invoked without (), not an invocation */
            return 0;
        }
        advance();
        plevel = 1;
        advance();
        skipws();
        while (1) {
            /*
             * copy literals literally
             */
            if (curchar == '\'' || curchar == '\"') {
                c = curchar;
                advance();
                *d++ = c;
                while (curchar != c && d < lim) {
                    *d++ = curchar;
                    if (curchar == '\\') {
                        advance();
                        *d++ = curchar;
                    }
                    advance();
                }
            }
            if (curchar == '(') {
                plevel++;
            }
            if (curchar == ')') {
                plevel--;
            }
            /*
             * only advance when we have a non-parenthesized comma
             */
            if (((plevel == 1) && (curchar == ',')) ||
                ((plevel == 0) && (curchar == ')'))) {
                *d++ = 0;
                /* Only count argument if we have content, OR if we already
                 * have args (to handle trailing comma case like FOO(a,)) */
                if (d > macbuffer + 1 || args > 0) {
                    parms[args++] = strdup(macbuffer);
                }
                if (curchar == ')') {
                    break;
                }
                d = macbuffer;
                advance();
                skipws();
                continue;
            }
            if (d >= lim) {
                gripe(ER_C_TL);
                break;
            }
            *d++ = curchar;
            *d = 0;
            advance();
        }
    } /* curchar should be ')' */

    if (args != m->parmcount) {
        gripe(ER_C_MA);
        for (i = 0; i < args; i++)
            free(parms[i]);
        return 0;
    }

    /*
     * now we copy the macro text to the macbuffer, expanding
     * the parameters where ever we find them
     */
    d = macbuffer;
    *d = '\0';
    s = m->mactext;

    while (*s) {
        c = *s;
        /* literals go straight across */
        if ((c == '\'') || (c == '\"')) {
            *d++ = *s++;
            while (*s != c && d < lim) {
                /* don't notice literal next quote */
                if (*s == '\\' && s[1] == c) {
                    *d++ = *s++;
                }
                *d++ = *s++;
            }
            *d++ = *s++;
            continue;
        }

        /* is it a glom or stringify */
        stringify = 0;
        if (c == '#') {
            c = *++s;
            if (c == '#') {
                c = *++s;
            } else {
                stringify = 1;
            }
        }

        /* if macro text has something that looks like an arg */
        if (is_id_start(c)) {
            n = strbuf;
            while ((c = *s) && is_id_cont(c)) {
                *n++ = *s++;
            }
            *n++ = 0;
            n = strbuf;
            /* if it matches our declared arg name */
            /*
             * Every identifier in every macro body is matched against
             * every declared parameter - a hundred and forty five
             * thousand strcmps on rules.c, for bodies that declare at
             * most a handful.  The first character settles nearly all
             * of them without the call.
             */
            for (i = 0; i < args; i++) {
                if (m->parms[i][0] == strbuf[0] &&
                    strcmp(m->parms[i], strbuf) == 0) {
                    n = parms[i];
                    break;
                }
            }
            if (stringify) {
                *d++ = '\"';
            }
            while (*n && d < lim) {
                *d++ = *n++;
            }
            if (stringify) {
                *d++ = '\"';
            }
            continue;
        }
        if (d >= lim) {
            gripe(ER_C_TL);
            break;
        }
        *d++ = *s++;
    }
    *d = 0;

    /*
     * Add trailing space to prevent token concatenation with following text.
     * Without this, "LTYPE lcval" where LTYPE=long becomes "longlcval".
     * Don't add if expansion is empty or already ends with whitespace.
     */
    if (d > macbuffer && d[-1] != ' ' && d[-1] != '\t') {
        *d++ = ' ';
        *d = 0;
    }

#ifdef DEBUG
    if (d - macbuffer > macpeak)
        macpeak = d - macbuffer;
#endif
    insertmacro(m->name, macbuffer);

    for (i = 0; i < args; i++)
        free(parms[i]);

    return 1;
}

/*
 */

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
