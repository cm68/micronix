/*
 * AST Pretty Printer for ccc compiler - binary format
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexeme.h"

static unsigned char *data;
static int pos, len;
static int indent;
static int blockCnt;

static int cur(void) { return pos < len ? data[pos] : -1; }
static void advance(void) { if (pos < len) pos++; }
static void unread(void) { if (pos > 0) pos--; }

static int read1(void) {
    if (pos >= len) return 0;
    return data[pos++];
}

static int read2(void) {
    int lo = read1();
    int hi = read1();
    return lo | (hi << 8);
}

static long read4(void) {
    long v = read1();
    v |= (long)read1() << 8;
    v |= (long)read1() << 16;
    v |= (long)read1() << 24;
    return v;
}

static char nameBuf[256];
static char *readName(void) {
    int i, n = read1();
    for (i = 0; i < n && i < 255; i++) nameBuf[i] = read1();
    nameBuf[n < 255 ? n : 255] = 0;
    return nameBuf;
}

static void prIndent(void) {
    int i;
    for (i = 0; i < indent; i++) printf("  ");
}

static void prln(char *s) { prIndent(); puts(s); }

#include "format.h"
#include "astops.h"

/* opArity returns -1 for special cases handled separately in parseExpr */
/*
 * -1 means the opcode has its own record shape and is handled by its
 * own block above; everything else is one child or two, and which is
 * which comes from astBinary() in format.h - the same table pass2
 * reads the stream with.  This used to default to two, so any unary
 * operator the list had not been told about ate its sibling.
 */
static int opArityPP(int c) {
    if (c == PREINC || c == POSTINC || c == PREDEC || c == POSTDEC)
        return -1; /* inc/dec special */
    if (c == BFEXTRACT || c == BFASSIGN) return -1;
    if (c == CALL || c == QUES) return -1;
    return astBinary(c) ? 2 : 1;
}

#define EXPR_BUF_SIZE 4096
static char exprBuf[EXPR_BUF_SIZE];
static int exprPos;

static void exprReset(void) { exprPos = 0; exprBuf[0] = 0; }
static void exprApp(char *s) {
    while (*s && exprPos < EXPR_BUF_SIZE - 1) exprBuf[exprPos++] = *s++;
    exprBuf[exprPos] = 0;
}
static void exprAppNum(long n) {
    char buf[24];
    sprintf(buf, "%ld", n);
    exprApp(buf);
}

static void parseExpr(void);

