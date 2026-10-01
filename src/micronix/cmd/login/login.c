/*
 * login - sign a user on to the system.
 *
 * cmd/login/login.c
 *
 * There is no source for login in the distribution, so this is the
 * disassembly of the 1.6 binary read back into C.  The binary is
 * login.dist, inode 45 of disks/dist/1011-8_dist_1.IMD, 23023 bytes,
 * August 1982, stripped.  The README beside this file has the map: the
 * calling convention the original compiler used, where login's own code
 * stops and the library it was linked against begins, and the addresses
 * of the entries on both sides of that seam.
 *
 * The code below is written for THIS tree's compiler and THIS tree's
 * library, and not for the one the binary was built with.  The original
 * library - libwsc.a on the C system disk - is not in this tree, so what
 * login used it for is either here, or is libc under a different name, or
 * is done by hand:
 *
 *	lenstr, cmpstr, cpystr, instr
 *			strlen, strcmp, strcpy+strcat, strchr.  The
 *			original's cpystr concatenated every argument in
 *			one call; here the same strings are built with
 *			strcat, which is what this tree has.
 *	ttyname, ttyslot
 *			not in libc here, the same gap ps and pr hit.
 *	crypt			not in this tree at all.
 *	getpwnam, endpwent	not in libc here either; owner.c reports
 *			the same thing and reads /etc/passwd by hand.
 *
 * So this file is the login from the binary, saying what the disassembly
 * says, written against the library this tree builds.  It is a
 * reconstruction of behaviour and not the original source, and it does
 * not rebuild the 1982 binary - see the README's caveats.
 */

#include <types.h>
#include <stdio.h>
#include <pwd.h>
#include <crypt.h>

/*
 * The records login reads and writes.  <pwd.h> is the tree's and the
 * binary agrees with it exactly: name+0, passwd+2, uid+4, gid+5,
 * person+6, dir+8, shell+10, twelve bytes with uid and gid one byte
 * each - the data image's own DB directives at 0x61cc and 0x61cd say
 * one byte.  login hands the kernel those two bytes as the single word
 * uid | gid << 8, which is what chown and setuid take here; the three
 * places it does so are marked below.
 *
 * The other three shapes are this file's own, because <utmp.h> does not
 * describe the lastlog record and <stat.h>'s struct is not what the
 * kernel fills.
 */

struct utmp {                   /* 20 bytes, v7 layout - same as <utmp.h> */
    char    ut_line[8];         /* +0  the tty name, e.g. "tty2" */
    char    ut_name[8];         /* +8  the login name */
    long    ut_time;            /* +16 */
};

struct lastlog {                /* 20 bytes: line, then the time */
    char    ll_line[16];        /* +0  the tty name */
    long    ll_time;            /* +16 */
};

/* The stat the kernel fills is the raw 36-byte v6 one, which is what
   stat.s and lseek.c in this tree describe: dev at +0, ino at +2, and a
   word-swapped long size at +8.  It is NOT <stat.h>'s struct, which is
   fourteen bytes of something else.  acct() and exists() reserve one and
   never read it; ttyname() reads its first four bytes. */
struct stat {
    short st_dev;               /* +0 */
    short st_ino;               /* +2 */
    char  st_rest[32];
};

struct sgttyb {                 /* 6 bytes: speeds, erase, kill, tflags */
    int  speeds;                /* +0 */
    char erase;                 /* +2 */
    char kill;                  /* +3 */
    int  tflags;                /* +4 */
};
#define ECHO    010             /* the bit echo_on/echo_off toggle */

/* ------------------------------------------------------------------ */
/* login's own data (data segment 0x6000-0x6070)                       */
/* ------------------------------------------------------------------ */

static char        *utmpfile     = "/etc/utmp";         /* 0x6002 -> text 0x0111 */
static char        *maildir      = "/usr/spool/mail/";  /* 0x6004 -> text 0x011b */
static char        *lastlogfile  = "/usr/adm/lastlog";  /* 0x6006 -> text 0x012c */
static char        *wtmpfile     = "/usr/adm/wtmp";     /* 0x6008 -> text 0x013d */
static char        *ttypath;                            /* 0x600a - acct() sets it,
                                                           from ttyname(0) */
