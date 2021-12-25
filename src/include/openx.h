/*
 * a simple library to manage an xterm instance that the simulator can access
 *
 * openx.h
 * Changed: <2021-12-23 15:35:26 curt>
 */
void
open_terminal(char *name, int signum, int *in_p, int *out_p, int cooked, char *logfile);

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
