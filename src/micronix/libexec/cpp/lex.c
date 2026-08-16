/*
 * this is a brute force lexer that uses a tight keyword lookup in kw.c
 * we do cpp conditionals in here
 * it handles different keyword tables for cpp, c, and asm
 *
 * XXX - maybe use character type bitmask
 */
#include <stdlib.h>
#include <string.h>
#include "cpp.h"

static unsigned char incomment = 0;
/* Track comment state across gettoken() calls */

static token_t prevTokType = 0;
/* Previous token type - used to detect valid label positions */

static char *pendingAsm = NULL;
/* Pending asm text to emit as STRING after ASM keyword */
static char pendingSemi = 0;
/* Emit SEMI after ASMSTR */

struct token cur, next;

/*
 * this is the place we build filenames, symbols and literal strings
 * no overflow checking or anything, we ain't got time for that shit
 */
char strbuf[STRBUFSIZE];

/*
 * Large buffer for asm blocks and long concatenated strings
 * BSS - uninitialized, doesn't bloat binary
 */
/*
 * String literals land here, adjacent ones concatenated.  It was a
 * kilobyte; the longest run of literals anywhere in this tree is 126
 * characters, in peep/rules.c, so this is four times the observed
 * worst case and still gives back five hundred bytes of heap on the
 * 64K machine - which is what decides whether a source compiles
 * there at all.  Overflow is diagnosed, not silently truncated: see
 * "string too long" below.
 */
#define BIGBUFSIZE 512

/* getlit(): out-of-band "terminator after backslash-newline" marker */
#define GL_END 0x100
static char bigbuf[BIGBUFSIZE];
static int bigbuflen;

unsigned long readcppconst();
char cpppseudofunc();

/*
 * cpp conditional state
 */
struct cond *cond;

unsigned char tflags;

/*
 * Check if current token matches and consume it if so
 *
 * Common pattern in parsing: test for token type and consume if matched.
 * This helper combines both operations to reduce code size throughout
 * the parser.
 *
 * Typical usage:
 *   if (match(SEMI)) { ... }  - consumes ';' if present
 *   while (match(COMMA)) { parseNext(); }  - loop over comma-separated list
 *
 * Parameters:
 *   t - Token type to check for
 *
 * Returns:
 *   1 if current token matched (and consumed), 0 otherwise
 *
 * Side effects:
 *   - Advances token stream if match succeeds (calls gettoken())
 */
char
match(token_t t)
{
    if (cur.type == t) {
        gettoken();
        return 1;
    }
    return 0;
}

/*
 * Check if current character matches and consume it if so
 *
 * Similar to match() but operates at character level rather than token level.
 * Used during low-level tokenization to test and consume single characters
 * from the input stream.
 *
 * Common usages:
 *   - Testing for specific delimiters: charmatch('\'')
 *   - Consuming expected characters: charmatch('0')
 *   - Operator parsing: charmatch('=')
 *
 * Parameters:
 *   c - Character to check for
 *
 * Returns:
 *   1 if curchar matched (and consumed), 0 otherwise
 *
 * Side effects:
 *   - Advances character stream if match succeeds (calls advance())
 */
char
charmatch(unsigned char c)
{
    if (curchar == c) {
        advance();
        return 1;
    } else {
        return 0;
    }
}

/*
 * Skip whitespace including newlines
 *
 * Advances past all space and newline characters. Used when whitespace
 * (including line breaks) is insignificant in the current context.
 *
 * Characters skipped:
 *   - Space (' ')
 *   - Newline ('\n')
 *
 * Characters NOT skipped:
 *   - Tab ('\t') - not considered whitespace by this function
 *
 * Side effects:
 *   - Advances character stream until non-whitespace found
 */
void
skipws()
{
    unsigned char c;

    while ((c = curchar) == ' ' || c == '\n') {
        advance();
    }
}

/*
 * Skip whitespace excluding newlines
 *
 * Advances past space characters but stops at newlines. Used in contexts
 * where newline is significant (preprocessor directives, ONELINE mode).
 *
 * Characters skipped:
 *   - Space (' ')
 *
 * Characters NOT skipped:
 *   - Newline ('\n') - significant in CPP directives
 *   - Tab ('\t') - not considered whitespace by this function
 *
 * Side effects:
 *   - Advances character stream until newline or non-space found
 */
void
skipws1()
{
    unsigned char c;

    while ((c = curchar) == ' ' || c == '\t') {
        advance();
    }
}

/* Skip ' ', '\t', '\n' (used at lexer points where newlines aren't significant). */
void
skipallws()
{
    unsigned char c;

    while ((c = curchar) == ' ' || c == '\t' || c == '\n')
        advance();
}

/*
 * Skip to end of current line
 *
 * Advances past all characters until newline or EOF. Used to discard
 * rest of preprocessor directives, comments, or error recovery.
 *
 * Stops at:
 *   - Newline ('\n') - NOT consumed, left as curchar
 *   - EOF (curchar == 0)
 *
 * Side effects:
 *   - Advances character stream
 *   - Leaves curchar at newline or 0
 */
void
skiptoeol()
{
    while (curchar && (curchar != '\n')) {
        advance();
    }
}

/*
 * Read integer constant in specified base from character stream
 *
 * Converts character sequence to integer value. Handles multiple bases
 * (binary, octal, decimal, hexadecimal) with full digit validation.
 *
 * Supported bases:
 *   - Binary (2):  0b1010
 *   - Octal (8):   0755, 010
 *   - Decimal (10): 123, 0d99
 *   - Hexadecimal (16): 0xABCD, 0xff
 *
 * Digit validation:
 *   - Rejects digits >= base (e.g., '8' in octal)
 *   - Accepts a-f/A-F for hex (case insensitive)
 *   - Stops at first invalid character
 *
 * Error handling:
 *   - If no digits consumed for base 2 or 16, reports ER_C_NX error
 *   - Base 8/10 allow zero-length (return 0)
 *
 * Parameters:
 *   base - Number base (2, 8, 10, or 16)
 *
 * Returns:
 *   Integer value parsed from input stream
 *
 * Side effects:
 *   - Consumes digit characters from input
 *   - Leaves curchar at first non-digit
 *   - May report error for empty hex/binary literals
 */