static struct lastlog lastlog;                          /* 0x600c - lastlog record */
static struct utmp  utmp;                               /* 0x6020 - utmp record */
static struct passwd *pw;                               /* 0x6034 - getpwnam result */
static struct stat  statbuf;                            /* 0x603f - exists()' buffer */
static int          ttyfd;                              /* 0x6063 - acct()' fd */
static long         now;                                /* 0x6065 - time() temp */
static char         catfd;                              /* 0x6069 - type()' fd; a char,
                                                           so only its low byte is
                                                           tested */
static int          catn;                               /* 0x606a - type()' read count */
static long         lastlogsize = 20L;                  /* 0x606c - sizeof lastlog */

/* 0x6000 - is the terminal echoing?  The data image starts it at 1, and
   promptline() tests the whole word. */
static int          echo_state = 1;                     /* 0x6000, 0x6001 = 0 */

static struct sgttyb ttyb_on;   /* 0x6072 - sgttyb used by echo_on();
                                   its tflags is at 0x6076 */
static struct sgttyb ttyb_off;  /* 0x6078 - sgttyb used by echo_off();
                                   its tflags is at 0x607c */

/* The three loop counters below are ordinary `register` variables in the
   source.  The original compiler gave each one a slot in the data segment
   instead of the stack, which is why they have names here at all. */
static char         sig_i;              /* 0x607e - ignore_signals()' counter */
static char         sig_j;              /* 0x607f - catch_signals()' counter */
static char         sig_k;              /* 0x6080 - reset_signals()' counter */

/* Likewise start_shell()' cursors. */
static char        *arg_p;              /* 0x6081 - the basename scan pointer */
static char        *arg_sp;             /* 0x6083 - the shell path (execv's) */
static char       **arg_ap;             /* 0x6085 - the argv fill pointer */

/* ------------------------------------------------------------------ */
/* login's own routines, called across this file                       */
/* ------------------------------------------------------------------ */

void  outstr(char *s);                      /* 0x01da */
void  login(char *name);                    /* 0x0221 */
int   exists(char *name);                   /* 0x0322 */
void  acct(void);                           /* 0x0347 */
void  type(char *name);                     /* 0x0460 */
int   chkname(char *name);                  /* 0x04db */
int   nographic(char *s);                   /* 0x0532 - start_shell's blank() */
int   alnumdash(char *s);                   /* 0x0570 */
void  lastlogin(void);                      /* 0x060c */
void  writestr(char *s);                    /* 0x075b */
void  promptline(char *prompt, char *buf);  /* 0x07aa */
void  echo_on(void);                        /* 0x0803 */
void  echo_off(void);                       /* 0x085f */
int   chkpass(char *name);                  /* 0x08f4 */
void  ignore_signals(void);                 /* 0x097c */
void  catch_signals(void);                  /* 0x09ab */
void  reset_signals(void);                  /* 0x09e5 */
void  start_shell(char *sh);                /* 0x0a39 */
char *strsave(char *s);                     /* 0x0b2c */
char *getword(char *sp, char *buf);         /* 0x0b77 */
char *skipblanks(char *p);                  /* 0x0bdb */

/* crypt is libc's now - lib/libc/crypt.c - and <crypt.h> declares
   it.  It was login's own code and lived below at 0x0c37. */

/* These two are not in libc here; they are written at the end of this
   file, and declared the way this tree declares things.  getpwnam and
   endpwent were declared here with them and are libc's now, declared
   by <pwd.h> - see the note below where their code used to be. */
char *ttyname();                            /* 0x35e7 */
int   ttyslot();                            /* 0x3719 */

