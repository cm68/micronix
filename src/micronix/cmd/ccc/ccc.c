/*
 * ccc - C compiler driver
 *
 * Orchestrates cpp, c0 (parser), and c1 (code generator)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

/*
 * Thirteen arrays in main are MAX_ARGS pointers wide, so this number
 * is multiplied by twenty-six bytes of stack frame.  At 2560 that is
 * 66560 bytes - more than the Z80 has address space for - and the
 * frame size wrapped instead of being refused: main's frame came out
 * as 1154, and the offset of a local landed at 19374, which added to
 * IY put the address inside .text.  The driver then wrote its
 * argument vector over its own code.
 *
 * 128 is far more than a command line on this machine will ever
 * carry, and keeps the frame near three kilobytes.
 */
#define MAX_ARGS 128

char *progname;

/*
 * Where everything else lives, worked out from argv[0].
 *
 * There is no environment to read: v6 has none, so the usual INCDIR
 * or ROOTDIR hack cannot work on the machine this has to run on.
 * argv[0] is what the system does give us, and it is enough - strip
 * the program name to get the directory it was found in, and the
 * library sits beside that directory rather than under it:
 *
 *	/bin/ccc		->  /lib
 *	/usr/local/bin/ccc	->  /usr/local/lib
 *	desthost/bin/ccc	->  desthost/lib
 *	ccc			->  ./../lib
 *
 * which is v7's arrangement - /bin/cc finding /lib/c0 - and means an
 * installation can be moved anywhere without being rebuilt.
 *
 * A name with no slash in it is the awkward one.  argv[0] is what the
 * user typed, so "ccc hello.c" gives us "ccc" and nothing about where
 * it was found - the shell searched for it and did not say where it
 * looked.  There is no PATH to consult on the system this has to run
 * on.  So that case falls back to DEFLIB, which is where this driver
 * was built to be installed: /lib on Micronix, and whatever prefix a
 * host install was configured with.  Invoke it by a path and it is
 * relocatable again.
 *
 * The path is left with the ".." in it rather than resolved: realpath
 * is a POSIX function and this program has to compile for a system
 * that predates it.  open() and exec() do not care.
 */
#define LIBDIRMAX 512
char libdir[LIBDIRMAX];
char libexecdir[LIBDIRMAX];
char bindir[LIBDIRMAX];

/*
 * The target this installation is for, when -m does not say.  A
 * driver built for Micronix produces Micronix binaries by default,
 * one built for CP/M produces .com files; -m is the override for
 * cross-building, and only picks which runtime is linked - every
 * target's libraries live side by side in the one lib directory.
 */
/*
 * The prefix on everything that makes or reads a micronix object and
 * runs on the host.
 *
 * On micronix these are the system's own programs and wear their own
 * names.  On the host they share a machine with programs of the same
 * name that mean something else entirely - ld, nm, ar, size, cc - so
 * every one of them is mx-prefixed, and the compiler is mxccc.  That
 * used to be a special case for the linker alone, spelled as an
 * ifdef around ld and mxld; the fork made the whole toolchain live on
 * both machines, so it is a rule now rather than an exception.
 *
 * CCC is the test because it answers exactly the right question: it
 * is defined when this driver was compiled by ccc, and a driver
 * compiled by ccc is one running on micronix.
 */
#ifdef CCC
#define MXPFX ""
#else
#define MXPFX "mx"
#endif

#ifndef DEFTARGET
#define DEFTARGET "micronix"
#endif

/*
 * Where the temporaries go.  /tmp exists on every system this runs
 * on, Micronix included.
 */
#ifndef TMPDIR
#define TMPDIR "/tmp"
#endif

/*
 * Where to look when argv[0] has no slash and so says nothing about
 * where we were found.  This is the one thing that is compiled in,
 * and it is only a fallback.
 */
#ifndef DEFLIBEXEC
#define DEFLIBEXEC "/libexec"
#endif

#ifndef DEFLIB
#define DEFLIB "/lib"
#endif

#ifndef DEFBIN
#define DEFBIN "/bin"
#endif

/*
 * Duplicate a string (strdup is POSIX, not C99)
 */
char *
strdup_(char *s)
{
    char *p = malloc(strlen(s) + 1);
    if (p)
        strcpy(p, s);
    return p;
}

#define strdup strdup_

/*
 * The temporaries, and getting rid of them on the way out of ANY exit.
 *
 * They were unlinked at the point each pass finished, which is right
 * up until a pass FAILS: every error path exits before reaching the
 * unlink below it, so a failed compile left its .x .i .1 .2 .n behind.
 *
 * That is not the usual harmless litter.  The names carry the pid, and
 * pids recycle, so a leftover file is a landmine for whatever process
 * draws that number next - and /tmp is sticky, so a file another user
 * left cannot be overwritten.  "sudo make install" seeds /tmp with
 * root-owned temporaries and an ordinary build later dies on one with
 *
 *	cannot create: /tmp/ccc11097_0.x
 *
 * naming whichever source happened to be compiling, which is nothing
 * to do with the source and does not reproduce.
 *
 * So they are registered as they are named and removed by one call
 * that every exit goes through.  Registering rather than unlinking a
 * known list keeps it honest: a temporary added later is covered by
 * the call that names it, not by remembering to extend a cleanup.
 */
#define MAXTMP 8
static char *tmpnames[MAXTMP];
static int ntmpnames;
static int keeptmps;        /* -k or -n: leave them for inspection */

