/*
 * emit.c - Lexeme stream and preprocessed output emission
 *
 * Binary .x format - all data as raw bytes (no hex encoding)
 */
#include <stdlib.h>
#include <string.h>
#include "cpp.h"
#include <unistd.h>

/* Global file descriptor */
char lexFd = -1;

/* Line tracking for LINENO emission */
static int lastLine = 0;
static char *lastName = NULL;

/* Brace balance tracking */
static int braceCount = 0;

/* Forward declarations */
void emitLine(int line, char *file);
void emitStructTok(struct token *t);
void emit1tok(struct token *t);

/*
 * A string literal held back to see whether another one follows it.
 * pendstr is the counted buffer, owned here; pendtok carries the
 * position of the first literal, which is the one the run reports.
 */
static char *pendstr = NULL;
static struct token pendtok;

/*
 * Initialize line tracking and emit initial line directive for source file
 */
void
emitFileStart(char *file)
{
    if (!noLineMarkers) {
        emitLine(1, file);
        lastLine = 1;
        lastName = filename;
    }
}

/*
 * Emit a simple token to .x file (1 byte)
 */
void
emitToken(unsigned char tok)
{
    outbufWrite(&tok, 1);
}

/*
 * Emit symbol: SYMID(26) + 2-byte id, the name having gone to the
 * .n sidecar.  The SYM(20) name form is never emitted.
 */
void
emitId(unsigned char tok, char *name)
{
    unsigned char b[3];
    unsigned short id = idOf(name);

    b[0] = tok;
    b[1] = id & 0xff;
    b[2] = (id >> 8) & 0xff;
    outbufWrite(b, 3);
}

void
emitSym(char *name)
{
    emitId(SYMID, name);
}

/*
 * Emit 4-byte little-endian value with tag
 */
void
emit4(unsigned char tag, unsigned long val)
{
    unsigned char buf[5];

    buf[0] = tag;
    /*
     * A long goes into the stream laid out the way the machine lays
     * one down - high word first, each word little-endian within
     * itself - so the Z80 build stores it and the reader loads it and
     * neither shifts anything.  The stream used to be little-endian
     * by definition and both ends copied the bytes straight, which
     * was the same thing until the layout moved.  See QLONG.md, NUXI.
     */
#ifdef CCC
    *(unsigned long *)&buf[1] = val;
#else
    buf[1] = (val >> 16) & 0xff;
    buf[2] = (val >> 24) & 0xff;
    buf[3] = val & 0xff;
    buf[4] = (val >> 8) & 0xff;
#endif
    outbufWrite(buf, 5);
}

void
emitNumber(long val)
{
    emit4(NUMBER, (unsigned long)val);
}

/* the same, but the source said L and that is the only way to know */
void
emitLNumber(long val)
{
    emit4(LNUMBER, (unsigned long)val);
}

/*
 * Emit string with 2-byte length: token + 2-byte len + string bytes
 * Used for both STRING and ASMSTR tokens
 */
void
emitStr2(unsigned char tok, char *str, int len)
{
    unsigned char hdr[3];
    /*
     * Clamp to 32767, not 65535: the .x length field is 16 bits, but
     * 65535 is -1 in a 16-bit int, so that comparison fired for EVERY
     * string under the z80 compilers.  32767 is representable
     * everywhere; on the z80 the test is simply never true.
     */
    if (len > 32767) len = 32767;
    hdr[0] = tok;
    hdr[1] = len & 0xff;
    hdr[2] = (len >> 8) & 0xff;
    outbufWrite(hdr, 3);
    outbufWrite(str, len);
}

/*
 * Emit string: STRING(22) + 2-byte len + string bytes
 */
void
emitString(char *str, int len)
{
    emitStr2(STRING, str, len);
}

/*
 * Emit asm string: ASMSTR(118) + 2-byte len + string bytes
 *
 * Never more than 255 bytes to a record.  pass1 copies every counted
 * record with a byte counter - names and labels are counted in one
 * byte, strings are capped by its buffer - and asm text was the one
 * record that could outrun it, so the bound is enforced here where
 * the text is whole.  Records split at line boundaries: each slice
 * becomes its own asm statement on the far side, and since a slice
 * ends on a newline the assembly reads back identically.  The length
 * field stays two bytes, so the format itself is unchanged; only the
 * guarantee is new.  No block in the tree comes within a factor of
 * five of needing a split.
 */
void
emitAsmString(char *str, int len)
{
    int cut;

    while (len > 255) {
        for (cut = 255; cut > 0 && str[cut - 1] != '\n'; cut--)
            ;
        if (cut == 0)
            cut = 255;      /* one enormous line: split it anyway */
        emitStr2(ASMSTR, str, cut);
        str += cut;
        len -= cut;
    }
    emitStr2(ASMSTR, str, len);
}

/*
 * Emit label: LABEL(112) + len byte + name bytes
 */
void
emitLabel(char *name)
{
    emitId(LABELID, name);
}

/*
 * Emit newline marker to .x: single NEWLINE byte (means line++)
 */