long
getint(unsigned char base)
{
    long i = 0;
    unsigned int b = base;  /* keep uchar out of long arithmetic (zc3) */
    unsigned int d;
    unsigned char c;
    int len = 0;

    while (1) {
        c = curchar;
        if (c < '0') break;
        if (c > '9') {
            c |= 0x20;
            if (c >= 'a' && c <= 'f') {
                c = 10 + c - 'a';
            } else {
                break;
            }
        } else {
            c -= '0';
        }
        if ((c+1) > base) {
            break;
        }
        d = c;
        i = i * b + d;
        advance();
        len++;
    }
    /* if no characters are consumed, note the error if base 2 or 16 */
    if ((len == 0) && ((base == 2) || (base == 16))) {
        gripe(ER_C_NX);
    }
    return i;
}

/*
 * Parse character literal with escape sequence handling
 *
 * Processes a single character from a character or string literal, handling
 * all standard C escape sequences plus custom extensions. This is the core
 * function for interpreting escape codes in both 'x' and "string" literals.
 *
 * Standard C escape sequences:
 *   \\b - backspace       \\n - newline        \\r - carriage return
 *   \\t - tab             \\f - form feed      \\v - vertical tab
 *   \\NNN - octal (up to 3 digits, 0-7)
 *   \\xNN - hexadecimal (any length, 0-9 a-f A-F)
 *   \\\\ - backslash      \\' - single quote   \\" - double quote
 *
 * Custom extensions:
 *   \\e - escape (0x1b)
 *   \\BNN - binary (0b prefix, then digits)
 *   \\DNN - decimal (0d prefix, then digits)
 *
 * Line continuation:
 *   - Backslash-newline: Increment line number and retry from next line
 *   - Allows multi-line literals in source code
 *
 * Invalid characters:
 *   - Non-printable ASCII (< 0x20 or > 0x7e) replaced with space
 *   - Error reported via ER_C_BC
 *
 * Parameters:
 *   None (reads from curchar/nextchar)
 *
 * Returns:
 *   Unsigned char value (0-255) after escape processing
 *
 * Side effects:
 *   - Consumes character(s) from input stream
 *   - May increment lineno for backslash-newline
 *   - Reports errors for invalid characters
 */
/* escape char -> value lookup: index = char - 'a' for a-z */
static char escval[] = {
    0, '\b', 0, 0, '\x1b', '\f', 0, 0, 0, 0, 0, 0, 0,
    '\n', 0, 0, 0, '\r', 0, '\t', 0, '\v', 0, 0, 0, 0
};

static unsigned char termin;  /* String terminator (' or ") */

/*
 * Numeric escape in a string/char literal: one byte.  Out-of-range
 * values are diagnosed and masked so they can never collide with
 * the out-of-band GL_END marker.
 */
int
escint(unsigned char base)
{
    int v = (int)getint(base);

    if (v > 255) {
        gripe(ER_C_NX);
        v &= 0xff;
    }
    return v;
}

int
getlit()
{
    unsigned char c;
top:
    c = curchar;
    if (c != '\\') {
        if ((c < 0x20) || (c > 0x7e)) {
            gripe(ER_C_BC);
            c = curchar = ' ';
        }
    } else {
        advance();
        c = curchar;
        /* Backslash-newline continuation: skip and get next char.
         * Don't increment lineno here - advance() already did when it
         * processed the newline character.
         * If the next char after continuation is the string terminator,
         * return the out-of-band GL_END to signal end-of-content
         * (0xff is a valid byte value: '\377', '\xff'). */
        if (c == '\n') {
            advance();
            if (curchar == termin) return GL_END;
            goto top;
        }
        if (c >= '0' && c <= '7') return escint(8);
        if ((c | 0x20) == 'x') { advance(); return escint(16); }
        if (c == 'B') { advance(); return escint(2); }
        if (c == 'D') { advance(); return escint(10); }
        if (c >= 'a' && c <= 'z') {
            unsigned char v = escval[c - 'a'];
            if (v)
                c = v;
        }
    }
    advance();
    return c;
}

/*
 * Parse numeric constant (character or integer literal)
 *
 * Detects and parses character literals ('x') and integer constants
 * (decimal, octal, hex, binary) from the input stream. Stores result
 * in next.v.numeric for token processing.
 *
 * Character literals:
 *   - Format: 'x' or '\\n' (with escape sequences)
 *   - Processes escape codes via getlit()
 *   - Missing closing quote generates ER_C_CD error
 *
 * Integer literals:
 *   - Decimal: 123, 456
 *   - Hexadecimal: 0x10, 0xFF
 *   - Octal: 0755, 010
 *   - Binary: 0b1010
 *   - Long suffix: 123L, 0xFFL (suffix consumed but ignored)
 *
 * Base detection:
 *   - 0x/0X prefix -> hexadecimal (base 16)
 *   - 0b/0B prefix -> binary (base 2)
 *   - 0d/0D prefix -> decimal (base 10)
 *   - 0 prefix (no suffix) -> octal (base 8)
 *   - Otherwise -> decimal (base 10)
 *
 * Parameters:
 *   None (reads from character stream)
 *
 * Returns:
 *   1 if number parsed (value stored in next.v.numeric), 0 if not a number
 *
 * Side effects:
 *   - Consumes numeric characters from input
 *   - Sets next.v.numeric to parsed value
 *   - May report error for malformed character literal
 */