/* ------------------------------------------------------------------ */
/*
 * 0x015e - main.  Argument 0 (argc) is kept in register variable r3
 * (0x6563) and argument 1 (argv) in r1 (0x655f); the exec path started by
 * 0x0a39 reads those two cells to build the shell's argv.  The loop never
 * exits: a failed login falls back to the "Name: " prompt.
 */
main(argc, argv)
char **argv;
{
    char name[BUFSIZ];                  /* DE-0x206 */
    register int argc_r;                /* r3 = 0x6563 */
    register char **argv_r;             /* r1 = 0x655f */

    argc_r = argc;
    argv_r = argv;
    catch_signals();                           /* 0x09ab - flag 0x607f */
    type("/etc/banner");              /* 0x0460 */
    for (;;) {
        promptline("Name: ", name);         /* 0x07aa */
        if (chkname(name) == 0)
            continue;
        pw = (struct passwd *)getpwnam(name);     /* 0x1103 */
        if (pw == (struct passwd *)0)
            continue;
        endpwent();                       /* 0x119b - close the passwd stream */
        if (chkpass(name) == 0)          /* 0x08f4 - prints "Password incorrect.\n" */
            continue;
        ignore_signals();                       /* 0x097c - flag 0x607e */
        login(name);                   /* 0x0221 - the session */
    }
}

/*
 * 0x01da - write a NUL-terminated string on the standard output.
 * The whole string is written in one call; the result is discarded.
 */
void outstr(str)
char *str;
{
    register char *s;                   /* r3 = 0x6563 */

    s = str;                            /* reloaded from 0x6563 after each call */
    write(1, s, strlen(s));
}

/*
 * 0x0221 - log the user in.  Everything that can fail before the shell is
 * started: accounting, the tty's owner and mode, the home directory, the
 * uid, the motd and the mail notice.
 */
void login(name)
char *name;
{
    char mailbuf[BUFSIZ];               /* DE-0x206 */
    register char *u;                   /* r3 = 0x6563 - the parameter's home;
                                           not reloaded: 0x0347 and 0x060c use
                                           their own register variable */

    u = name;
    acct();                             /* 0x0347 - utmp/wtmp accounting */
    lastlogin();                        /* 0x060c - lastlog */
    chdir("/dev");
    chown(ttypath, (pw->gid << 8) | pw->uid);         /* LIB: chown(path, owner) */
    chmod(ttypath, 0622);               /* rw--w--w- */
    if (chdir(pw->dir) < 0) {
        perror(pw->dir);             /* LIB: perror - writes to fd 2 */
        return;
    }
    setuid((pw->gid << 8) | pw->uid);
    type("/etc/motd");                  /* 0x0460 */
    strcpy(mailbuf, maildir);
    strcat(mailbuf, pw->name);
    if (exists(mailbuf))                /* 0x0322 - does the mail file exist? */
        outstr("You have mail.\n");
    reset_signals();                           /* 0x09e5 - flag 0x6080 */
    start_shell(pw->shell);               /* 0x0a39 - exec, using argc/argv */
}

/*
 * 0x0322 - true if the named file exists: stat() succeeded.
 */
exists(name)
char *name;
{
    if (stat(name, &statbuf) < 0)
        return (0);
    return (1);
}

/*
 * 0x0347 - utmp/wtmp accounting, at the start of a session.
 *
 * If neither utmp nor wtmp exists there is nothing to do; otherwise the
 * terminal's name goes into the utmp record, which is appended to wtmp and
 * then written to this tty's own slot in utmp (20 bytes each, seeked to
 * 20 * ttyslot).
 *
 * The fstat() of fd 0 below fills a local that the function never looks at
 * again; it is in the binary, so it is here.
 */
void acct()
{
    struct stat st;                     /* DE-42, never read */

    if (!exists(utmpfile) && !exists(wtmpfile))
        return;
    ttypath = ttyname(0);
    if (ttypath == (char *)0)
        return;
    strcpy(utmp.ut_line, ttypath);
    strcpy(utmp.ut_name, pw->name);
    time(&now);
    utmp.ut_time = now;
    ttyfd = open(wtmpfile, 1);
    seek(ttyfd, 0, 2);                  /* end of wtmp */
    write(ttyfd, &utmp, sizeof utmp);
    close(ttyfd);
    ttyfd = open(utmpfile, 1);
    fstat(0, &st);
    seek(ttyfd, 20 * ttyslot(0), 0);    /* this tty's slot in utmp (0x4e05: 20*n) */
    write(ttyfd, &utmp, sizeof utmp);
    close(ttyfd);
}

