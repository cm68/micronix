/*
 * the password routines - crypt and getpass
 *
 * /include/crypt.h
 *
 * One header for the two, where the seventh edition split them:
 * getpass is declared in v7's <stdio.h> and crypt in a <crypt.h> that
 * arrived later.  Neither name had a home here, and a header of its
 * own is a smaller thing to add than a declaration in the tree's
 * stdio.h, which every program includes and which ccc owns.
 *
 * Both are in libc - lib/libc/crypt.c and lib/libc/getpass.c.
 */
char *crypt(char *pw, char *salt);
char *getpass(char *prompt);

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