static void parseExpr(void) {
    int c = cur();

    if (c == -1) { exprApp("(EOF)"); return; }
    if (c == AST_EMPTY) { advance(); exprApp("()"); return; }

    /* Symbol reference */
    if (c == SYM) {
        advance();
        exprApp("$");
        exprApp(readName());
        return;
    }

    /* Numeric constant */
    if (c == NUMBER) {
        advance();
        int w = read1();
        long v = read4();
        exprAppNum(v);
        exprApp(":");
        exprApp(widthName(w));
        return;
    }

    /* Inc/Dec */
    if (c == PREINC || c == POSTINC || c == PREDEC || c == POSTDEC) {
        char *opn = (c == PREINC) ? "PREINC" : (c == POSTINC) ? "POSTINC" : (c == PREDEC) ? "PREDEC" : "POSTDEC";
        advance();
        int w = read1();
        exprApp("(");
        exprApp(opn);
        exprApp(":");
        exprApp(widthName(w));
        exprApp(" ");
        parseExpr();
        int delta = read2();
        char buf[16];
        sprintf(buf, " %d)", delta);
        exprApp(buf);
        return;
    }

    /* Bitfield extract */
    if (c == BFEXTRACT) {
        advance();
        int off = read1();
        int wid = read1();
        exprApp("(BFEXT ");
        exprAppNum(off);
        exprApp(":");
        exprAppNum(wid);
        exprApp(" ");
        parseExpr();
        exprApp(")");
        return;
    }

    /* Bitfield assign */
    if (c == BFASSIGN) {
        advance();
        int off = read1();
        int wid = read1();
        exprApp("(BFSET ");
        exprAppNum(off);
        exprApp(":");
        exprAppNum(wid);
        exprApp(" ");
        parseExpr();
        exprApp(" ");
        parseExpr();
        exprApp(")");
        return;
    }

    /* Sign extend */
    if (c == SEXT) {
        advance();
        int w = read1();
        exprApp("(SEXT:");
        exprApp(widthName(w));
        exprApp(" ");
        parseExpr();
        exprApp(")");
        return;
    }

    /* Register variable (leaf node) */
    if (c == REGVAR) {
        advance();
        int w = read1();
        int reg = read1();
        exprApp("(REGVAR:");
        exprApp(widthName(w));
        exprApp(" ");
        exprApp(regName(reg));
        exprApp(")");
        return;
    }

    /* Local variable (leaf node) */
    if (c == LOCALVAR) {
        advance();
        int w = read1();
        /*
         * TWO bytes - emitExpr writes emit2 for a frame offset,
         * because a local below the callee-save slots passes -128.
         * Reading one printed the right number for a small offset
         * and left the high byte to be read as the NEXT opcode: a
         * parameter at IY+4 is "04 00", and the stray zero came back
         * as E_O_F in the middle of an expression.
         */
        int off = (short)read2();
        char buf[32];
        sprintf(buf, "(LOCALVAR:%s IY%+d)", widthName(w), off);
        exprApp(buf);
        return;
    }

    /* Regular operator */
    int opChar = c;
    advance();

    /* Call */
    if (opChar == CALL) {
        int i, retType = read1();
        int argc = read1();
        char buf[32];
        sprintf(buf, "(CALL:%c/%d ", retType, argc);
        exprApp(buf);
        parseExpr();
        for (i = 0; i < argc; i++) {
            exprApp(" ");
            parseExpr();
        }
        exprApp(")");
        return;
    }

    /* Ternary */
    if (opChar == QUES) {
        int w = read1();
        exprApp("(?:");
        exprApp(widthName(w));
        exprApp(" ");
        parseExpr();
        exprApp(" ");
        parseExpr();
        exprApp(" ");
        parseExpr();
        exprApp(")");
        return;
    }

    /* Regular operator with width */
    int w = read1();
    int arity = opArityPP(opChar);

    exprApp("(");
    /*
     * An opcode with no name is the interesting case, so say WHICH:
     * "???" alone means the reader has fallen behind and gives
     * nothing to chase it with.  The number is what identifies the
     * record shape that is missing.
     */
    if (opName(opChar)[0] == '?') {
        char ub[16];
        sprintf(ub, "?op%d?", opChar);
        exprApp(ub);
    } else {
        exprApp(opName(opChar));
    }
    exprApp(":");
    exprApp(widthName(w));
    exprApp(" ");
    parseExpr();
    if (arity == 2) {
        exprApp(" ");
        parseExpr();
    }
    exprApp(")");
}

static char *getExpr(void) {
    exprReset();
    parseExpr();
    return exprBuf;
}

static void parseStmt(void);

static void parseDecl(void) {
    if (cur() != AST_DECL) return;
    advance();
    int typeChar = read1();
    char *name = readName();
    char nameCopy[256];
    strcpy(nameCopy, name);
    int reg = read1();
    int off = read1();
    if (off > 127) off -= 256;

    prIndent();
    printf("DECL %s : %s reg=%s off=IY%+d\n", nameCopy, widthName(typeChar), regName(reg), off);
}