/*
 * A COPY of the name, because the caller frees its own as it goes and
 * the registry has to outlive that.
 */
void
addtmp(char *p)
{
    if (p && ntmpnames < MAXTMP)
        tmpnames[ntmpnames++] = strdup(p);
}

void
rmtmps(void)
{
    while (ntmpnames > 0) {
        ntmpnames--;
        if (!keeptmps)
            unlink(tmpnames[ntmpnames]);
        free(tmpnames[ntmpnames]);
        tmpnames[ntmpnames] = 0;
    }
}

/*
 * Rename a file over another one.  link-then-unlink rather than
 * rename(), which v6 does not have - and the two have the same
 * restriction anyway, that both names are on one filesystem.
 */
int
moveover(char *from, char *to)
{
    unlink(to);
    if (link(from, to) != 0)
        return -1;
    unlink(from);
    return 0;
}

void
usage(void)
{
    printf("usage: %s [<options>] <files...>\n", progname);
    printf("  files: .c (compile) .s (assemble) .o .a (link)\n");
    printf("  -o <output>    Output file: the binary, or with -c/-s\n");
    printf("                 the .o or .s (default: a.out)\n");
    printf("  -c             Compile and assemble only, keep .o\n");
    printf("  -s             Compile only, keep .s (no assembly)\n");
    printf("  -k             Keep all intermediates (.x, .ast, .dat, .s, .o)\n");
    printf("  -O             Run the peephole optimizer over the assembly\n");
    printf("  -S             Strip symbols from output\n");
    printf("  -9             Use 9-char symbols in output\n");
    printf("  -I<dir>        Include directory\n");
    printf("  -i<dir>        System include directory (default /usr/include)\n");
    printf("  -m <system>    Target system: micronix (default) or cpm\n");
    printf("  -D<var>[=val]  Define macro\n");
    printf("  -E             Preprocess only\n");
    printf("  -H             Use .tok (readable) input for pass1 instead of .x\n");
    printf("  -l<lib>        Link with library lib<lib>.a\n");
    printf("  -L<dir>        Add <dir> to library search path\n");
    printf("  -x             Print commands as they execute\n");
    printf("  -n             Print commands without executing (dry run)\n");
    printf("  -C <flags>     Pass -v <flags> to cpp\n");
    printf("  -1 <flags>     Pass -v <flags> to pass1 (c0)\n");
    printf("  -2 <flags>     Pass -v <flags> to pass2 (c1)\n");
    exit(1);
}

/*
 * Get basename without extension (.c, .s, .o, .a)
 * Returns a newly allocated string
 */
char *
getBaseNoExt(char *filename)
{
    char *slash = strrchr(filename, '/');
    char *result;
    char *dot;

    /*
     * The last component, found here rather than with basename():
     * that is in <libgen.h>, which neither Micronix nor CP/M has, and
     * this driver has to compile for both.
     */
    result = strdup(slash ? slash + 1 : filename);

    /* Remove known extensions */
    dot = strrchr(result, '.');
    if (dot && (strcmp(dot, ".c") == 0 || strcmp(dot, ".s") == 0 ||
                strcmp(dot, ".o") == 0 || strcmp(dot, ".a") == 0)) {
        *dot = '\0';
    }

    return result;
}

/*
 * Drop the 16-byte object header from a linked image so what is left
 * is what CP/M loads at 0x100.  The image is copied a fixed buffer at
 * a time through a temp file rather than into one heap allocation: a
 * .com can approach 64KB, and on the 64K machine the image plus that
 * buffer will not fit beside the heap the driver has already spent.
 * The whole-file read that used to be here malloc'd the image and
 * then came back short - the file was complete, the buffer was not.
 */
#define WSHDRLEN 16
#define STRIPBUF 512

int
stripHeader(char *path)
{
    FILE *in, *out;
    char tmp[64];
    char buf[STRIPBUF];
    long len;
    size_t n, total;

    in = fopen(path, "rb");
    if (!in) {
        perror(path);
        return -1;
    }
    fseek(in, 0L, SEEK_END);
    len = ftell(in);
    if (len <= WSHDRLEN) {
        fprintf(stderr, "%s: too short to be an image\n", path);
        fclose(in);
        return -1;
    }
    fseek(in, (long)WSHDRLEN, SEEK_SET);

    sprintf(tmp, "%s/cccstrip_%d", TMPDIR, (int)getpid());
    out = fopen(tmp, "wb");
    if (!out) {
        perror(tmp);
        fclose(in);
        return -1;
    }

    total = 0;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) {
            fclose(in);
            fclose(out);
            unlink(tmp);
            return -1;
        }
        total += n;
    }
    fclose(in);
    fclose(out);

    if (total != (size_t)(len - WSHDRLEN)) {
        fprintf(stderr, "%s: short read\n", path);
        unlink(tmp);
        return -1;
    }
    if (moveover(tmp, path) != 0) {
        perror(path);
        unlink(tmp);
        return -1;
    }
    return 0;
}

/*
 * Print a command line
 */
void
printCommand(char **args)
{
    int i;
    for (i = 0; args[i]; i++) {
        if (i > 0) printf(" ");
        printf("%s", args[i]);
    }
    printf("\n");
}

/*
 * Execute a command with arguments
 * Returns exit status of child process
 */