char
isnumber()
{
    unsigned char base;
    unsigned char c;
    unsigned char nc;
    char *p;

    if (charmatch('\'')) {
        termin = '\'';  /* Tell getlit() what terminates this literal */
        next.v.numeric = getlit();
        if (next.v.numeric == GL_END || curchar != '\'') {
            gripe(ER_C_CD);
        }
        advance();
        return 1;
    }

    /* Check for float starting with . (e.g., .5) */
    if (curchar == '.' && nextchar >= '0' && nextchar <= '9') {
        p = strbuf;
        *p++ = '0';  /* Prepend 0 for atof */
        *p++ = '.';
        advance();
        while ((c = curchar) >= '0' && c <= '9') {
            *p++ = c;
            advance();
        }
        c = curchar | 0x20;
        if (c == 'e') {
            *p++ = curchar;
            advance();
            if (curchar == '+' || curchar == '-') {
                *p++ = curchar;
                advance();
            }
            while ((c = curchar) >= '0' && c <= '9') {
                *p++ = c;
                advance();
            }
        }
        *p = '\0';
        gripe(ER_C_CD);
        next.v.numeric = 0;
        c = curchar | 0x20;
        if (c == 'f' || c == 'l') {
            advance();
        }
        return 2;
    }

    if ((curchar < '0') || (curchar > '9')) {
        return 0;
    }

    base = 10;
    if (charmatch('0')) {
        c = curchar | 0x20;
        if (c == 'x') {
            base = 16;
            advance();
        } else if (c == 'b') {
            base = 2;
            advance();
        } else if (c == 'd') {
            base = 10;
        } else {
            base = 8;
        }
    }
    next.v.numeric = getint(base);

    /* Check for float literal (decimal point or exponent) */
    /* Treat . as float if followed by: digit, e/E, or non-identifier char */
    /* This allows: 1.5, 1.e5, 1. but NOT 1.foo (member access) */
    c = curchar | 0x20;
	nc = nextchar | 0x20;
	if (base == 10 && ((curchar == '.' && (
			(nextchar >= '0' && nextchar <= '9') ||
			nc == 'e' ||
			!((nc >= 'a' && nc <= 'z') || nextchar == '_')))
				   || c == 'e')) {

		/* Build float string: integer part + optional . + frac + optional exp */
		p = fmtlong(strbuf, next.v.numeric);
		if (curchar == '.') {
			*p++ = '.';
			advance();
			while ((c = curchar) >= '0' && c <= '9') {
				*p++ = c;
				advance();
			}
		}
		c = curchar | 0x20;
		if (c == 'e') {
			*p++ = curchar;
			advance();
			if (curchar == '+' || curchar == '-') {
				*p++ = curchar;
				advance();
			}
			while ((c = curchar) >= '0' && c <= '9') {
				*p++ = c;
				advance();
			}
		}
		*p = '\0';
		gripe(ER_C_CD);
		next.v.numeric = 0;
		/* Skip optional f/F/l/L suffix */
		c = curchar | 0x20;
		if (c == 'f' || c == 'l') {
			advance();
		}
		return 2;  /* Return 2 for float */
	}

    /*
     * An L suffix is what makes a constant a long, and the only thing
     * that can: everything downstream sizes a constant by how big it
     * is, so "5L" was a byte and passing one to a function put two
     * bytes on the stack where the callee read four.  LNUMBER has been
     * in the lexeme set and in pass1's reader all along, waiting for
     * something to produce it.
     */
    if (c == 'l') {
        advance();
        return 3;
    }

    return 1;
}

/*
 * Parse C identifier into string buffer
 *
 * Detects and extracts C identifiers (keywords and symbols) following the
 * standard identifier rules: [A-Za-z_][A-Za-z0-9_]*
 *
 * Identifier rules:
 *   - First character: letter (a-z, A-Z) or underscore (_)
 *   - Subsequent characters: letter, digit (0-9), or underscore
 *   - No length limit (limited by STRBUFSIZE)
 *
 * Lookahead behavior:
 *   - IMPORTANT: Does NOT advance past last character
 *   - Leaves curchar at last identifier character
 *   - Leaves nextchar at first non-identifier character
 *   - Caller must call advance() to move past identifier
 *
 * This unusual convention allows the caller to inspect the character
 * immediately following the identifier (for macro detection, etc.) before
 * consuming it.
 *
 * Buffer usage:
 *   - Writes to global strbuf
 *   - Null-terminates result
 *   - No overflow checking (assumes STRBUFSIZE sufficient)
 *
 * Parameters:
 *   None (reads from curchar/nextchar, writes to strbuf)
 *
 * Returns:
 *   1 if identifier found (result in strbuf), 0 if curchar not identifier start
 *
 * Side effects:
 *   - Advances curchar to last identifier character (NOT past it)
 *   - Writes identifier to strbuf with null terminator
 */
char
issym()
{
    char *s = strbuf;

    /* if not a symbol starter */
    if (!(((curchar >= 'a') && (curchar <= 'z')) || 
          ((curchar >= 'A') && (curchar <= 'Z')) ||
          (curchar == '_'))) {
        return 0;
    }

    while (1) {
        *s++ = curchar;
        *s = 0;

        if (!((nextchar >= 'A' && nextchar <= 'Z') ||
            (nextchar >= 'a' && nextchar <= 'z') ||
            (nextchar >= '0' && nextchar <= '9') ||
            (nextchar == '_'))) {
            break;
        }
        advance();
    }

    /* Check identifier length limit (14 chars + leading underscore = 15 total) */
    if ((s - strbuf) > 14) {
        gripe(ER_W_SYMTRUNC);
    }

#ifdef DEBUG
    if (VERBOSE(V_SYM)) {
        fdprintf(2,"issym = %s curchar = %c nextchar = %c\n",
            strbuf, curchar, nextchar);
    }
#endif
    return 1;
}

/*
 * Process preprocessor directive
 *
 * Handles all C preprocessor directives (#define, #if, #include, etc.)
 * including conditional compilation with nesting support. This is the
 * core preprocessor implementation.
 *
 * Directives supported:
 *   #define NAME [value]     - Define macro (object-like or function-like)
 *   #undef NAME              - Remove macro definition
 *   #include "file" or <file> - Insert file contents
 *   #if expr                 - Start conditional block (nests)
 *   #ifdef NAME              - True if macro defined
 *   #ifndef NAME             - True if macro NOT defined
 *   #elif expr               - Else-if in conditional
 *   #else                    - Else in conditional
 *   #endif                   - End conditional block
 *
 * Conditional compilation stack:
 *   - Maintains linked list of struct cond
 *   - Each #if/#ifdef/#ifndef pushes new level
 *   - #endif pops level
 *   - Flags track state: C_TRUE (active), C_ELSESEEN, C_TRUESEEN
 *
 * State tracking:
 *   - C_TRUE: Current block is active (output tokens)
 *   - C_TRUESEEN: At least one branch was true (skips remaining branches)
 *   - C_ELSESEEN: #else seen (error if another #else encountered)
 *
 * Token skipping:
 *   - False blocks skip tokens via gettoken() loop
 *   - Nested conditionals handled correctly
 *   - #if/#elif in true blocks break to return next token
 *
 * #include handling:
 *   - Angle brackets <file> search system include path
 *   - Quotes "file" search user include paths
 *   - Calls insertfile() to push file on textbuf stack
 *   - Primes character stream after insertion
 *
 * Expression evaluation:
 *   - #if and #elif call readcppconst() to eval expression
 *   - ONELINE mode limits expression to single line
 *   - defined() pseudofunction supported
 *
 * Parameters:
 *   t - Preprocessor directive token type (IF, IFDEF, DEFINE, etc.)
 *
 * Side effects:
 *   - Modifies cond stack (pushes/pops levels)
 *   - Consumes tokens from input stream
 *   - May insert files or macro definitions
 *   - Updates C_TRUE/C_ELSESEEN/C_TRUESEEN flags
 */
