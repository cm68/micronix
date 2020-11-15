struct stat {
	
	/*
	 * micronix has the dev_t unpacked, v7+ wants it packed
	 */
	union {
		struct {
			unsigned char minor_b;
			unsigned char major_b;
		} dev_b;
		unsigned short dev_s;
	} dev_u;
#define	minor	dev_b.minor_b
#define	major	dev_b.major_b

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

/*
 * compatibility macros for v7+
 */
#define	S_IFMT		S_TYPE
#define	S_IFDIR		S_ISDIR

#define	st_dev		dev_u.dev_s
#define	st_ino		inumber
#define	st_mode		flags
#define	st_ino		inumber