/*
 * 0x0460 - copy a file to the standard output, a bufferful at a time.
 * The fd is kept in a char (0x6069), so only its low byte is tested and an
 * fd of 128 or more would be taken for a failure.
 */
void type(name)
char *name;
{
    char buf[BUFSIZ];                   /* DE-0x206 */

    catfd = open(name, 0);
    if (catfd < 0)
        return;
    for (;;) {
        catn = read(catfd, buf, BUFSIZ);
        if (catn <= 0)
            break;
        write(1, buf, catn);
    }
    close(catfd);
}

/*
 * 0x04db - is this a usable login name?
 *   not the empty string, at least one graphic (printable, non-blank)
 *   character, and nothing but letters, digits and '-'.
 */
chkname(name)
char *name;
{
    register char *u;                   /* r3 = 0x6563 */

    u = name;
    if (u == (char *)0)
        return (0);
    if (*u == '\0')
        return (0);
    if (nographic(u))                      /* no graphic character at all */
        return (0);
    if (!alnumdash(u))                     /* something other than [0-9A-Za-z-] */
        return (0);
    return (1);
}

/*
 * 0x0532 - true if the string holds no graphic (printable, non-blank)
 * character: every character is blank, a control character, or >= 0x7f.
 */
nographic(s)
char *s;
{
    register char *u;                   /* r3 = 0x6563 */

    u = s;
    while (*u) {
        if (*u > ' ' && *u < 0x7f)
            return (0);                 /* a graphic character */
        u++;
    }
    return (1);
}

/*
 * 0x0570 - true if the string is empty or every character is a letter, a
 * digit or '-'.
 */
alnumdash(s)
char *s;
{
    register char *u;                   /* r3 = 0x6563 */

    u = s;
    while (*u) {
        if (*u >= '0' && *u <= '9') {
            u++;
            continue;
        }
        if (*u >= 'A' && *u <= 'Z') {
            u++;
            continue;
        }
        if (*u >= 'a' && *u <= 'z') {
            u++;
            continue;
        }
        if (*u == '-') {
            u++;
            continue;
        }
        return (0);
    }
    return (1);
}

/*
 * 0x060c - read this user's lastlog entry, tell the user about the previous
 * login, and record this one.  Uses pw (0x6034) and the record buffer at
 * 0x600c; the record is 20 bytes, {char line[16]; long time;}.
 */