/*
 * Push a new conditional scope onto cond stack with truth-value v.
 * Honors parent-false: if any enclosing #if is false, the new scope is
 * also false regardless of its own value.
 */
void
push_cond(unsigned long v)
{
    struct cond *c = (struct cond *)xalloc(sizeof(*c));
    c->next = cond;
    cond = c;

    /*
     * Inside a dead block, nothing this conditional says can bring
     * the text back.  Marking it merely false is not enough: #else
     * asks whether a branch has been taken yet, sees that none has,
     * and takes itself - so
     *
     *		#ifdef A
     *		...
     *		#else
     *		#ifdef B
     *		...
     *		#else
     *		   this came out		<- with A and B both defined
     *		#endif
     *		#endif
     *
     * emitted the innermost arm of a block that was skipped.  It is
     * the shape every three-way host/target guard has.  C_TRUESEEN
     * says "a branch has already been taken", which is what makes
     * #else and #elif both no-ops, and is the truth here: the arm
     * that was taken was in an enclosing conditional.
     */
    if (c->next && !(c->next->flags & C_TRUE)) {
        cond->flags = C_TRUESEEN;
        return;
    }
    cond->flags = (v ? (C_TRUE | C_TRUESEEN) : 0);
}

void
doCpp(unsigned char t)
{
    char *s;
    unsigned char k;
    struct cond *c;
    unsigned long v;


    switch (t) {
    case PP_IF:
        v = readcppconst();
        push_cond(v);
#ifdef DEBUG
        if (VERBOSE(V_CPP)) {
            fdprintf(2,"#if %d: cond->flags = 0x%02x (C_TRUE=%d)\n",
                v, cond->flags, !!(cond->flags & C_TRUE));
        }
#endif
        /*
         * Don't call skiptoeol() - readcppconst() in ONELINE mode
         * already advanced past the line
         */
        return;
    case PP_IFNDEF:
    case PP_IFDEF:
        skipws1();
        if (!issym()) {
            gripe(ER_C_MN);
            skiptoeol();
            return;
        }
        advance();
        v = mdefined(strbuf);  /* true if macro is defined */
        if (t == PP_IFNDEF) v = !v;    /* invert for ifndef */
        push_cond(v);
        skiptoeol();
        return;
    case PP_ENDIF:
        if (!(tflags & ONELINE)) {
            skiptoeol();
        }
#ifdef DEBUG
        if (VERBOSE(V_CPP)) {
            fdprintf(2,"#endif: cond=%p", cond);
            if (cond) {
                fdprintf(2," flags=0x%02x (C_TRUE=%d)",
                    cond->flags, !!(cond->flags & C_TRUE));
            }
            fdprintf(2,"\n");
        }
#endif
        if (!cond) {
            gripe(ER_C_CU);
            return;
        }
        c = cond;
        cond = c->next;
        free(c);
#ifdef DEBUG
        if (VERBOSE(V_CPP)) {
            fdprintf(2,"After pop: cond=%p", cond);
            if (cond) {
                fdprintf(2," flags=0x%02x (C_TRUE=%d)",
                    cond->flags, !!(cond->flags & C_TRUE));
            }
            fdprintf(2,"\n");
        }
#endif
        return;
    case PP_ELSE:
        if (!(tflags & ONELINE)) {
            skiptoeol();
        }
        if (!cond) {
            gripe(ER_C_CU);
            return;
        }
        if (cond->flags & C_ELSESEEN) {
            gripe(ER_C_ME);
            return;
        }
        cond->flags |= C_ELSESEEN;  /* Mark that we've seen #else */
        if (cond->flags & C_TRUESEEN) {
            /* Already had a true condition, #else block should be false */
            cond->flags &= ~C_TRUE;
        } else {
            /* Haven't had a true condition yet, #else block should be true */
            cond->flags |= (C_TRUE | C_TRUESEEN);
        }
        return;
    case PP_ELIF:
        if (!cond) {
            skiptoeol();
            gripe(ER_C_CU);
            return;
        }
        v = readcppconst();
        if (cond->flags & C_ELSESEEN) {
            gripe(ER_C_ME);
            return;
        }
        if (cond->flags & C_TRUESEEN) {
            cond->flags ^= C_TRUE;
        } else {
            cond->flags |= (v ? (C_TRUE | C_TRUESEEN) : 0);
        }
        /*
         * Don't call skiptoeol() - readcppconst() in ONELINE mode
         * already advanced past the line
         */
        return;
    case PP_DEFINE:
        skipws1();
        if (!issym()) {
            gripe(ER_C_MN);
            return;
        }
        advance();
        macdefine(strbuf);
        return;
    case PP_UNDEF:
        skipws1();
        if (!issym()) {
            gripe(ER_C_MN);
            return;
        }
        advance();
        macundefine(strbuf);
        return;
    case PP_INCLUDE:
#ifdef DEBUG
        if (VERBOSE(V_CPP)) {
            fdprintf(2,"Processing INCLUDE directive\n");
        }
#endif
        skipws1();
        if (curchar == '<') {
            k = '>';
        } else if (curchar == '\"') {
            k = '\"';
        } else {
            gripe(ER_C_ID);
            return;
        }
        advance();
        s = strbuf;
        while ((curchar != '\n') && (curchar != ' ') && (curchar != k)) {
            *s++ = curchar;
            advance();
        }
        *s = 0;
        if (curchar != k) {
            gripe(ER_C_ID);
        }
        skiptoeol();
#ifdef DEBUG
        if (VERBOSE(V_CPP)) {
            fdprintf(2,"After skiptoeol: curchar='%c'(0x%x) "
                "nextchar='%c'(0x%x)\n",
                curchar >= 32 ? curchar : '?', curchar,
                nextchar >= 32 ? nextchar : '?', nextchar);
            fdprintf(2,"About to insertfile: '%s' sys=%d\n", strbuf, k == '>');
        }
#endif
        insertfile(strbuf, k == '>');
        /* insertfile now initializes curchar/nextchar directly */
        return;
    }
}

/*
 * Parse string literal into bigbuf
 *
 * Appends to bigbuf starting at bigbuflen position, supporting
 * string concatenation. Length tracked in bigbuflen (2-byte).
 *
 * Returns:
 *   1 if string parsed (appended to bigbuf), 0 if curchar not '\"'
 */