int
execCommand(char *cmd, char **args)
{
    int pid;
    int status;

    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        /* Child process */
        execv(cmd, args);
        /* If execv returns, it failed */
        perror(cmd);
        exit(1);
    }

    /*
     * wait, not waitpid: there is only ever one child in flight here,
     * and v6 has no waitpid.
     */
    if (wait(&status) < 0) {
        perror("wait");
        exit(1);
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else {
        return 1;  /* Abnormal termination */
    }
}

/*
 * The passes know identifiers only as @id; their spellings
 * sit in the .n sidecar.  The passes stay ignorant - the driver owns
 * diagnostics, so it runs c0 and c1 with stderr through a pipe and
 * rewrites @id to the name on the way past.  Lookup is two seeks,
 * same as c1's: a 2-byte count, 2-byte offsets, NUL-terminated names
 * in id order, ids 1-based.
 */
static int nfd = -1;

static void
nname(unsigned int id, char *buf, int size)
{
    unsigned char two[2];
    int n, i;

    lseek(nfd, (long)(2 + 2 * (id - 1)), 0);
    read(nfd, (char *)two, 2);
    lseek(nfd, (long)(two[0] | (two[1] << 8)), 0);
    n = read(nfd, buf, size - 1);
    for (i = 0; i < n; i++)
        if (!buf[i])
            return;
    buf[i] = 0;
}

int
execFiltered(char *cmd, char **args, char *nfile)
{
    int pfd[2];
    int pid;
    int status;
    char buf[128];
    char nam[20];
    int n, i;
    char c;
    int at = 0;             /* digits seen after '@', +1 */
    unsigned int id = 0;

    nfd = nfile ? open(nfile, O_RDONLY) : -1;
    if (nfd < 0)
        return execCommand(cmd, args);

    if (pipe(pfd) < 0) {
        perror("pipe");
        exit(1);
    }

    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        close(pfd[0]);
        /*
         * close-then-dup, not dup2: dup returns the lowest free
         * descriptor, so closing 2 first puts the pipe there.  That
         * is how it was done before dup2 existed, and Micronix is
         * from before dup2 existed.
         */
        close(2);
        dup(pfd[1]);
        close(pfd[1]);
        execv(cmd, args);
        perror(cmd);
        exit(1);
    }

    close(pfd[1]);
    while ((n = read(pfd[0], buf, sizeof(buf))) > 0) {
        for (i = 0; i < n; i++) {
            c = buf[i];
            if (at) {
                if (c >= '0' && c <= '9') {
                    id = id * 10 + (c - '0');
                    at = 2;
                    continue;
                }
                if (at > 1) {
                    nname(id, nam, sizeof(nam));
                    write(2, nam, strlen(nam));
                } else {
                    write(2, "@", 1);
                }
                at = 0;
            }
            if (c == '@') {
                at = 1;
                id = 0;
                continue;
            }
            write(2, &c, 1);
        }
    }
    if (at > 1) {
        nname(id, nam, sizeof(nam));
        write(2, nam, strlen(nam));
    } else if (at) {
        write(2, "@", 1);
    }
    close(pfd[0]);
    close(nfd);
    nfd = -1;

    if (wait(&status) < 0) {
        perror("wait");
        exit(1);
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else {
        return 1;
    }
}

/*
 * The eleven paths this builds are file scope, not locals in main.
 * Together they are eleven kilobytes, and a frame that size on a Z80
 * pushes every local past the 7-bit (iy+d) window - so their
 * addresses stop being (iy+d) operands and have to be worked out into
 * HL.  A store whose address AND value are both in HL has no rule in
 * pass2, and ten assignments in this file quietly produced no code at
 * all.  See GAP at the top of the tree.
 *
 * They are written once and read throughout, so file scope costs
 * nothing, and an eleven kilobyte frame was never defensible on this
 * machine anyway.
 */
static char cpp_path[1024];
static char cc1_path[1024];
static char cc2_path[1024];
static char asm_path[1024];
static char ld_path[1024];
static char astpp_path[1024];
static char peep_path[1024];

static char chdr_path[1024];
static char libc_path[1024];
static char libu_path[1024];
static char sysinc_path[1024];
static char ldlib_path[1024];   /* -L for the system library directory */