static void parseStmt(void) {
    int c = cur();
    if (c == -1) return;
    advance();

    static char lineBuf[EXPR_BUF_SIZE + 256];

    /* Block */
    if (c == AST_BLOCK) {
        int i, declCnt = read1();
        int stmtCnt = read1();
        int blockNum = blockCnt++;
        sprintf(lineBuf, "BLOCK %d (%d stmts) {", blockNum, stmtCnt);
        prln(lineBuf);
        indent++;
        for (i = 0; i < declCnt; i++) parseDecl();
        for (i = 0; i < stmtCnt; i++) parseStmt();
        indent--;
        prln("}");
        return;
    }

    /* If */
    if (c == IF) {
        int hasElse;
        int nlabels = read1();  /* nlabels comes first, used for code gen */
        char *cond = getExpr();
        sprintf(lineBuf, "IF [%d labels] (%s)", nlabels, cond);
        prln(lineBuf);
        indent++;
        parseStmt();
        indent--;
        hasElse = read1();  /* hasElse comes AFTER then block */
        if (hasElse == 1) {
            prln("ELSE");
            indent++;
            parseStmt();
            indent--;
        }
        return;
    }

    /* Return */
    if (c == RETURN) {
        int hasVal = read1();
        if (hasVal == 1) {
            char *e = getExpr();
            sprintf(lineBuf, "RETURN %s", e);
            prln(lineBuf);
        } else {
            prln("RETURN");
        }
        return;
    }

    /* Label */
    if (c == LABEL) {
        char *name = readName();
        sprintf(lineBuf, "LABEL %s:", name);
        prln(lineBuf);
        return;
    }

    /* Goto */
    if (c == GOTO) {
        char *name = readName();
        sprintf(lineBuf, "GOTO %s", name);
        prln(lineBuf);
        return;
    }

    /* Switch */
    if (c == SWITCH) {
        int i, hasLabel = read1();
        char labelBuf[256];
        int caseCnt;
        char *expr;
        labelBuf[0] = 0;
        if (hasLabel == 1) strcpy(labelBuf, readName());
        caseCnt = read1();
        expr = getExpr();
        if (labelBuf[0])
            sprintf(lineBuf, "SWITCH [%s] (%s) [%d cases] {", labelBuf, expr, caseCnt);
        else
            sprintf(lineBuf, "SWITCH (%s) [%d cases] {", expr, caseCnt);
        prln(lineBuf);
        indent++;
        for (i = 0; i < caseCnt; i++) parseStmt();
        indent--;
        prln("}");
        return;
    }

    /* Case */
    if (c == CASE) {
        int i, stmtCnt = read1();
        char *val = getExpr();
        sprintf(lineBuf, "CASE %s: [%d stmts]", val, stmtCnt);
        prln(lineBuf);
        indent++;
        for (i = 0; i < stmtCnt; i++) parseStmt();
        indent--;
        return;
    }

    /* Default */
    if (c == DEFAULT) {
        int i, stmtCnt = read1();
        sprintf(lineBuf, "DEFAULT: [%d stmts]", stmtCnt);
        prln(lineBuf);
        indent++;
        for (i = 0; i < stmtCnt; i++) parseStmt();
        indent--;
        return;
    }

    /* Asm */
    if (c == ASM) {
        int i, asmLen = read2();
        int hasNL = 0;
        char *asmBuf = malloc(asmLen + 1);
        for (i = 0; i < asmLen; i++) asmBuf[i] = read1();
        asmBuf[asmLen] = 0;

        /* Check for newlines */
        for (i = 0; i < asmLen; i++) if (asmBuf[i] == '\n') { hasNL = 1; break; }

        if (!hasNL) {
            sprintf(lineBuf, "ASM { %s }", asmBuf);
            prln(lineBuf);
        } else {
            prln("ASM {");
            indent++;
            char *p = asmBuf, *q;
            while ((q = strchr(p, '\n')) != NULL) {
                *q = 0;
                prln(p);
                p = q + 1;
            }
            if (*p) prln(p);
            indent--;
            prln("}");
        }
        free(asmBuf);
        return;
    }

    /* Empty statement */
    if (c == SEMI) { prln(";"); return; }

    /* Expression statement - unrecognized opcode is start of expression */
    unread();
    prln(getExpr());
}

static void parseInit(void);
static char initBuf[4096];
static int initPos;