char
isstring()
{
	int c;

    if (!charmatch('\"')) {
        return 0;
    }
    termin = '"';  /* Tell getlit() what terminates this string */
    while (!charmatch('\"')) {
        c = getlit();
        if (c == GL_END) {
            /* Backslash-newline before terminator - consume the quote */
            advance();
            break;
        }
        if (bigbuflen >= BIGBUFSIZE - 1) {
            error("string too long");
            /* consume rest of string to avoid cascading errors */
            while (!charmatch('\"'))
                getlit();
            break;
        }
        bigbuf[bigbuflen++] = c;
    }
    bigbuf[bigbuflen] = 0;
#ifdef DEBUG
    if (VERBOSE(V_STR)) {
        fdprintf(2,"isstring: %s(%d)\n", bigbuf, bigbuflen);
    }
#endif
    return 1;
}

/*
 * character to token translation for single char tokens
 * simpleChars has the characters, simpleToks has their token values
 */
char simpleChars[] = "{},[]();=.+-/*%&|^<>!~?:";
char simpleToks[] = {
    BEGIN, END, COMMA, LBRACK, RBRACK, LPAR, RPAR, SEMI,
    ASSIGN, DOT, PLUS, MINUS, DIV, STAR, MOD, AND, OR, XOR,
    LT, GT, BANG, TWIDDLE, QUES, COLON, 0
};

/*
 * list of characters that can be doubled, and the resulting token
 */
char dblChars[] = "+-|&=><";
char dbltok[] = {
    INCR, DECR, LOR, LAND, EQ, RSHIFT, LSHIFT, 0
};

/*
 * list of characters that can have '=' appended
 * and then, what token that turns them into
 */
char eqChars[] = "+-*/%&|^><!";
char eqtok[] = {
    PLUSEQ, SUBEQ, MULTEQ, DIVEQ, MODEQ, ANDEQ, OREQ, XOREQ,
    GE, LE, NEQ, 0
};

/*
 * Free memory allocated for current token
 *
 * Releases heap-allocated memory associated with the current token before
 * advancing to the next token. Prevents memory leaks during tokenization.
 *
 * Token memory management:
 *   - SYM tokens: Allocate name string (freed here)
 *   - STRING tokens: NOT freed (persist throughout compilation)
 *   - Other tokens: No heap allocation (nothing to free)
 *
 * STRING token persistence:
 *   - String literals referenced in expressions and initializers
 *   - Must survive entire compilation
 *   - Stored as synthetic global variables (_str0, _str1, etc.)
 *   - Freed at end of compilation (not per-token)
 *
 * Double-free prevention:
 *   - Sets cur.v.name to NULL after freeing
 *   - Prevents crashes if freetoken() called twice
 *
 * Side effects:
 *   - Frees cur.v.name if cur.type == SYM
 *   - Sets cur.v.name to NULL
 */
void
freetoken()
{
    /* SYM names are interned (pool-owned) - nothing to free here.
     * STRING memory persists; filter buffers may still reference it. */
}

/*
 * Get next token from input stream
 *
 * This is the main lexer/tokenizer function that implements the complete
 * C tokenization process including preprocessor integration, comment
 * handling, and operator recognition. It maintains a two-token lookahead
 * (cur and next) required for recursive descent parsing.
 *
 * Token stream management:
 *   - Shifts next -> cur
 *   - Parses new token into next
 *   - Maintains token history for debugging (tokHist circular buffer)
 *
 * Preprocessing integration:
 *   - Comment stripping (C and C++ style)
 *   - CPP directive processing (define, if, include, etc.)
 *   - Macro expansion via macexpand()
 *   - Conditional compilation (skips false blocks)
 *
 * Token types recognized:
 *   - Identifiers/keywords (via issym() + kwlook())
 *   - Numbers (via isnumber() - decimal, hex, octal, binary, char)
 *   - String literals (via isstring())
 *   - Operators (single-char, doubled, with '=' suffix)
 *   - Special operators: ->, punctuation
 *
 * Operator tokenization:
 *   - Single-char operators: + - * / % & | ^ < > ! ~ ? : = . , ; braces brackets parens
 *   - Doubled operators: ++ -- || && == >> << (same char twice)
 *   - Equals-suffix: += -= *= /= %= &= |= ^= >= <= != >>= <<= ||= &&=
 *   - Arrow operator: ->
 *
 * Preprocessor directive handling:
 *   - Hash at column 0 triggers CPP processing
 *   - Non-column-0 hash treated as token (for stringify in macros)
 *   - Directives processed by doCpp()
 *   - False blocks skip all tokens except nested directives
 *
 * Comment handling:
 *   - C-style: block comments (may nest, handled as single comment)
 *   - C++ style: line comments to end of line
 *   - Tracked via incomment flag across gettoken() calls
 *   - Comments invisible to parser
 *
 * Macro expansion:
 *   - Checks identifiers via macexpand()
 *   - Expands and re-tokenizes replacement text
 *   - Handles function-like macros with arguments
 *   - defined() pseudofunction in conditional expressions
 *
 * ONELINE mode:
 *   - Used for conditional expression evaluation
 *   - Translates newline to ';' to terminate expression
 *   - Enabled via tflags & ONELINE
 *
 * Output for preprocessor (-E flag):
 *   - Calls outputToken() to emit token text
 *   - Writes to .i file and/or asm capture buffer
 *   - Preserves spacing and formatting
 *
 * Asm block support:
 *   - Captures tokens to asmCbuf when active
 *   - Converts newlines to semicolons for asm statements
 *   - Skips braces (parsing markers, not asm text)
 *
 * EOF handling:
 *   - Returns E_O_F token when curchar == 0
 *   - Checks for unclosed conditional directives
 *
 * Error recovery:
 *   - Invalid tokens converted to ';' with ER_C_UT error
 *   - Continues parsing after errors
 *
 * Side effects:
 *   - Advances cur and next tokens
 *   - Consumes characters from input stream
 *   - May expand macros, process directives, include files
 *   - Updates incomment, lineend, tflags
 *   - Writes to preprocessor output and/or asm buffer
 */
/* Track when newline encountered for asm capture semicolon insertion */
unsigned char lineend = 0;

