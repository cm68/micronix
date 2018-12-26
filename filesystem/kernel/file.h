       /*
	* System file list structure
	*/
struct file
	{
	UCHAR		mode;	/* IREAD, IWRITE, PIPE == IEXEC */
	char		count;
	struct inode *	inode;
	ULONG		rwptr;
	}
	flist[];

#define PIPE	IEXEC		/* must include inode.h before this */

