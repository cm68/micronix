/*
 * presumably, this is returned by some library function that reads
 * the /etc/passwd file
 */
struct passwd {
	char	*name;
	char	*passwd;

	UINT8	uid;
	UINT8	gid;

	char	*person;
	char 	*dir;
	char	*shell;
};
