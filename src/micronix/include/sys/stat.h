/*
 * in-core data structure for stat and fstat calls
 *
 * include/sys/stat.h
 * Changed: <2021-12-23 14:36:34 curt>
 * 
 * v6 did not have a well-defined system interface contract, where utilities
 * like ls.c had their own copy of the stat structure in their code. needless
 * to say, this is not great software engineering.  in fact, v7 was the first
 * version where the notion of include files as an interface contract was
 * implemented to any great deal.
 * 
 * we adopt this model with enthusiasm; there's one source of truth, and it's in
 * the include files.
 *
 * anybody that includes this also needs to include sys/fs.h
 * to get struct dsknod
 */

/*
 * this structure is passed to the kernel when asking about a file 
 * micronix has the dev_t unpacked, v7+ wants it packed
 */
struct stat {
    union {
        struct {
            UINT8 minor_b;
            UINT8 major_b;
        }      dev_b;
        UINT dev_s;
    }     dev_u;
    UINT ino;
    struct dsknod d;
};

/*
 * these are all used as access macros to get at this composite type as a
 * bonus, they are also the v7 fields, so v7 utilities port much easier
 */
#define	st_dev		dev_u.dev_s
#define	st_ino		ino
#define	st_mode		d.d_mode
#define	st_nlink	d.d_nlink
#define	st_uid		d.d_uid
#define	st_gid		d.d_gid
#define	st_size0	d.d_size0
#define	st_size1	d.d_size1
#define	st_addr		d.d_addr
#define	st_rtime	d.d_atime
#define	st_mtime	d.d_mtime

/*
 * v7 names the fields differently, maybe they didn't trust their c compiler
 * to deal with 2 structures having the same name with different offsets.
 * struct elements are each different namespaces, so this isn't required by
 * c.
 * 
 * also, note that v7 has a 32 bit file size, and reuses S_LARGE to define some
 * new file types.  porting will need to deal with this.
 */

#define S_ALLOC		0100000 /* inode is allocated */

#define S_IFMT		0060000 /* inode type */
#define S_IFREG		0000000 /* a file */
#define S_IIO		0020000 /* io nodes have this set */
#define S_IFCHR		0020000 /* cdev */
#define S_IFDIR		0040000 /* directory */
#define S_IFBLK		0060000 /* bdev */

#define S_LARGE		0010000 /* large file addressing */
#define S_ISUID		0004000 /* set uid */
#define S_ISGID		0002000 /* set gid */
#define S_1WRITE	0001000 /* exclusive write (!) */

#define S_PERM		0000777 /* permissions masks */

#define	IOWNER		0000700
#define	IGROUP		0000070
#define	IOTHER		0000007

#define	IREAD			04
#define	IWRITE			02
#define	IEXEC			01

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