void lastlogin()
{
    long offset;                        /* DE-10 */
    char buf[BUFSIZ];                   /* DE-522 */
    register int fd;                    /* r3 = 0x6563 */

    fd = open(lastlogfile, 2);
    if (fd < 0)
        return;
    offset = lastlogsize * (long)((pw->gid << 8) | pw->uid);    /* the uid's slot */
    lseek(fd, offset, 0);                       /* LIB: lseek(long) */
    read(fd, &lastlog, sizeof lastlog);
    if (lastlog.ll_time != 0) {
        strcpy(buf, "\nLast logged in on ");
        strcat(buf, lastlog.ll_line);
        strcat(buf, ", ");
        strcat(buf, ctime(&lastlog.ll_time));
        outstr(buf);
    } else
        outstr("\nWelcome to Micronix.\n");
    time(&lastlog.ll_time);
    strcpy(lastlog.ll_line, ttyname(0));
    lseek(fd, offset, 0);
    write(fd, &lastlog, sizeof lastlog);
    close(fd);
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */

/* 0x075b - print a string on standard output.
   Exactly write(1, s, strlen(s)); no value of its own is set, so this
   is a void function. */
void writestr(char *s)                              /* H075b */
{
    write(1, s, strlen(s));         /* LIB: write H4821, strlen H3e59 */
}

/* 0x07aa - prompt, read a line, and die on end of file.
   The newline is only emitted when echo was off: the caller turned it
   off for a password, so the user's RETURN was never echoed and the
   cursor is still on the prompt line. */
void promptline(char *prompt, char *buf)            /* H07aa */
{
    writestr(prompt);                               /* 0x075b */
    gets(buf);                                      /* LIB: gets H2062 */
    if (!echo_state)                                /* 0x6000 */
        write(1, "\n", 1);              /* the one-char string at 0x07a8 */
    if (stdin->_flag & (_IOEOF | _IOERR))           /* 0x623d: 020|040 */
        exit(0);                                    /* LIB: exit H29c2 */
}

/* 0x0803 - turn terminal echo back on (used after reading a password).
   The sgttyb it uses is its own at 0x6072.  A failing gtty/stty is
   fatal. */
void echo_on(void)                                  /* H0803 */
{
    if (gtty(0, &ttyb_on) < 0) {    /* LIB: gtty H458e (syscall 32) */
        perror(0);                                   /* LIB: perror H40c0 */
        exit(0);
    }
    ttyb_on.tflags |= ECHO;                         /* 0x6076 |= 010 */
    if (stty(0, &ttyb_on) < 0) {    /* LIB: stty H47e2 (syscall 31) */
        perror(0);
        exit(0);
    }
    echo_state = 1;                                 /* 0x6000 = 1, 0x6001 = 0 */
}

/* 0x085f - turn terminal echo off (done before a password is typed).
   The mirror of 0x0803, with its own sgttyb at 0x6078. */
void echo_off(void)                                 /* H085f */
{
    if (gtty(0, &ttyb_off) < 0) {
        perror(0);
        exit(0);
    }
    ttyb_off.tflags &= ~ECHO;                       /* 0x607c &= 0xF7 */
    if (stty(0, &ttyb_off) < 0) {
        perror(0);
        exit(0);
    }
    echo_state = 0;                                 /* 0x6000 = 0x6001 = 0 */
}

/* 0x08f4 - the password check.  1 if the account needs no password or
   the typed one matches, otherwise "Password incorrect.\n" and 0.

   The single argument is dead in this build.  main passes its line
   buffer, but the code here copies that parameter into a register
   variable (0x6563) and never reads it again; the password is read
   into this function's own 512-byte local (DE-0x206..DE-0x006), below
   the pointer local x (DE-0x208). */
int chkpass(char *ignored)                          /* H08f4 */
{
    char *x;                                    /* DE-0x208 */
    char buf[512];                              /* DE-0x206 */

    if (*pw->passwd == 0)              /* no password: let them in */
        return 1;
    echo_off();                                 /* 0x085f: hide the typing */
    promptline("Password: ", buf);              /* 0x07aa, string at 0x08e9 */
    echo_on();                                  /* 0x0803 */
    x = crypt(buf, pw->passwd);        /* 0x0c37 (login's own) */
    if (strcmp(x, pw->passwd) == 0)    /* LIB: cmpstr H3a0f */
        return 1;
    writestr("Password incorrect.\n");          /* 0x01da, string at 0x08d4 */
    return 0;
}

/* 0x097c - ignore every signal.  One of the first things main does. */
void ignore_signals(void)                           /* H097c */
{
    register int i;                             /* the compiler put it at
                                                   0x607e, see above */

    for (i = 1; i <= 15; i++)
        signal(i, 1);                   /* LIB: signal H463c; 1 = SIG_IGN */
}

/* 0x08c0 - the handler 0x09ab installs.  Not one of the twelve entries:
   it is only ever reached by address.  A signal arriving while login
   waits for a name or a password must not leave the terminal with echo
   off, so it ignores every signal (no re-entry), turns echo back on and
   exits. */
static void catch_handler(void)                     /* H08c0 */
{
    ignore_signals();                           /* 0x097c */
    echo_on();                                  /* 0x0803 */
    exit(0);                                    /* 0x29c2 */
}

/* 0x09ab - catch every signal except 7 with the handler above, so that
   a HUP or an INT while the name is being read restores the terminal. */
void catch_signals(void)                            /* H09ab */
{
    register int i;                             /* 0x607f */

    for (i = 1; i <= 15; i++) {
        if (i == 7)                             /* left alone */
            continue;
        signal(i, catch_handler);               /* 0x08c0 */
    }
}

/* 0x09e5 - hand the shell a clean signal state: everything back to the
   default, except 7, which stays ignored.  Called just before the shell
   is exec'd (0x0221's tail). */
void reset_signals(void)                            /* H09e5 */
{
    register int i;                             /* 0x6080 */

    for (i = 1; i <= 15; i++)
        signal(i, i == 7 ? 1 : 0);              /* 1 = SIG_IGN, 0 = SIG_DFL */
}

/* 0x0a39 - exec the login shell.  The shell field of the passwd entry is
   split into words, each word is copied into fresh memory for argv, and
   argv[0] is replaced by "-" + the basename of the command, the
   convention that marks a shell as a login shell.

   bss-style locals: a 512-byte word buffer at DE-0x606 and the argv
   array at DE-0x406 (0x400 bytes, room for 256 pointers; the loop has
   no bound check).  The path handed to execv is the tokenised first word
   (kept in 0x6083 before argv[0] is overwritten).  On failure the path
   is reported and login exits rather than falling back to a promptless
   state. */
void start_shell(char *sh)                          /* H0a39 */
{
    char buf[512];                              /* DE-0x606 */
    char *argv[256];                            /* DE-0x406 */
    char *sp, **ap, *p;

    if (nographic(sh))                              /* 0x0532: nothing printable */
        sh = "/bin/sh";                         /* the string at 0x0a31 */
    ap = argv;
    for (sp = sh; ; ) {
        sp = getword(sp, buf);                  /* 0x0b77: next word */
        if (buf[0] == 0)                        /* no words left */
            break;
        *ap++ = strsave(buf);                   /* 0x0b2c */
    }
    *ap = 0;
    sh = argv[0];                   /* the command as written in passwd; the
                                       compiled code parks it in 0x6083 */
    for (p = argv[0]; strchr(p, '/') != (char *)0; p++)  /* LIB: instr 0x3b8e */
        ;                                       /* p = the basename */
    strcpy(buf, "-");
    strcat(buf, p);                  /* LIB: 0x3a5a */
    argv[0] = strsave(buf);
    execv(sh, argv);                            /* LIB: execv H4274 */
    perror(sh);                                 /* LIB: perror H40c0 */
    exit(0);
}

/* 0x0b2c - a private copy of a string, in fresh memory (the library's
   strdup).  Returns the copy; malloc's result is not checked here. */
char *strsave(char *s)                              /* H0b2c */
{
    char *p = malloc(strlen(s) + 1);            /* LIB: 0x275d, 0x3e59 */

    strcpy(p, s);                         /* LIB: 0x3a5a */
    return p;
}

/* 0x0b77 - pull the next word out of a string.  Skips leading blanks,
   copies printable characters into buf (stopping at a blank or at any
   byte >= 0x7f), terminates it, and returns the position it stopped at
   so the caller can continue from there. */
char *getword(char *sp, char *buf)                  /* H0b77 */
{
    sp = skipblanks(sp);                        /* 0x0bdb */
    while (*sp > ' ' && *sp < 0x7f)
        *buf++ = *sp++;
    *buf = 0;
    return sp;
}

/* 0x0bdb - skip spaces and tabs, returning the first character that is
   neither.  The parameter is advanced in place, which is how the
   compiler holds the loop variable. */
char *skipblanks(char *p)                           /* H0bdb */
{
    while (*p == ' ' || *p == '\t')
        p++;
    return p;
}

/* ================================================================== *
 * The routines this tree does not have.
 *
 * login was linked against a library that carried these; this tree's
 * libc carries none of them, and the tree's own answer to that is to
 * write them where they are used - owner.c reads /etc/passwd for itself
 * and ps.c defines its own ttyname.  These are written the same way.
 *
 * They are NOT part of login's own code in the binary.  Each one says
 * where it came from in the 1982 image and what was changed.
 * ================================================================== */

/*
 * ttyname - the path of the terminal open on a file descriptor.
 *
 * From 0x3609, with the entry at 0x35e7.  The 1982 routine and this one
 * work the same way, which is not the way a modern ttyname works: it does
 * not ask the kernel, it reads the /dev directory a 16-byte entry at a
 * time and stats each name until one has the device and inode that
 * fstat() reports for the descriptor.
 *
 *   3609  frame                        locals: buf[32], two 36-byte stats
 *   3611  the two results below are cleared
 *   3624  fstat(fd, &dst)              fails -> nothing is found
 *   3639  open("/dev", 0)              fails -> nothing is found
 *   3654  the slot counter starts at 1 - the index of the first entry
 *   3664  read(dirfd, &de, 16)         a short read is the end of /dev
 *   36a1  an entry whose inode is 0 is an empty slot: count it and go on
 *   36aa  strcpy(buf,"/dev/"); strcat(buf, de.name); stat(buf, &st)
 *   36d6  compare four bytes of the two stats - dev at +0, ino at +2 -
 *         in the order 2,3,1,0, which is how the compiler emitted the
 *         equality test.  Equal -> found.
 *   3697  otherwise count the entry and read the next one
 *   370a  close(dirfd) and return what the two results hold
 *
 * The worker is shared.  0x35e7 returns the pointer at 0x63fe, 0x3719
 * returns the counter at 0x63fc - so ttyslot is not a second scan, it is
 * the index the same scan counted on its way to the match.  That is why
 * ttyslot() below ignores the descriptor it is handed: 0x3719 pushes 0
 * and calls 0x3609 itself.
 *
 * The index is one-based and counts every entry in /dev, empty slots
 * included, so it is the raw directory position and not the nth live
 * tty.  login seeks to 20 * ttyslot(0) in /etc/utmp, so this is the
 * program's own idea of which slot a terminal owns; it is reported here
 * as the binary has it, not as it ought to be.
 */

static struct {                 /* the 16-byte directory entry read at 0x63ea */
    short de_ino;               /* +0 */
    char  de_name[14];          /* +2 */
} de;

static int   tty_slot;          /* 0x63fc - 1-based; ttyslot()'s answer */
static char *tty_name;          /* 0x63fe - the found name; ttyname()'s */
static char  tty_dirfd;         /* 0x6400 - the /dev descriptor, kept in a
                                   char, so an fd of 128 or more would be
                                   taken for a failure */

static void
tty_scan(int fd)
{
    struct stat dst, st;
    char buf[32];

    tty_name = (char *)0;
    tty_slot = 0;
    if (fstat(fd, &dst) < 0)
        return;
    tty_dirfd = open("/dev", 0);
    if (tty_dirfd < 0)
        return;
    tty_slot = 1;
    tty_name = de.de_name;
    for (;;) {
        if (read(tty_dirfd, &de, sizeof de) != sizeof de) {
            tty_slot = 0;
            tty_name = (char *)0;
            break;
        }
        if (de.de_ino != 0) {
            strcpy(buf, "/dev/");
            strcat(buf, de.de_name);
            if (stat(buf, &st) >= 0 &&
                st.st_dev == dst.st_dev && st.st_ino == dst.st_ino)
                break;
        }
        tty_slot++;
    }
    close(tty_dirfd);
}

char *
ttyname(fd)
int fd;
{
    tty_scan(fd);
    return (tty_name);
}

int
ttyslot(fd)
int fd;
{
    tty_scan(0);                /* 0x3719 pushes 0, not the argument */
    return (tty_slot);
}

/* ------------------------------------------------------------------ *
 * The /etc/passwd reader that used to sit here - getpwent, setpwent,
 * getpwnam and endpwent, read out of 0x0f0b, 0x1031, 0x1035, 0x1103,
 * 0x115d and 0x119b - is lib/libc/getpwent.c now.  The note it
 * carried explains why it belongs there and not here: in the 1982
 * binary these are one library module and login is only their caller.
 * The addresses and that reasoning moved to the library with the
 * code.
 *
 * What did not move is the storage, because it was never login's:
 * pwf, pwline and pwent were at 0x60c4, 0x60c6 and 0x61c8, and they
 * are the library's statics now.  login's copy of <pwd.h> still
 * describes the same twelve bytes, and login still reads uid and gid
 * out of it together, because setuid and chown here take the owner
 * packed as uid | gid << 8.
 *
 * <pwd.h> declares the four functions now, and the flag-carrying
 * calls below are unchanged.
 */

/* ------------------------------------------------------------------ *
 * crypt - the password hash - used to sit here, from 0x0c37 to
 * 0x0f07, and is lib/libc/crypt.c now.
 *
 * It was always a library routine rather than login's own: its seed
 * strings and its seventeen rounds say so, and the note it carried is
 * with it in the library.  passwd and su want the same hash, so it
 * belongs where they can reach it.  login's only trace of it is the
 * #include <crypt.h> above.
 */

/* ------------------------------------------------------------------ *
 * Where the reconstruction stops, and why.
 *
 * Everything above to the end of getword() is login's own code, in
 * address order, from 015e to 119b.  Above it in the binary's address
 * space - so, not reconstructed here - is the C library that was
 * statically bound into login, and it is libc's, not login's.  The
 * binary is stripped, so the seam is argued from what each routine
 * does: the formatted-output engine at 12d3 is _doprnt, the stdio layer
 * follows it, then the allocator.  The README carries the map in full.
 *
 * That library is NOT the one this file is written against.  login was
 * built with Whitesmiths C, whose library is on the C system disk
 * (disks/dist/1014-8_c_1.IMD) as libwsc.a and libu.a; its own names for
 * the four string helpers login calls are lenstr, cmpstr, cpystr and
 * instr, and the comments above still record which of them each call
 * site was.  This tree has no libwsc.a, so the calls are written against
 * what the tree does have:
 *
 *      lenstr        ->  strlen
 *      cmpstr        ->  strcmp.  Both of the 1982 compares, 0x3a0f and
 *                        0x3bda, return NONZERO when the strings are
 *                        equal and 0 at the first difference - they are
 *                        streq and not strcmp, which is why getpwnam()
 *                        tests == 0 to keep looking.
 *      cpystr        ->  strcpy then strcat.  cpystr concatenated every
 *                        argument in one call; this tree's strcat does
 *                        the same work one string at a time.
 *      instr         ->  strchr
 *
 * and the rest - gets, malloc, exit, perror, ctime, execv, and the
 * syscall wrappers - are libc and libu here by the same names, with two
 * exceptions worth naming because they are easy to miss.  The tree's
 * ctime() takes only the time pointer, so the second argument the binary
 * passed is dropped.  And login's own global that holds the terminal's
 * path cannot be called ttyname when it calls ttyname(), which is why
 * the original library entry is named ttyn above and ttypath below.
 *
 * What is NOT here is the three routines at the end of the file.  In
 * 1982 they were library members - getpwent.o for the passwd reader,
 * and ttyname/ttyslot for the /dev scan - and no library this tree
 * builds carries them.  owner.c reports the same gap for getpwnam and
 * answers it by reading /etc/passwd by hand; ps.c reports it for
 * ttyname and answers it by defining ttyname() itself.  These three are
 * written the same way, and each says at its head where it came from in
 * the 1982 image.
 *
 * With those three in place this file links: ccc -c takes it clean
 * against this tree's headers, and ccc -o produces a binary against
 * libc.a and libu.a with no undefined symbols.  It does not build the
 * 1982 login - see the README's caveats - but it does build.
 */
