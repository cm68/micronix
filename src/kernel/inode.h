/*
 * inode.h 
 */
/*
 * Disk inode structure. See below for the structure
 * used inside the kernel.
 */
struct dsknod
{
    UINT mode;                  /* see below */
    char nlinks;                /* number of directory links */
    UCHAR uid;                  /* user id of owner */
    UCHAR gid;                  /* group id of owner */
    UCHAR size0;                /* high byte of 24-bit size */
    UINT size1;                 /* low word of 24-bit size */
    UINT addr[8];               /* block numbers or device number */
    ULONG rtime;                /* time of last read */
    ULONG wtime;                /* time of last write */
};

/*
 * Internal inode structure.
 * (The stat system calls know the order of the first 36 bytes.)
 */
struct inode
{
    UINT dev;                   /* device number */
    UINT inum;                  /* inumber */

    UINT mode;                  /* see below */
    UCHAR nlinks;               /* number of directory links */
    UCHAR uid;                  /* user id of owner */
    UCHAR gid;                  /* group id of owner */
    UCHAR size0;                /* high byte of 24-bit size */
    UINT size1;                 /* low word of 24-bit size */
    UINT addr[8];               /* block numbers or device number */
    ULONG rtime;                /* time of last read */
    ULONG wtime;                /* time of last write */

    UCHAR flags;                /* see below */
    char count;                 /* count of in-core references */
    struct mount *mount;        /* mounted on this inode */
    ULONG size;                 /* for computational convenience */
    UINT time;                  /* "time" of last reference */
};

/*
 * Mode bits.
 */
#define IALLOC	0100000         /* inode is allocated */
#define ITYPE	 060000         /* 2-bit file type mask */
#define	 IIO	 020000         /* mask to identify io files */
#define	 IBIO	 060000         /* block-type io file */
#define	 ICIO	 020000         /* char-type io file */
#define	 IDIR	 040000         /* directory */
#define	 IORD	 000000         /* ordinary file */
#define ILARGE	 010000         /* large-file addressing */
#define ISETUID	  04000         /* set user-id on execution */
#define ISETGID	  02000         /* set group-id on execution */
#define I1WRITE	  01000         /* allow only one writer */
#define IOWNER	   0700         /* owner permisions mask */
#define IGROUP	    070         /* group permissions mask */
#define IOTHER	     07         /* other permissions mask */
#define	 IREAD	     04         /* read permission */
#define	 IWRITE	     02         /* write permission */
#define	 IEXEC	     01         /* execute permission */

/*
 * Flag bits
 */
#define IBUSY	1               /* changing: do not access */
#define IMOD	2               /* inode has been modified */
#define IWANT	4               /* wakeup address of inode on release */
#define IRONLY	8               /* device is mounted read-only */
#define IWRLOCK 16              /* do not open for writing */
#define IPIPE	32              /* delay disk write of full blocks */
/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