void
emitNewline(void)
{
    unsigned char c = NEWLINE;
    outbufWrite(&c, 1);
}

/*
 * Emit line number with file to .x: LINENO(116) + 2-byte line + len byte + filename
 */
void
emitLine(int line, char *file)
{
    unsigned char hdr[4];
    int len = strlen(file);

    if (len > 255) len = 255;
    hdr[0] = LINENO;
    hdr[1] = line & 0xff;
    hdr[2] = (line >> 8) & 0xff;
    hdr[3] = len;
    outbufWrite(hdr, 4);
    outbufWrite(file, len);
}



/*
 * Send out the string literal being held, if there is one.
 */
void
emitflstr(void)
{
    if (!pendstr)
        return;
    pendtok.v.str = pendstr;
    pendstr = NULL;
    emit1tok(&pendtok);		/* frees the buffer for us */
}

/*
 * Emit a token from struct to .x stream (used by pull-based filter chain)
 *
 * Adjacent string literals are joined here rather than in the lexer.
 * The lexer works on characters and so cannot see two literals that
 * only became adjacent when a macro was expanded, which is most of
 * them in this tree - the pass2 rule table is built out of named
 * fragments written side by side.  C puts the concatenation after
 * expansion for exactly that reason, and this is the first point in
 * cpp that is after it.  So a string is held back until the token
 * after it arrives: another string extends it, anything else releases
 * it.
 */
void
emitStructTok(struct token *t)
{
    int oldlen, addlen;
    char *joined;

    if (t->type == STRING) {
        addlen = (unsigned char)t->v.str[0] |
                 ((unsigned char)t->v.str[1] << 8);
        if (!pendstr) {
            tokcpy(&pendtok, t);
            pendstr = t->v.str;
        } else {
            oldlen = (unsigned char)pendstr[0] |
                     ((unsigned char)pendstr[1] << 8);
            joined = xalloc(oldlen + addlen + 3);
            joined[0] = (oldlen + addlen) & 0xff;
            joined[1] = ((oldlen + addlen) >> 8) & 0xff;
            memcpy(joined + 2, pendstr + 2, oldlen);
            memcpy(joined + 2 + oldlen, t->v.str + 2, addlen);
            joined[2 + oldlen + addlen] = '\0';
            free(pendstr);
            free(t->v.str);
            pendstr = joined;
        }
        t->v.str = NULL;
        return;
    }

    emitflstr();
    emit1tok(t);
}

void
emit1tok(struct token *t)
{
    /* Emit line info to .x when line or file changes (unless -N) */
    if (!noLineMarkers) {
        if (lastName != t->filename) {
            /* File changed - emit full LINENO with filename */
            emitLine(t->lineno, t->filename ? t->filename : "");
            lastLine = t->lineno;
            lastName = t->filename;
        } else if (t->lineno == lastLine + 1) {
            /* Line incremented by 1 - emit single NEWLINE byte */
            emitNewline();
            lastLine = t->lineno;
        } else if (t->lineno != lastLine) {
            /* Line jumped - emit full LINENO */
            emitLine(t->lineno, t->filename ? t->filename : "");
            lastLine = t->lineno;
        }
    }

    /* Emit to lexeme stream */
    if (t->type >= KW_FIRST && t->type <= KW_LAST) {
        if (t->type == SIZEOF_KW)
            emitToken(SIZEOF);
        else if (t->type == CONST || t->type == VOLATILE)
            ;  /* Skip const/volatile - not supported */
        else
            emitToken(t->type);
    } else switch (t->type) {
    case SYM:
#ifdef DEBUG
        if (VERBOSE(V_FILTER))
            fdprintf(2, "emitStructTok SYM name=%s\n", t->v.name ? t->v.name : "(null)");
#endif
        emitSym(t->v.name);
        break;
    case NUMBER:
        emitNumber(t->v.numeric);
        break;
    case INUMBER:
        emit4(INUMBER, (unsigned long)t->v.numeric);
        break;
    case LNUMBER:
        emitLNumber(t->v.numeric);
        break;
    case STRING:
        {
            int len = (unsigned char)t->v.str[0] |
                      ((unsigned char)t->v.str[1] << 8);
            emitString(t->v.str + 2, len);
        }
        break;
    case ASMSTR:
        emitAsmString(t->v.name, strlen(t->v.name));
        break;
    case LABEL:
        emitLabel(t->v.name);
        break;
    case E_O_F:
        break;
    case BEGIN:
        braceCount++;
        emitToken(t->type);
        break;
    case END:
        braceCount--;
        emitToken(t->type);
        break;
    default:
        emitToken(t->type);
        break;
    }

    /* Free STRING memory after emission - only used once */
    if (t->type == STRING && t->v.str) {
        free(t->v.str);
        t->v.str = NULL;
    }
}

/*
 * Check brace balance at EOF - call before emitting E_O_F
 */
void
emitChkBraces(void)
{
    if (braceCount != 0) {
        char buf[64];
        fmtstr(buf, "cpp: brace mismatch at EOF: %d unmatched {\n",
               braceCount);
        write(2, buf, strlen(buf));
        exit(1);
    }
}
