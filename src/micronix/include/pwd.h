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
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