int
main(int argc, char **argv)
{
    char *output_file = NULL;
    int keep_all = 0;        /* -k: keep all intermediates */
    int compile_only = 0;    /* -c: compile+assemble to .o */
    int named_output = 0;    /* -o was given, so it names the output */
    int asm_only = 0;        /* -s: compile to .s only */
    int print_cmds = 0;      /* -x: print commands as they execute */
    int no_exec = 0;         /* -n: don't execute (dry run) */
    int strip_syms = 0;      /* -S: strip symbols from output */
    int nine_char = 0;       /* -9: use 9-char symbols */
    int use_prep = 0;        /* -H: use .i file for pass1 instead of .x */
    int optimize = 0;        /* -O: run the peephole over c1's assembly */

    /*
     * -m: which system the output runs on.  It selects the runtime
     * tree - micronix/ or cpm/ - that the headers, the libraries and
     * the startup object come from.  Micronix is the default because
     * that is what the simulator runs and what every test here links.
     */
    char *target = DEFTARGET;
    int cpm_target = 0;      /* target is CP/M: image layout differs */

    /* Input files by type */
    char *c_files[MAX_ARGS];
    char *s_files[MAX_ARGS];
    char *o_files[MAX_ARGS];
    char *a_files[MAX_ARGS];
    int c_count = 0, s_count = 0, o_count = 0, a_count = 0;
    int o_input_count = 0;   /* .o files from cmdline (vs generated) */

    /* Linker options */
    char *ld_libs[MAX_ARGS];  /* -l<lib> options */
    char *ld_paths[MAX_ARGS]; /* -L<dir> options */
    int ld_lib_count = 0;
    int ld_path_count = 0;

    char *cpp_base[MAX_ARGS];  /* Base cpp args (options only) */
    char *cc1_base[MAX_ARGS];  /* Base cc1 args (options only) */
    char *cc2_base[MAX_ARGS];  /* Base cc2 args (options only) */
    int cpp_base_argc = 0;
    int cc1_base_argc = 0;
    int cc2_base_argc = 0;

    int status;
    int i;

    progname = argv[0];

    /*
     * Strip the program name off argv[0] to get the directory it was
     * found in, then take the lib beside it.  A name with no slash in
     * it was found on the search path or in the current directory;
     * "." is what that means here.
     */
    {
        char *slash = strrchr(progname, '/');

        if (slash) {
            int n = slash - progname;

            if (n > LIBDIRMAX - 16)
                n = LIBDIRMAX - 16;
            strncpy(bindir, progname, n);
            bindir[n] = '\0';
            strcpy(libdir, bindir);
            strcpy(libexecdir, bindir);
            strcat(libdir, "/../lib");
            strcat(libexecdir, "/../libexec");
        } else {
            strcpy(bindir, DEFBIN);
            strcpy(libdir, DEFLIB);
            strcpy(libexecdir, DEFLIBEXEC);
        }
    }

    /*
     * The passes live in LIBEXEC, which is what that directory has
     * always been for: programs run by other programs.  The assembler
     * and the linker are user commands - a person types them - so they
     * live beside the driver in BIN.
     *
     * lib is for libraries - libc.a, crt0.o and the headers, further
     * down - and keeping the two apart means a person reading /lib
     * sees things to link against and nothing else.
     */
    sprintf(cpp_path, "%s/%spass0", libexecdir, MXPFX);
    sprintf(cc1_path, "%s/%sc0", libexecdir, MXPFX);
    sprintf(cc2_path, "%s/%sc1", libexecdir, MXPFX);
    sprintf(asm_path, "%s/%sasz", bindir, MXPFX);
    sprintf(ld_path, "%s/%sld", bindir, MXPFX);
    sprintf(astpp_path, "%s/%sastpp", libexecdir, MXPFX);
    sprintf(peep_path, "%s/%speep", libexecdir, MXPFX);

    /*
     * The runtime is per target and cannot be resolved until -m has
     * been seen, so it is built after the argument loop below.
     */

    /* Initialize base argument arrays with program names */
    cpp_base[cpp_base_argc++] = cpp_path;
    cc1_base[cc1_base_argc++] = cc1_path;
    cc2_base[cc2_base_argc++] = cc2_path;

    /* Parse arguments */
    argc--;
    argv++;

    while (argc > 0) {
        if (strcmp(argv[0], "-h") == 0 || strcmp(argv[0], "--help") == 0) {
            usage();
        } else if (strcmp(argv[0], "-o") == 0) {
            argc--;
            argv++;
            if (argc == 0) {
                fprintf(stderr, "Error: -o requires an argument\n");
                usage();
            }
            output_file = argv[0];
            argc--;
            argv++;
        } else if (strcmp(argv[0], "-k") == 0) {
            keep_all = 1;
            keeptmps = 1;
            argc--;
            argv++;
        } else if (strcmp(argv[0], "-c") == 0) {
            compile_only = 1;
            argc--;
            argv++;
        } else if (strcmp(argv[0], "-s") == 0) {
            asm_only = 1;
            argc--;
            argv++;
        } else if (strcmp(argv[0], "-O") == 0) {
            optimize = 1;
            argc--;
            argv++;
        } else if (strcmp(argv[0], "-S") == 0) {
            strip_syms = 1;
            argc--;
            argv++;
        } else if (strcmp(argv[0], "-9") == 0) {
            nine_char = 1;
            argc--;
            argv++;
        } else if (strcmp(argv[0], "-H") == 0) {
            use_prep = 1;
            argc--;
            argv++;
        } else if (strcmp(argv[0], "-x") == 0) {
            print_cmds = 1;
            argc--;
            argv++;
        } else if (strcmp(argv[0], "-m") == 0) {
            /* Which system to build for: picks the runtime tree */
            argc--;
            argv++;
            if (argc == 0) {
                fprintf(stderr, "Error: -m requires an argument\n");
                usage();
            }
            target = argv[0];
            if (strcmp(target, "micronix") != 0 &&
                strcmp(target, "cpm") != 0) {
                fprintf(stderr, "Error: unknown target %s "
                        "(micronix or cpm)\n", target);
                exit(1);
            }
            argc--;
            argv++;
        } else if (strcmp(argv[0], "-n") == 0) {
            no_exec = 1;
            keeptmps = 1;
            argc--;
            argv++;
        } else if (argv[0][0] == '-' &&
                   (argv[0][1] == 'I' || argv[0][1] == 'i' ||
                    argv[0][1] == 'D')) {
            /* Pass -I, -i, or -D options to cpp */
            if (cpp_base_argc >= MAX_ARGS) {
                fprintf(stderr, "Error: too many arguments\n");
                rmtmps();
                exit(1);
            }
            cpp_base[cpp_base_argc++] = argv[0];
            argc--;
            argv++;
        } else if (strcmp(argv[0], "-E") == 0) {
            /* Pass -E to cpp (preprocess only) */
            if (cpp_base_argc >= MAX_ARGS) {
                fprintf(stderr, "Error: too many arguments\n");
                rmtmps();
                exit(1);
            }
            cpp_base[cpp_base_argc++] = argv[0];
            argc--;
            argv++;
        } else if (strcmp(argv[0], "-N") == 0) {
            /* Pass -N to cpp (suppress line markers) */
            if (cpp_base_argc >= MAX_ARGS) {
                fprintf(stderr, "Error: too many arguments\n");
                rmtmps();
                exit(1);
            }
            cpp_base[cpp_base_argc++] = argv[0];
            argc--;
            argv++;
        } else if (strcmp(argv[0], "-C") == 0) {
            /* Pass -v <flags> to cpp */
            argc--;
            argv++;
            if (argc == 0) {
                fprintf(stderr, "Error: -C requires an argument\n");
                usage();
            }
            if (cpp_base_argc >= MAX_ARGS - 1) {
                fprintf(stderr, "Error: too many arguments\n");
                rmtmps();
                exit(1);
            }
            cpp_base[cpp_base_argc++] = "-v";
            cpp_base[cpp_base_argc++] = argv[0];
            argc--;
            argv++;
        } else if (strcmp(argv[0], "-1") == 0) {
            /* Pass -v <flags> to pass1 */
            argc--;
            argv++;
            if (argc == 0) {
                fprintf(stderr, "Error: -1 requires an argument\n");
                usage();
            }
            if (cc1_base_argc >= MAX_ARGS - 1) {
                fprintf(stderr, "Error: too many arguments\n");
                rmtmps();
                exit(1);
            }
            cc1_base[cc1_base_argc++] = "-v";
            cc1_base[cc1_base_argc++] = argv[0];
            argc--;
            argv++;
        } else if (strcmp(argv[0], "-2") == 0) {
            /* Pass -v <flags> to pass2 */
            argc--;
            argv++;
            if (argc == 0) {
                fprintf(stderr, "Error: -2 requires an argument\n");
                usage();
            }
            if (cc2_base_argc >= MAX_ARGS - 1) {
                fprintf(stderr, "Error: too many arguments\n");
                rmtmps();
                exit(1);
            }
            cc2_base[cc2_base_argc++] = "-v";
            cc2_base[cc2_base_argc++] = argv[0];
            argc--;
            argv++;
        } else if (argv[0][0] == '-' && argv[0][1] == 'l') {
            /* -l<lib>: pass to linker */
            if (argv[0][2] == '\0') {
                fprintf(stderr, "Error: -l requires a library name\n");
                usage();
            }
            ld_libs[ld_lib_count++] = argv[0];
            argc--;
            argv++;
        } else if (argv[0][0] == '-' && argv[0][1] == 'L') {
            /* -L<dir>: pass to linker */
            if (argv[0][2] == '\0') {
                fprintf(stderr, "Error: -L requires a directory\n");
                usage();
            }
            ld_paths[ld_path_count++] = argv[0];
            argc--;
            argv++;
        } else if (argv[0][0] == '-') {
            fprintf(stderr, "Error: unknown option: %s\n", argv[0]);
            usage();
        } else {
            /* Input file - classify by extension */
            char *ext = strrchr(argv[0], '.');
            if (access(argv[0], R_OK) != 0) {
                fprintf(stderr,
                    "Error: file '%s' not found or not readable\n",
                    argv[0]);
                exit(1);
            }
            if (ext && strcmp(ext, ".c") == 0) {
                c_files[c_count++] = argv[0];
            } else if (ext && strcmp(ext, ".s") == 0) {
                s_files[s_count++] = argv[0];
            } else if (ext && strcmp(ext, ".o") == 0) {
                o_files[o_count++] = argv[0];
            } else if (ext && strcmp(ext, ".a") == 0) {
                a_files[a_count++] = argv[0];
            } else {
                fprintf(stderr, "Error: unknown file type: %s\n", argv[0]);
                rmtmps();
                exit(1);
            }
            argc--;
            argv++;
        }
    }

    /*
     * Now that -m is known, resolve the runtime.  libc is the pure C
     * library, the same object for either target, but each target's
     * tree keeps its own copy beside its own runtime so a target
     * directory is self-contained; the system-call layer and the
     * startup object genuinely differ, so they are named per target.
     */
    cpm_target = (strcmp(target, "cpm") == 0);
    /*
     * The headers do not live beside the passes on a Unix, they live
     * in /usr/include, and that is where the Micronix tree keeps them
     * - so libdir/../usr/include, which is /usr/include when the
     * driver is /bin/ccc and lib/../usr/include in an install tree.
     * Nothing about where the tree lives is compiled in, the same way
     * libdir itself is worked out from argv[0].
     *
     * CP/M's headers live in a tree of their own, /usr/cpm/include,
     * because its stdio is not Micronix's: the FILE layer there sits
     * over fcb's rather than file descriptors, and the flag bits and
     * the stdio.h that spells them differ.  One -i root per target is
     * the whole of the difference as far as a source sees it.
     */
    if (cpm_target) {
        sprintf(libc_path, "%s/cpm/libc.a", libdir);
        sprintf(sysinc_path, "-i%s/../usr/cpm/include", libdir);
        sprintf(libu_path, "%s/cpm/libcpm.a", libdir);
        sprintf(chdr_path, "%s/cpm/crtcpm.o", libdir);
    } else {
        sprintf(libc_path, "%s/libc.a", libdir);
        sprintf(sysinc_path, "-i%s/../usr/include", libdir);
        sprintf(libu_path, "%s/libu.a", libdir);
        sprintf(chdr_path, "%s/crt0.o", libdir);
    }

    /* Check for input files */
    if (c_count + s_count + o_count + a_count == 0) {
        fprintf(stderr, "Error: no input files specified\n");
        usage();
    }

    /*
     * -o names the object when we are stopping at one.  Without this
     * -c wrote foo.o beside foo.c and there was no way to say
     * otherwise, so a build that wanted its objects somewhere else had
     * to cd into the output directory and name the source one level
     * up.  Every rule in the tree that cross-compiles was written that
     * way around this.
     *
     * One output, one input: -o says what to call the thing, and with
     * several sources there is no one thing to call it.  That is what
     * cc has always done here.
     */
    named_output = (output_file != NULL);
    if (named_output && (compile_only || asm_only) &&
        c_count + s_count > 1) {
        fprintf(stderr, "Error: -o with -c or -s takes one input file\n");
        exit(1);
    }

    /* Set default output file */
    if (!output_file) {
        output_file = "a.out";
    }

    /* Track how many .o files existed before we generate more from .c files */
    o_input_count = o_count;

    /* Process each .c file: cpp -> c0 -> c1 -> asm */
    for (i = 0; i < c_count; i++) {
        char *src = c_files[i];
        char *base = getBaseNoExt(src);
        char *lex_file;
        char *prep_file;
        char *name_file;
        char *temp1_file;
        char *temp2_file;
        char *asm_file;
        char *obj_file;
        char *tmpbase;
        char *cpp_args[MAX_ARGS];
        char *cc1_args[MAX_ARGS];
        char *cc2_args[MAX_ARGS];
        char *as_args[8];
        int cpp_argc, cc1_argc, cc2_argc, j;

        /*
         * The intermediates go in /tmp; only what the user asked to
         * keep is written beside the source.  Compiling a file out of
         * a directory you cannot write to is ordinary, and five
         * temporaries per source landing in the middle of someone's
         * tree is not something a compiler should do.
         *
         * The name carries the pid and the file's position on the
         * command line: two ccc's running at once must not collide,
         * and neither must two sources in the one run when -k asks
         * for the temporaries to be left behind.
         *
         * .s is the exception.  Under -s it is the OUTPUT, and goes
         * where the user expects to find it.
         */
        tmpbase = malloc(64);
        sprintf(tmpbase, "%s/ccc%d_%d", TMPDIR, (int)getpid(), i);

        lex_file = malloc(strlen(tmpbase) + 10);
        sprintf(lex_file, "%s.x", tmpbase);
        addtmp(lex_file);
        prep_file = malloc(strlen(tmpbase) + 10);
        sprintf(prep_file, "%s.tok", tmpbase);
        addtmp(prep_file);
        name_file = malloc(strlen(tmpbase) + 10);
        sprintf(name_file, "%s.nam", tmpbase);
        addtmp(name_file);
        temp1_file = malloc(strlen(tmpbase) + 10);
        sprintf(temp1_file, "%s.ast", tmpbase);
        addtmp(temp1_file);
        temp2_file = malloc(strlen(tmpbase) + 10);
        sprintf(temp2_file, "%s.dat", tmpbase);
        addtmp(temp2_file);

        asm_file = malloc(strlen(base) + strlen(tmpbase) +
                          strlen(output_file) + 10);
        if (asm_only) {
            if (named_output)
                strcpy(asm_file, output_file);
            else
                sprintf(asm_file, "%s.s", base);
        } else {
            /* a temporary only when it is not what was asked for */
            sprintf(asm_file, "%s.s", tmpbase);
            addtmp(asm_file);
        }

        obj_file = malloc(strlen(base) + strlen(output_file) + 10);
        if (named_output && compile_only)
            strcpy(obj_file, output_file);
        else
            sprintf(obj_file, "%s.o", base);

        if (!no_exec) printf("=== Compiling %s ===\n", src);

        /*
         * Build cpp args: sysinc + -DCCC + base options + -o tmp + source.
         *
         * Ours goes first so that the user's -i and -I come after it and
         * win.  It used to go last, which made the headers we ship
         * impossible to override: building for Micronix picked up the
         * stub sys/stat.h out of lib/include rather than the real one,
         * and nothing on the command line could say otherwise.
         */
        cpp_argc = 0;
        cpp_args[cpp_argc++] = cpp_base[0];     /* argv[0], the program */
        cpp_args[cpp_argc++] = sysinc_path;
        /*
         * Three flags, three questions, and they are not the same
         * question.
         *
         * CCC says which compiler is compiling this - the calling
         * sequence, the structure packing, the byte order, the word
         * length, and what the language and library do not have.  It
         * is always defined, because the only compiler that defines
         * it is this one.
         *
         * MICRONIX and CPM say which system the result runs on -
         * crt0, libc, where the headers are - and exactly one of
         * them is defined, from -m.  A source asking "am I on
         * micronix" was asking CCC before, which answered a
         * different question and happened to agree.
         *
         * Anything host-side asks the host compiler instead: gcc
         * defines __GNUC__ without being told.
         */
        cpp_args[cpp_argc++] = "-DCCC";
        cpp_args[cpp_argc++] = cpm_target ? "-DCPM" : "-DMICRONIX";
        for (j = 1; j < cpp_base_argc; j++)
            cpp_args[cpp_argc++] = cpp_base[j];
        cpp_args[cpp_argc++] = "-o";
        cpp_args[cpp_argc++] = tmpbase;
        cpp_args[cpp_argc++] = src;
        cpp_args[cpp_argc] = NULL;

        if (print_cmds || no_exec)
            printCommand(cpp_args);
        if (!no_exec) {
            status = execCommand(cpp_path, cpp_args);
            if (status != 0) {
                fprintf(stderr, "Error: pass0 failed on %s\n", src);
                rmtmps();
                exit(status);
            }
        }

        /* Build pass1 args: c0 source.x temp1 temp2 (or .i with -H) */
        cc1_argc = 0;
        for (j = 0; j < cc1_base_argc; j++)
            cc1_args[cc1_argc++] = cc1_base[j];
        cc1_args[cc1_argc++] = use_prep ? prep_file : lex_file;
        cc1_args[cc1_argc++] = temp1_file;
        cc1_args[cc1_argc++] = temp2_file;
        cc1_args[cc1_argc] = NULL;

        if (print_cmds || no_exec)
            printCommand(cc1_args);
        if (!no_exec) {
            status = execFiltered(cc1_path, cc1_args, name_file);
            if (status != 0) {
                fprintf(stderr, "Error: c0 failed on %s\n", src);
                rmtmps();
                exit(status);
            }
        }

        /* Clean up .x and .i if they exist */
        if (!keep_all && !no_exec) {
            unlink(lex_file);
            unlink(prep_file);
        }
        free(lex_file);
        free(prep_file);

        /* Build pass2 args: c1 temp1 temp2 asm_file */
        cc2_argc = 0;
        for (j = 0; j < cc2_base_argc; j++)
            cc2_args[cc2_argc++] = cc2_base[j];
        cc2_args[cc2_argc++] = temp1_file;
        cc2_args[cc2_argc++] = temp2_file;
        cc2_args[cc2_argc++] = asm_file;
        cc2_args[cc2_argc] = NULL;

        if (print_cmds || no_exec)
            printCommand(cc2_args);
        if (!no_exec) {
            status = execFiltered(cc2_path, cc2_args, name_file);
            if (status != 0) {
                fprintf(stderr, "Error: c1 failed on %s\n", src);
                rmtmps();
                exit(status);
            }
        }

        /* Clean up temp files unless -k or -n */
        if (!keep_all && !no_exec) {
            unlink(temp1_file);
            unlink(temp2_file);
            unlink(name_file);
        }
        free(temp1_file);
        free(temp2_file);
        free(name_file);

        /*
         * Peephole, if asked for.  It rewrites the assembly in place -
         * through a temporary, so a failure leaves the original where
         * it was rather than a half written file - which keeps .s
         * meaning "the assembly that gets assembled" whether or not
         * -O was given.  To see what it changed, compile twice.
         */
        if (optimize) {
            char *peep_file;
            char *peep_args[8];

            /*
             * Beside the assembly it replaces, not beside the source:
             * rename() does not cross a filesystem, and under -s the
             * assembly is in the source directory while under -c it
             * is in /tmp.
             */
            peep_file = malloc(strlen(asm_file) + 10);
            sprintf(peep_file, "%s.ps", asm_file);

            peep_args[0] = peep_path;
            peep_args[1] = asm_file;
            peep_args[2] = peep_file;
            peep_args[3] = NULL;

            if (print_cmds || no_exec)
                printCommand(peep_args);
            if (!no_exec) {
                status = execCommand(peep_path, peep_args);
                if (status != 0) {
                    fprintf(stderr, "Error: peep failed on %s\n", asm_file);
                    rmtmps();
                    exit(status);
                }
                if (moveover(peep_file, asm_file) != 0) {
                    fprintf(stderr, "Error: cannot replace %s\n", asm_file);
                    rmtmps();
                    exit(1);
                }
            }
            free(peep_file);
        }

        /* If -s, we're done with this file */
        if (asm_only) {
            if (!no_exec) printf("  -> %s\n", asm_file);
            free(asm_file);
            free(obj_file);
            free(base);
            continue;
        }

        /* Assemble to .o */
        as_args[0] = asm_path;
        as_args[1] = "-o";
        as_args[2] = obj_file;
        as_args[3] = asm_file;
        as_args[4] = NULL;

        if (print_cmds || no_exec)
            printCommand(as_args);
        if (!no_exec) {
            status = execCommand(asm_path, as_args);
            if (status != 0) {
                fprintf(stderr, "Error: assembler failed on %s\n", asm_file);
                rmtmps();
                exit(status);
            }

            /* Clean up .s file unless -k */
            if (!keep_all)
                unlink(asm_file);
        }
        free(asm_file);

        /* Add to object list for linking */
        o_files[o_count++] = obj_file;
        if (!no_exec) printf("  -> %s\n", obj_file);
        free(base);

        /*
         * This source is done with its temporaries.  The unlinks above
         * have already taken them one pass at a time; this is what
         * empties the REGISTRY, so the next source starts with room in
         * it - six names per source against MAXTMP, so leaving them
         * registered would silently stop covering the second file.
         */
        rmtmps();
    }

    /* If -S, we're done */
    if (asm_only) {
        return 0;
    }

    /* Process each .s file: assemble to .o */
    for (i = 0; i < s_count; i++) {
        char *src = s_files[i];
        char *base = getBaseNoExt(src);
        char *obj_file;
        char *as_args[8];

        obj_file = malloc(strlen(base) + strlen(output_file) + 10);
        if (named_output && compile_only)
            strcpy(obj_file, output_file);
        else
            sprintf(obj_file, "%s.o", base);

        if (!no_exec) printf("=== Assembling %s ===\n", src);

        as_args[0] = asm_path;
        as_args[1] = "-o";
        as_args[2] = obj_file;
        as_args[3] = src;
        as_args[4] = NULL;

        if (print_cmds || no_exec)
            printCommand(as_args);
        if (!no_exec) {
            status = execCommand(asm_path, as_args);
            if (status != 0) {
                fprintf(stderr, "Error: assembler failed on %s\n", src);
                rmtmps();
                exit(status);
            }
        }

        o_files[o_count++] = obj_file;
        if (!no_exec) printf("  -> %s\n", obj_file);
        free(base);
    }

    /* If -c, we're done */
    if (compile_only) {
        return 0;
    }

    /* Link all object files and libraries */
    {
        char *ld_args[MAX_ARGS];
        int ld_argc = 0;

        if (!no_exec) printf("\n=== Linking -> %s ===\n", output_file);

        ld_args[ld_argc++] = ld_path;
        if (strip_syms)
            ld_args[ld_argc++] = "-s";
        if (nine_char)
            ld_args[ld_argc++] = "-9";
        ld_args[ld_argc++] = "-o";
        ld_args[ld_argc++] = output_file;

	/*
	 * Both systems load at 0x100.  Micronix reads the segment
	 * table out of the header and places data and bss itself; CP/M
	 * has no loader worth the name - it reads the file to 0x100
	 * and jumps - so text, data and bss have to come out as one
	 * contiguous image, which is what naming the same origin for
	 * all three gets.
	 */
	ld_args[ld_argc++] = "-Ttext=0x100";
	if (cpm_target) {
	    ld_args[ld_argc++] = "-Tdata=0x100";
	    ld_args[ld_argc++] = "-Tbss=0x100";
	}

	/*
	 * The system library directory, so that -l works at all.
	 *
	 * ld searches nothing by default: findlib walks the -L list and
	 * that list came only from the command line, so "-lutil" found
	 * nothing however plainly /lib/libutil.a was sitting there.  The
	 * driver already works libdir out from its own path and hands ld
	 * crt0.o, libc.a and libu.a out of it by full name; a library
	 * asked for by name should come from the same place.  vi asks
	 * for -lutil, is linked by this driver, and did not build.
	 *
	 * First, so that a -L on the command line still wins for a
	 * library that exists in both.
	 */
	sprintf(ldlib_path, "-L%s", libdir);
	ld_args[ld_argc++] = ldlib_path;

	/* Add library search paths (-L options) */
	for (i = 0; i < ld_path_count; i++)
	    ld_args[ld_argc++] = ld_paths[i];

	/* c object header */
	ld_args[ld_argc++] = chdr_path;

        /* Add object files */
        for (i = 0; i < o_count; i++)
            ld_args[ld_argc++] = o_files[i];

        /*
         * THE DRIVER'S OWN LIBRARIES GO LAST, after everything the
         * command line named.  There used to be a libc_path here as
         * well, ahead of them, and it meant a library named on the
         * command line could not override anything: ccc's libc was
         * asked first and answered, and the named one was left with
         * whatever was still missing.
         *
         * Micronix has its own libc in lib/libc, built from the same
         * sources, and links it by naming it - so every program in
         * that tree was quietly taking ccc's copy of anything the two
         * both define, which is nearly all of it.  An edit to the
         * system's own C library reached nothing, and the binary gave
         * no sign of which one it had.
         *
         * ld resolves in order and takes the first archive that can
         * answer, so naming a library first is how a program says
         * which one it means.  These are the fallback, and a fallback
         * goes at the end.
         */

        /* Add library files */
        for (i = 0; i < a_count; i++)
            ld_args[ld_argc++] = a_files[i];

        /* Add user-specified libraries (-l options) */
        for (i = 0; i < ld_lib_count; i++)
            ld_args[ld_argc++] = ld_libs[i];

        /*
         * libc, libu, libc: they call each other - libc's printf wants
         * write from libu, libu's perror wants libc - and ld takes one
         * pass per archive, so each has to be offered again after the
         * other has had its say.
         */
        ld_args[ld_argc++] = libc_path;
        ld_args[ld_argc++] = libu_path;
        ld_args[ld_argc++] = libc_path;

        ld_args[ld_argc] = NULL;

        if (print_cmds || no_exec)
            printCommand(ld_args);
        if (!no_exec) {
            status = execCommand(ld_path, ld_args);
            if (status != 0) {
                fprintf(stderr, "Error: linker failed\n");
                rmtmps();
                exit(status);
            }

            /*
             * A .com file is the bytes CP/M loads at 0x100 and
             * nothing else.  ld writes a 16-byte Whitesmith's
             * header in front of them, so cut it off; the Makefiles
             * that built these images by hand all ended in a
             * "tail -c +17" for the same reason.
             */
            if (cpm_target && stripHeader(output_file) != 0)
                exit(1);

            /* Clean up generated .o files unless -k */
            if (!keep_all) {
                for (i = o_input_count; i < o_count; i++) {
                    if (unlink(o_files[i]) != 0)
                        perror(o_files[i]);
                }
            }
        }
    }

    if (!no_exec)
        printf("\n=== Build successful: %s ===\n", output_file);

    return 0;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
