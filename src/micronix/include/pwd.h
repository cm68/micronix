/*
 * presumably, this is returned by some library function that reads
 * the /etc/passwd file
 *
 * /include/pwd.h
 *
 * Changed: <2021-12-23 15:12:53 curt>
 */
struct passwd {
	char *name;
	char *passwd;

	unsigned char uid;
	unsigned char gid;

	char *person;
	char *dir;
	char *shell;
};

/*
 * lib/libc/getpwent.c.  The record returned is in a static area the
 * next call overwrites - copy anything to be kept.
 */
struct passwd *getpwent(void);
struct passwd *getpwuid(int);
struct passwd *getpwnam(char *);
int setpwent(void);
int endpwent(void);

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
