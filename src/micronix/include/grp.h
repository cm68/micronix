/*
 * presumably, this is returned by some library function that reads
 * the /etc/group file
 *
 * /include/grp.h
 *
 * The sibling of pwd.h, and deliberately not shaped like it: a
 * group's gid is an int here and not one of struct passwd's single
 * bytes, because /etc/group holds the gid in full and chgrp reads it
 * that way.  The one byte in struct passwd is what the password file
 * and the chown() packing have room for, and is not a property of
 * group ids.
 */
struct group {
	char *gr_name;
	char *gr_passwd;

	int gr_gid;

	char **gr_mem;
};

/*
 * lib/libc/getgrent.c.  The record returned is in a static area the
 * next call overwrites - copy anything to be kept.
 */
struct group *getgrent();
struct group *getgrgid();
struct group *getgrnam();
int setgrent();
int endgrent();

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
