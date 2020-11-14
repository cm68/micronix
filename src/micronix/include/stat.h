struct stat
	{
	UTINY	minor,
		major;

	UCOUNT	inumber,
		flags;

	UTINY	nlinks,
		uid,
		gid,
		size0;

	UCOUNT	size1;

	UCOUNT	addr[8];

	ULONG	actime,
		modtime;
	};


# define S_ALLOC	0100000
# define S_TYPE		0060000
# define S_PLAIN	0000000
# define S_ISDIR	0040000
# define S_ISCHAR	0020000
# define S_ISBLOCK	0060000
# define S_LARGE	0010000
# define S_SUID		0004000
# define S_SGID		0002000
# define S_STICKY	0001000
# define S_PERM		0000777