void
gettoken()
{
    token_t t;
    register unsigned char c;

    freetoken();

    memcpy(&cur, &next, sizeof(cur));
    /* Track what just became cur for label detection on NEXT token */
    prevTokType = cur.type;
    next.v.str = 0;
    next.type = NONE;

    /* Emit SEMI after ASMSTR */
    if (pendingSemi) {
        pendingSemi = 0;
        next.type = SEMI;
        return;
    }

    /* If pending asm text, return it as ASMSTR token */
    if (pendingAsm) {
        next.type = ASMSTR;
        /*
         * A copy, not the buffer.  This lexer reads one token ahead,
         * so with a pointer into bigbuf a function holding two asm
         * blocks emitted the second one's text for the first and the
         * third one's for both of the others - the buffer had already
         * been refilled by the lookahead before pass2 ever saw the
         * statement.  One block per function hid it, which is all
         * there was until now.
         */
        next.v.name = permdup(pendingAsm);
        pendingAsm = NULL;
        pendingSemi = 1;  /* emit SEMI on next call */
        return;
    }

    while (1) {
        c = curchar;
        if (c == 0) {
            next.type = E_O_F;
#ifdef DEBUG
            if (VERBOSE(V_CPP)) {
                fdprintf(2,"Reached EOF: cond=%p\n", cond);
                if (cond) {
                    fdprintf(2,"  cond->flags=0x%02x (C_TRUE=%d)\n",
                        cond->flags, !!(cond->flags & C_TRUE));
                }
            }
#endif
            break;
        }
        /* Handle comments before checking for preprocessor directives */
        if (c == '/') {
            if (nextchar == '*') {
                /*
                 * Always enter comment mode, even if already in one
                 * (nested comments become single comment)
                 */
#ifdef DEBUG
                if (VERBOSE(V_TOKEN)) {
                    fdprintf(2,"Found comment start at line %d\n", lineno);
                }
#endif
                incomment = 1;
                advance();
                advance();
                continue;
            }
        }
        if (!incomment && (c == '/') && (nextchar == '/')) {
            skiptoeol();
            continue;
        }
        if ((incomment) && (c == '*') && (nextchar == '/')) {
#ifdef DEBUG
            if (VERBOSE(V_TOKEN)) {
                fdprintf(2,
                    "Found comment end at line %d, advancing past */ \n",
                    lineno);
            }
#endif
            incomment = 0;
            advance();
            advance();
            continue;
        }
        if (incomment) {
            advance();
            continue;
        }
        /* Now safe to check for # - we know we're not in a comment */
        if (c == '#') {
            advance();
#ifdef DEBUG
            if (VERBOSE(V_CPP)) {
                fdprintf(2,"Found # at column=%d (will%s process) cond=%p",
                    column, (column == 1) ? "" : " NOT", cond);
                if (cond) {
                    fdprintf(2," C_TRUE=%d", !!(cond->flags & C_TRUE));
                }
                fdprintf(2,"\n");
            }
#endif
            if (column != 1) {
                /* Not a CPP directive, treat as token */
                next.type = '#';
                break;
            }
            /* CPP directive at column 0 */
            skipws1();
            if (issym()) {
                t = kwlook((unsigned char *)strbuf, cppkw);
#ifdef DEBUG
                if (VERBOSE(V_CPP)) {
                    fdprintf(2,"CPP keyword: '%s' -> %d\n", strbuf, t);
                }
#endif
                if (t != 0xff) {
                    advance();
                    /*
                     * In false block, only process conditional directives.
                     * Skip #include, #define, #undef, etc.
                     */
                    if (cond && !(cond->flags & C_TRUE)) {
                        if (t != PP_IF && t != PP_IFDEF && t != PP_IFNDEF &&
                            t != PP_ELIF && t != PP_ELSE && t != PP_ENDIF) {
                            skiptoeol();
                            continue;
                        }
                    }
#ifdef DEBUG
                    if (VERBOSE(V_CPP) && (t == PP_IF || t == PP_ELIF)) {
                        fdprintf(2,
                            "Before doCpp(%s): cur.type=0x%02x "
                            "next.type=0x%02x\n",
                            t == PP_IF ? "IF" : "ELIF", cur.type, next.type);
                    }
#endif
                    doCpp(t);
#ifdef DEBUG
                    if (VERBOSE(V_CPP) && (t == PP_IF || t == PP_ELIF)) {
                        fdprintf(2,
                            "After doCpp(%s): cur.type=0x%02x "
                            "next.type=0x%02x cond=%p\n",
                            t == PP_IF ? "IF" : "ELIF", cur.type, next.type,
                            cond);
                        if (cond) {
                            fdprintf(2,"  cond->flags=0x%02x (C_TRUE=%d)\n",
                                cond->flags, !!(cond->flags & C_TRUE));
                        }
                    }
#endif
                    /*
                     * After processing any CPP directive, continue looping
                     * to get the next real token.
                     */
                    continue;
                }
                gripe(ER_C_BD);
            }
            if (isnumber()) {
                lineno = next.v.numeric;
                skiptoeol();
                continue;
            }
        }
        c = curchar;	/* the # arm may have moved the stream */
        if (c == '\n') {
            lineend = 1;
        }
        if ((tflags & ONELINE) && (c == '\n')) {
            next.type = SEMI;
            /*
             * Don't advance - leave curchar at newline so next
             * gettoken() stops
             */
            break;
        }
        if (!(tflags & ONELINE) && cond && !(cond->flags & C_TRUE) &&
            c != '#') {
#ifdef DEBUG
            if (VERBOSE(V_CPP)) {
                fdprintf(2,
                    "Skipping line in false block: curchar=0x%02x ('%c')\n",
                    curchar,
                    (curchar >= ' ' && curchar < 127) ? curchar : '?');
            }
#endif
            skiptoeol();
            if (curchar == '\n') {
                advance();  /* consume the newline */
            }
            continue;
        }
        if ((c == ' ') || (c == '\t') || (c == '\n')) {
            advance();
#ifdef DEBUG
            if (VERBOSE(V_CPP) && cond) {
                fdprintf(2,
                    "After advance past whitespace: curchar=0x%02x "
                    "('%c') column=%d\n", curchar,
                    (curchar >= ' ' && curchar < 127) ? curchar : '?',
                    column);
            }
#endif
            /*
             * After advancing past whitespace, check if we should
             * skip rest of line
             */
            if (cond && curchar != '#' && curchar != 0) {
                if (!(cond->flags & C_TRUE)) {
                    skiptoeol();
                    if (curchar == '\n') {
                        advance();  /* consume the newline */
                    }
                }
            }
            continue;
        }
        /* Capture line number and filename at start of token */
        next.lineno = lineno;
        next.filename = filename;
        if (issym()) {
            /* Check for defined() pseudofunction in #if expressions */
            if (cpppseudofunc()) {
                /*
                 * cpppseudofunc() replaced the function with
                 * '0' or '1'
                 */
                continue;
            }
            if (macexpand(strbuf)) {
                continue;
            }
            advance();
            t = kwlook((unsigned char *)strbuf, ckw);
            if (t != 0xff) {
                next.type = t;  /* keyword token directly */
                /* Special handling for asm { } - capture raw text */
                if (t == ASM) {
                    /* Skip whitespace to find { */
                    skipallws();
                    if (curchar == '{') {
                        int depth = 1;
                        char *p;
                        char asmbig = 0;
                        bigbuflen = 0;
                        advance();  /* skip { */
                        /* Capture until matching } */
                        while (depth > 0 && (c = curchar)) {
                            if (c == '{') depth++;
                            else if (c == '}') {
                                depth--;
                                if (depth == 0) break;
                            }
                            if (bigbuflen < BIGBUFSIZE - 1)
                                bigbuf[bigbuflen++] = c;
                            else
                                asmbig = 1;
                            advance();
                        }
                        bigbuf[bigbuflen] = 0;
                        if (curchar == '}') advance();
                        /* Trim trailing whitespace */
                        while (bigbuflen > 0 && (bigbuf[bigbuflen-1] == ' ' ||
                               bigbuf[bigbuflen-1] == '\t' ||
                               bigbuf[bigbuflen-1] == '\n'))
                            bigbuf[--bigbuflen] = 0;
                        /* Trim leading whitespace */
                        p = bigbuf;
                        while (*p == ' ' || *p == '\t' || *p == '\n') p++;
                        if (p != bigbuf) {
                            bigbuflen = strlen(p);
                            memcpy(bigbuf, p, bigbuflen + 1);
                        }
                        /*
                         * Saying so, rather than assembling whatever
                         * fell inside 512 bytes.  It truncated mid
                         * instruction and the assembler then failed
                         * on a label it could not see the definition
                         * of, which says nothing about the cause.
                         */
                        if (asmbig)
                            error("asm block too long (limit 511)");
                        /* Point to bigbuf for next token */
                        pendingAsm = bigbuf;
                    }
                }
                break;
            }
            /*
             * Check for label: symbol followed by : (with optional whitespace).
             * Labels only valid at statement start - after SEMI, BEGIN, COLON,
             * LABEL, or at start. Not valid after ARROW, DOT, operators, etc.
             */
            {
                char is_label = 0;
                char can_be_label = (prevTokType == SEMI || prevTokType == BEGIN ||
                                   prevTokType == COLON || prevTokType == LABEL ||
                                   prevTokType == 0 || prevTokType == END);
                /* Skip whitespace to check for colon */
                skipws1();
                if (curchar == ':' && can_be_label) {
                    is_label = 1;
                    advance();  /* consume the colon */
                }
                next.type = is_label ? LABEL : SYM;
            }
            /* Enforce symbol length limit with warning */
            if (strlen(strbuf) > MAXSYMLEN) {
                gripe(ER_W_SYMTRUNC);
                strbuf[MAXSYMLEN] = '\0';
            }
            next.v.name = intern(strbuf);
            break;
        }
        {
            char numtype = isnumber();
            if (numtype) {
                /* isnumber() returns 2 for a float literal; it has
                 * already griped, so treat the zero it left as an
                 * ordinary number and keep parsing. */
                next.type = (numtype == 3) ? LNUMBER : NUMBER;
                break;
            }
        }
        /* String literal - uses bigbuf for concatenation */
        bigbuflen = 0;
        if (isstring()) {
            next.type = STRING;
            /* Concatenate adjacent string literals (C89/C90 feature) */
            /* Skip whitespace/comments and check for another string */
        concat:
            skipallws();
            /* Skip C-style comments */
            if (curchar == '/' && nextchar == '*') {
                advance(); advance();
                while (!(curchar == '*' && nextchar == '/')) {
                    if (curchar == 0) break;
                    advance();
                }
                advance(); advance();
                goto concat;
            }
            /* Skip C++ comments */
            if (curchar == '/' && nextchar == '/') {
                while (curchar != '\n' && curchar != 0)
                    advance();
                goto concat;
            }
            /* More strings to concatenate? isstring() appends to bigbuf */
            while (curchar == '"') {
                if (!isstring())
                    break;
                goto concat;
            }
            /* Copy result with 2-byte length prefix */
            next.v.str = xalloc(bigbuflen + 3);
            next.v.str[0] = bigbuflen & 0xff;
            next.v.str[1] = (bigbuflen >> 8) & 0xff;
            memcpy(next.v.str + 2, bigbuf, bigbuflen + 1);
            break;
        }

        /* from here, it had better be an operator */
        t = lookupc(simpleChars, curchar);
        if (t == 0xff) {
            gripe(ER_C_UT);
            curchar= ';';
            t = lookupc(simpleChars, ';');
        }

        next.type = simpleToks[t];
        c = curchar;	/* save what we saw */
        advance();

        /* check for ellipsis ... */
        if (c == '.' && curchar == '.') {
            advance();
            if (curchar == '.') {
                advance();
                next.type = ELLIPSIS;
            }
            /* else two dots is an error, but we'll let parser catch it */
        }

        /*
         * See if the character is doubled.  This can be an operator,
         * and if it is, the token is FINISHED - the checks below test
         * c, the first character, not the token built here, so
         * falling into them let a following character rewrite a
         * complete operator into a different one:
         *
         *      argc-->1        --  then >  became ->
         *      *p++=='.'       ++  then =  became +=
         *
         * Both are legal source that the PDP-11 compiler took, both
         * came out of porting 2.11BSD, and the parser received an
         * operator nobody had written.  >>= and <<= are the only
         * suffix a doubled operator takes, and that is settled here.
         */
        if (curchar == c) {
            t = lookupc(dblChars, c);
            if (t != 0xff) {
                next.type = dbltok[t];
                advance();
                /* check for >>= or <<= */
                if (curchar == '=' && (c == '>' || c == '<')) {
                    next.type = (c == '>') ? RSHIFTEQ : LSHIFTEQ;
                    advance();
                }
                break;
            }
        }

        /* see if the character has an '=' appended.  this can be an operator */
        if (curchar == '=') {
            t = lookupc(eqChars, c);
            if (t != 0xff) {
                next.type = eqtok[t];
                advance();
            }
        }
        if ((c == '-') && (curchar == '>')) {
            next.type = ARROW;
            advance();
        }
        break;
    }

    lineend = 0;
#ifdef DEBUG
    if (VERBOSE(V_TOKEN)) {
        fdprintf(2,"cur.type = 0x%02x \'%c\'\n", cur.type,
            cur.type > ' ' ? cur.type : ' ');
    }
#endif
    return;
}