static void initReset(void) { initPos = 0; initBuf[0] = 0; }
static void initApp(char *s) {
    while (*s && initPos < 4095) initBuf[initPos++] = *s++;
    initBuf[initPos] = 0;
}
static void initAppNum(long n) {
    char buf[24];
    sprintf(buf, "%ld", n);
    initApp(buf);
}

static void parseInit(void) {
    int c = cur();

    /* Array */
    if (c == LBRACK) {
        int i, count;
        advance();
        advance(); /* elem type */
        count = read1();
        initApp("[");
        for (i = 0; i < count; i++) {
            if (i > 0) initApp(", ");
            parseInit();
        }
        if (cur() == RBRACK) advance();
        initApp("]");
        return;
    }

    /* Aggregate */
    if (c == BEGIN) {
        int i, count;
        advance();
        count = read1();
        initApp("{");
        for (i = 0; i < count; i++) {
            if (i > 0) initApp(", ");
            parseInit();
        }
        if (cur() == END) advance();
        initApp("}");
        return;
    }

    /* Scalar */
    if (c == NUMBER) {
        advance();
        int w = read1();
        long val = read4();
        initAppNum(val);
        initApp(":");
        initApp(widthName(w));
        return;
    }

    /* Symbol */
    if (c == SYM) {
        advance();
        initApp("$");
        initApp(readName());
        return;
    }

    /* Widened */
    if (c == WIDEN) {
        advance();
        read1(); /* skip target type */
        initApp("(W ");
        parseInit();
        initApp(")");
        return;
    }

    /* ADD expression (for pointer arithmetic in initializers) */
    if (c == PLUS) {
        advance();
        read1(); /* skip type suffix */
        initApp("(");
        parseInit();
        initApp(" + ");
        parseInit();
        initApp(")");
        return;
    }

    /* Unknown */
    advance();
    initApp("?");
}

static char *getInit(void) {
    initReset();
    parseInit();
    return initBuf;
}

static void parseFunction(void) {
    int retType = read1();
    char *name = readName();
    char nameCopy[256];
    strcpy(nameCopy, name);

    int paramCnt = read1();
    int localCnt = read1();
    /*
     * frm_size is TWO bytes and frameSaveBase follows it - see
     * emitFunc() in pass1/outfn.c.  Reading one byte and skipping
     * the save base left this two bytes behind at the top of every
     * function, and a stream reader that is behind does not misprint
     * a field, it turns everything after into noise.
     */
    int frmSize = read2();
    (void)read1();		/* frameSaveBase - printed by nothing here */

    /* Params */
    char paramsBuf[1024];
    int i, plen = 0;
    paramsBuf[0] = 0;
    for (i = 0; i < paramCnt; i++) {
        int ptype, preg, poff;
        char *pname;
        char pnameCopy[256];
        if (cur() != AST_DECL) continue;
        advance();
        ptype = read1();
        pname = readName();
        strcpy(pnameCopy, pname);
        preg = read1();
        poff = read1();
        if (poff > 127) poff -= 256;

        if (plen > 0) { paramsBuf[plen++] = ','; paramsBuf[plen++] = ' '; }
        plen += sprintf(paramsBuf + plen, "%s:%s@", pnameCopy, widthName(ptype));
        if (preg)
            plen += sprintf(paramsBuf + plen, "%s", regName(preg));
        else
            plen += sprintf(paramsBuf + plen, "IY%+d", poff);
    }

    /* Locals */
    char localsBuf[1024];
    int llen = 0;
    localsBuf[0] = 0;
    for (i = 0; i < localCnt; i++) {
        int ltype, lreg, loff;
        char *lname;
        char lnameCopy[256];
        if (cur() != AST_DECL) continue;
        advance();
        ltype = read1();
        lname = readName();
        strcpy(lnameCopy, lname);
        lreg = read1();
        /*
         * Two bytes here and one for a parameter above: emitLocals()
         * writes emit2, emitParams() writes emit1.  The asymmetry is
         * real - a local can sit below the callee-save slots and pass
         * -128, a parameter cannot.
         */
        loff = (short)read2();

        if (llen > 0) { localsBuf[llen++] = ','; localsBuf[llen++] = ' '; }
        llen += sprintf(localsBuf + llen, "%s:%s@", lnameCopy, widthName(ltype));
        if (lreg)
            llen += sprintf(localsBuf + llen, "%s", regName(lreg));
        else
            llen += sprintf(localsBuf + llen, "IY%+d", loff);
    }

    printf("\nFUNCTION %s(%s) -> %s [frame=%d]\n", nameCopy, paramsBuf, widthName(retType), frmSize);
    if (localsBuf[0]) printf("  LOCALS: %s\n", localsBuf);
    puts("{");
    indent = 1;
    parseStmt();
    puts("}");
}

