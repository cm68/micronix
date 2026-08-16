/*
 * cpp - C Preprocessor
 *
 * Main driver for the preprocessor.
 * Produces <basename>.x - lexeme stream (compact token format)
 *
 * Uses lex.c for tokenization, io.c for file handling,
 * and macro.c for macro processing.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpp.h"
#include <fcntl.h>
#include <unistd.h>
#ifdef __GNUC__
#include <sys/wait.h>
#endif

#ifdef DEBUG
#include "dbgtags.c"
#endif

/* The normalizer: lexer in, pass1-ready lexeme stream out */
extern void norm_init(void);
extern void norm_run(void);

/* Global state */
char *curFile;
int lineNo;
char exitCode = 0;
char noLineMarkers = 0;  /* -N flag: suppress LINENO/NEWLINE in .x */
static char idFile[128]; /* the sidecar's name */
#ifdef DEBUG
short verbose;
#endif

/* Include path list */
#define MAX_INCLUDES 32
char *includePaths[MAX_INCLUDES];
unsigned char numIncludes = 0;

#ifdef CCC
/*
 * cpp does all io on raw file descriptors; without this stub, exit()
 * drags in the stdio flush machinery (fclose/fflush/buf + __sibuf),
 * about 1KB of text and bss we never use.
 */
void
_cleanup(void)
{
}
#endif

/*
 * Error reporting
 */
void
errout(char *buf)
{
    write(2, buf, strlen(buf));
}

int
opcreat(char *file)
{
    int fd = creat(file, 0644);
    if (fd < 0) {
        char buf[140];
        fmtstr(buf, "cannot create: %s\n", file);
        errout(buf);
        exit(1);
    }
    return fd;
}

void
error(char *msg)
{
    char buf[256];
    fmtstr(buf, "%s:%d: error: %s\n", filename ? filename : curFile, lineno, msg);
    errout(buf);
    exitCode = 1;
}

void
usage(void)
{
    errout("usage: cpp [options] <source.c>\n");
    errout("  -o <base>      Output base name (.x file)\n");
    errout("  -I<dir>        Add include directory\n");
    errout("  -i<dir>        System include directory\n");
    errout("  -D<name>[=val] Define macro\n");
    errout("  -E             Preprocess and dump to stdout (runs xdump)\n");
    errout("  -p             Also generate .i file (runs xdump)\n");
    errout("  -N             Suppress line markers\n");
    errout("  -h             Show this help\n");
#ifdef DEBUG
    errout("  -v <mask>      Set verbosity (hex bitmask)\n");
#ifdef __GNUC__
    {
        int i;
        for (i = 0; vopts[i]; i++) {
            fdprintf(2, "\t%x %s\n", 1 << i, vopts[i]);
        }
    }
#endif
#endif
    exit(1);
}

/*
 * Lexer wrapper for pull-based filter chain
 * Copies current token to output and advances lexer
 */
void
lex_get(struct token *out)
{
    tokcpy(out, &cur);
#ifdef DEBUG
    if (VERBOSE(V_FILTER))
        fdprintf(2, "lex_get: type=%d\n", out->type);
#endif
    gettoken();
}

/*
 * Initialize the filter pipeline
 */
void
filterInit(void)
{
    norm_init();
}

/*
 * Process source file - lex all tokens and emit to .x stream
 */
void
process(char *sourcefile)
{
    curFile = sourcefile;

    /* Push source file then initialize I/O (advance() needs tbtop) */
    pushfile(sourcefile);
    ioinit();

    /* Emit initial line directive for source file */
    emitFileStart(sourcefile);

    /* Prime the lexer - two calls needed to fill cur and next */
    gettoken();
    gettoken();

    /* The normalizer pulls the filter chain and pushes to emit */
    norm_run();

    /* A string literal at the very end is still being held */
    emitflstr();

    /* Check brace balance and emit EOF token */
    emitChkBraces();
    emitToken(E_O_F);
}

char lexFile[128];
#ifndef CPM
/*
 * The .i name.  Only -p and -E use it, and both of those hand over to
 * xdump, which cannot be reached on CP/M - there is no path to name it
 * by.  A hundred and twenty-eight bytes of bss for a filename nothing
 * reads is a hundred and twenty-eight bytes off the heap, and the heap
 * is what decides whether a source compiles on the 64K machine.
 */
char ppFile[128];
#endif