/*
 * Handle defined() pseudofunction in preprocessor expressions
 *
 * Implements the defined() operator for #if and #elif directives. This
 * is a standard C preprocessor feature (though not in original K&R) that
 * tests if a macro name is currently defined.
 *
 * Syntax supported:
 *   defined(NAME)  - Returns 1 if NAME is defined, 0 otherwise
 *
 * Usage contexts:
 *   - #if defined(DEBUG) || defined(TRACE)
 *   - #elif defined(__GNUC__)
 *   - #if !defined(NDEBUG) && defined(CHECKS)
 *
 * Processing:
 *   1. Checks if identifier in strbuf is "defined"
 *   2. Verifies CPPFUNCS flag enabled (only in #if expressions)
 *   3. Expects '(' after "defined"
 *   4. Reads macro name identifier
 *   5. Expects ')' to close
 *   6. Looks up macro with maclookup()
 *   7. Replaces entire "defined(NAME)" with '0' or '1' in curchar
 *
 * Replacement mechanism:
 *   - Sets curchar to '0' or '1' character
 *   - Lexer continues and parses as number token
 *   - Expression evaluator sees 0 or 1 constant
 *
 * Error handling:
 *   - Missing '(': Reports ER_C_DP, returns '0'
 *   - Missing ')': Reports ER_C_DP, returns '0'
 *   - Not in CPPFUNCS mode: Returns 0 (not recognized)
 *
 * Parameters:
 *   None (reads from strbuf and character stream)
 *
 * Returns:
 *   1 if defined() pseudofunction processed (curchar set to '0' or '1')
 *   0 if not a defined() call
 *
 * Side effects:
 *   - Consumes "defined(NAME)" from input stream
 *   - Sets curchar to '0' or '1'
 */