static void parseGlobal(void) {
    if (cur() == '$') advance();
    char *name = readName();
    char nameCopy[256];
    strcpy(nameCopy, name);
    int typeChar = cur(); advance();

    /* Array */
    if (typeChar == 'a') {
        int count = read2();
        int elemSize = read2();
        int hasInit = read1();
        printf("GLOBAL %s : array[%d] of %d-byte", nameCopy, count, elemSize);
        if (hasInit == 1) {
            char *init = getInit();
            printf(" = %s\n", init);
        } else {
            puts("");
        }
        return;
    }

    /* Struct */
    if (typeChar == 'r') {
        int size = read2();
        int hasInit = read1();
        printf("GLOBAL %s : struct[%d]", nameCopy, size);
        if (hasInit == 1) {
            char *init = getInit();
            printf(" = %s\n", init);
        } else {
            puts("");
        }
        return;
    }

    /* Pointer or primitive */
    int hasInit = read1();
    printf("GLOBAL %s : %s", nameCopy, widthName(typeChar));
    if (hasInit == 1) {
        char *init = getInit();
        printf(" = %s\n", init);
    } else {
        puts("");
    }
}

static void parseString(void) {
    int i, n;
    char *name = readName();
    char nameCopy[256];
    strcpy(nameCopy, name);
    n = read1();
    printf("STRING _%s = \"", nameCopy);
    for (i = 0; i < n; i++) {
        int b = read1();
        if (b >= 32 && b < 127 && b != '"' && b != '\\')
            putchar(b);
        else
            printf("\\x%02x", b);
    }
    puts("\"");
}

static int parseTopLevel(void) {
    int c = cur();
    if (c == -1) return 0;
    advance();

    if (c == AST_FUNC) parseFunction();
    else if (c == AST_GLOBAL) parseGlobal();
    else if (c == STRING) parseString();
    else if (c == ASM) {
        /* Global asm */
        int i, asmLen = read2();
        char *asmBuf = malloc(asmLen + 1);
        for (i = 0; i < asmLen; i++) asmBuf[i] = read1();
        asmBuf[asmLen] = 0;
        printf("ASM {\n%s\n}\n", asmBuf);
        free(asmBuf);
    }
    else printf("??? top-level: '%c' (0x%02x)\n", c, c);

    return 1;
}

int main(int argc, char **argv) {
    FILE *f;
    if (argc > 1) {
        f = fopen(argv[1], "rb");
        if (!f) { perror(argv[1]); return 1; }
    } else {
        f = stdin;
    }

    /* Read entire file */
    fseek(f, 0L, SEEK_END);
    len = ftell(f);
    fseek(f, 0L, SEEK_SET);
    if (len < 0) {
        /* stdin - read in chunks */
        int cap = 4096;
        data = malloc(cap);
        len = 0;
        int n;
        while ((n = fread(data + len, 1, cap - len, f)) > 0) {
            len += n;
            if (len == cap) {
                cap *= 2;
                data = realloc(data, cap);
            }
        }
    } else {
        data = malloc(len + 1);
        fread(data, 1, len, f);
    }
    if (f != stdin) fclose(f);

    puts("========================================");
    puts("AST Pretty Printer Output (binary)");
    puts("========================================");

    while (parseTopLevel())
        ;

    puts("");
    puts("========================================");

    free(data);
    return 0;
}