int
main(int argc, char **argv)
{
    char *source = NULL;
    char *outbase = NULL;
    register char *a;
    char **ap;
    int n;
    int i;
    int ppOnly = 0;
    int ppOutput = 0;

    /*
     * The target's name, predefined the way zc3 predefines it.  The
     * headers guard machine-specific shapes with "#if z80" - jmp_buf's
     * size, cpm.h wholesale - and under a cpp that says nothing about
     * its machine every one of those guards failed shut: setjmp.h
     * compiled to no typedef at all and the first jmp_buf was a
     * syntax error.  Costs one numeric-macro slot (two bytes).
     */
    addDefine("z80=1");

    /* Parse arguments */
    ap = argv + 1;
    n = argc - 1;
    while (n--) {
        a = *ap++;
        if (strcmp(a, "-o") == 0) {
            if (n == 0) usage();
            n--;
            outbase = *ap++;
        } else if (a[0] == '-' && a[1] == 'I') {
            /* Add to include path */
            if (numIncludes < MAX_INCLUDES)
                includePaths[numIncludes++] = a + 2;
        } else if (a[0] == '-' && a[1] == 'i') {
            /* System include path */
            sysIncPath = a + 2;
        } else if (a[0] == '-' && a[1] == 'D') {
            /* Define macro */
            addDefine(a + 2);
        } else if (strcmp(a, "-E") == 0) {
            ppOnly = 1;
        } else if (strcmp(a, "-p") == 0) {
            ppOutput = 1;
        } else if (strcmp(a, "-N") == 0) {
            noLineMarkers = 1;
        } else if (strcmp(a, "-h") == 0) {
            usage();
        } else if (strcmp(a, "-v") == 0) {
            if (n == 0) usage();
            n--;
#ifdef DEBUG
            verbose = strtol(*ap, 0, 0);
#endif
            ap++;
        } else if (a[0] == '-') {
            char buf[64];
            fmtstr(buf, "Unknown option: %s\n", a);
            errout(buf);
            usage();
        } else {
            source = a;
        }
    }

    if (!source) {
        errout("No source file specified\n");
        usage();
    }

    /* Derive output base from source if not specified */
    if (!outbase) {
        char *dot;
        outbase = permdup(source);
        dot = strrchr(outbase, '.');
        if (dot) *dot = '\0';
    }

    /* Create output file names */
    fmtstr(lexFile, "%s.x", outbase);
#ifndef CPM
    fmtstr(ppFile, "%s.tok", outbase);
#endif
    fmtstr(idFile, "%s.nam", outbase);

#ifdef DEBUG
#ifdef __GNUC__
    if (verbose) {
        int j = 0;

        for (i = 0; i < 32; i++) {
            if (!vopts[i])
                break;
            if (verbose & (1 << i))
                j |= (1 << i);
        }

        fdprintf(2, "verbose: %x (", j);
        for (i = 0; vopts[i]; i++) {
            if (j & (1 << i)) {
                fdprintf(2, "%s", vopts[i]);
                j ^= (1 << i);
                if (j) {
                    fdprintf(2, " ");
                }
            }
        }
        fdprintf(2, ")\n");
    }
#endif
#endif

    /* Open output file */
    lexFd = opcreat(lexFile);

    /* Add include paths - current directory first, then -I paths */
    addInclude("");  /* Current directory */
    ap = includePaths;
    n = numIncludes;
    while (n--)
        addInclude(*ap++);

    /* Initialize token filter */
    filterInit();

    /* Process the source file */
    process(source);
    /* the id-to-name sidecar, for c1 and the driver */
    internWrite(idFile);

    close(lexFd);

    /*
     * -p mode: write the readable .i beside the .x.
     *
     * The rendering lives in xdump rather than here, so that its
     * tables stay out of this image, and the host gets at it by
     * forking.  That is a host-only convenience: -p is for reading
     * what cpp did, it is not part of the compile the driver runs,
     * and a second process is not something to ask of the target.
     * CP/M has no fork at all, and on Micronix it would be spending
     * a process on a debugging aid.  Run xdump over the .x by hand
     * there - it is the same program with the same arguments.
     */
#ifdef __GNUC__
    if (ppOutput) {
        int pid = fork();
        if (pid == 0) {
            /* Child: exec xdump */
            if (noLineMarkers)
                execlp("xdump", "xdump", "-N", "-o", ppFile, lexFile, (char *)0);
            else
                execlp("xdump", "xdump", "-o", ppFile, lexFile, (char *)0);
            perror("xdump");
            _exit(1);
        } else if (pid > 0) {
            /* Parent: wait for xdump */
            int status;
            waitpid(pid, &status, 0);
            if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
                exitCode = 1;
        } else {
            perror("fork");
            exitCode = 1;
        }
    }
#else
    if (ppOutput) {
        errout("cpp: -p needs xdump run separately on this target\n");
        exitCode = 1;
    }
#endif

    /*
     * -E mode: hand over to xdump, which renders the .x as text.
     *
     * This is an exec and not a fork, so it costs no second process
     * and Micronix is happy to do it.  CP/M is not: chaining exists
     * there but "/bin/xdump" is not a name it can hold - there are no
     * directories - and pulling execl in for a path that cannot
     * resolve costs six hundred bytes of a TPA that has none to
     * spare.  Run xdump over the .x by hand there.
     */
    if (ppOnly) {
#ifdef CPM
        errout("cpp: -E needs xdump run separately on this target\n");
        return 1;
#else
#ifdef CCC
        if (noLineMarkers)
            execl("/bin/xdump", "xdump", "-N", lexFile, (char *)0);
        else
            execl("/bin/xdump", "xdump", lexFile, (char *)0);
#else
        if (noLineMarkers)
            execlp("xdump", "xdump", "-N", lexFile, (char *)0);
        else
            execlp("xdump", "xdump", lexFile, (char *)0);
#endif
        perror("xdump");
        return 1;
#endif
    }

#ifdef DEBUG
    /*
     * Behind a verbosity bit, not just DEBUG.  The host passes are
     * built -DDEBUG, so this fired on every compile the tree does and
     * on every compile anyone did with the installed driver - eighteen
     * lines of pool census in front of the diagnostics that say what
     * is actually wrong with the source.
     */
    if (VERBOSE(V_SYM))
        { extern void poolstats(void); poolstats(); }
#endif
    return exitCode;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