char
cpppseudofunc()
{
    int r = 0;

    if ((strcmp("defined", strbuf) == 0) && (tflags & CPPFUNCS)) {
        advance();
        skipws1();
        if (curchar != '(') {
            gripe(ER_C_DP);
            curchar = '0';
            return 1;
        }
        advance();
        skipws1();
        if (issym()) {
            r = mdefined(strbuf);
            advance();
        }
        skipws1();
        if (curchar != ')') {
            gripe(ER_C_DP);
            r = 0;
        } else {
            advance();
        }
        curchar = r ? '1' : '0';
        return 1;
    }
    return 0;
}

/*
 * Evaluate constant expression for #if/#elif directive
 *
 * Parses and evaluates compile-time constant expressions in preprocessor
 * conditionals. Handles recursive #if nesting during expression evaluation
 * by carefully saving and restoring global state.
 *
 * Recursion challenge:
 *   - Called from doCpp() which is called from gettoken()
 *   - May call gettoken() internally to parse expression
 *   - Expression parsing may encounter #if and call doCpp() recursively
 *   - Must preserve cur/next/tflags across recursion
 *
 * State management:
 *   - Saves cur token (expression parser will modify it)
 *   - Sets ONELINE mode (newline terminates expression)
 *   - Enables CPPFUNCS (allows defined() pseudofunction)
 *   - Disables writeCppfile (don't output #if expression text)
 *   - Restores all state after evaluation
 *
 * ONELINE mode:
 *   - Translates newline to ';' token
 *   - Expression parser stops at ';'
 *   - Prevents #if expression from spanning multiple lines
 *
 * Token priming:
 *   - Calls gettoken() twice to fill cur and next
 *   - First call loads next
 *   - Second call shifts next->cur and loads new next
 *   - parseConst() then has proper two-token lookahead
 *
 * Expression evaluation:
 *   - Calls parseConst(SEMI) to parse until semicolon
 *   - Supports all C operators: arithmetic, logical, bitwise, relational
 *   - Constant folding performed during parse
 *   - Result is compile-time constant (unsigned long)
 *
 * defined() support:
 *   - CPPFUNCS flag enables cpppseudofunc()
 *   - Allows: #if defined(DEBUG) && !defined(NDEBUG)
 *
 * Parameters:
 *   None (reads from character stream)
 *
 * Returns:
 *   Unsigned long value of evaluated expression
 *
 * Side effects:
 *   - Consumes expression tokens from input
 *   - Temporarily modifies tflags, writeCppfile
 *   - Saves and restores cur token
 *   - Leaves curchar after expression
 *   - Clears lineend flag
 */
unsigned long
readcppconst()
{
    unsigned long val;
    char savedtflags = tflags;
    struct token saved_cur;

    memcpy(&saved_cur, &cur, sizeof(cur));
    /* SYM names are interned - pointer copy via memcpy is sufficient.
     * STRING memory needs a real clone (the freetoken call in inner gettoken
     * may reuse the buffer). */
    if (cur.type == STRING && cur.v.str) {
        int len = (unsigned char)cur.v.str[0] |
                  ((unsigned char)cur.v.str[1] << 8);
        saved_cur.v.str = xalloc(len + 3);
        memcpy(saved_cur.v.str, cur.v.str, len + 3);
    }

    tflags = ONELINE | CPPFUNCS;

    /* Skip whitespace before reading the first token */
    skipws1();

#ifdef DEBUG
    if (VERBOSE(V_CPP)) {
        fdprintf(2,
            "readcppconst: before gettoken: curchar=0x%02x ('%c')\n",
            curchar, (curchar >= ' ' && curchar < 127) ? curchar : '?');
    }
#endif

    /* Get the first token of the expression */
    gettoken();
#ifdef DEBUG
    if (VERBOSE(V_CPP)) {
        fdprintf(2,
            "After 1st gettoken: cur.type=0x%02x next.type=0x%02x "
            "curchar=0x%02x\n",
            cur.type, next.type, curchar);
    }
#endif
    gettoken();
#ifdef DEBUG
    if (VERBOSE(V_CPP)) {
        fdprintf(2,
            "After 2nd gettoken: cur.type=0x%02x next.type=0x%02x "
            "curchar=0x%02x\n",
            cur.type, next.type, curchar);
    }
#endif

    val = parseConst(SEMI);

#ifdef DEBUG
    if (VERBOSE(V_CPP)) {
        fdprintf(2,"After parseConst: curchar=0x%02x\n", curchar);
    }
#endif

    memcpy(&cur, &saved_cur, sizeof(cur));
    tflags = savedtflags;
    lineend = 0;

#ifdef DEBUG
    if (VERBOSE(V_CPP)) {
        fdprintf(2,
            "readcppconst returning: val=%lu curchar=0x%02x "
            "next.type=0x%02x\n",
            val, curchar, next.type);
    }
#endif

    return val;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
