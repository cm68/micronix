/*
 * it's a good idea to keep this below 4k, to prevent
 * any and all kinds of overlapping load insanity
 *
 * the global variables and register variables are to
 * get the footprint below 4k.
 */
#include <types.h>
#include <sys/fs.h>
#include <sys/dir.h>
#include <obj.h>

#ifdef __STDC__
#define	INIT
#else
#define	INIT = 0
#endif
int inumber = 1;
struct dsknod *inode INIT;

/*
 * disk buffers - we need at most 2 to be valid at one time
 * we care about this for space reasons.
 */
union diskbuf {
	struct dsknod ibuf[16];
	struct obj obj;
	UINT16 indir[256];
	struct dir dir[32];
	char bytes[512];
};

union diskbuf disk0 INIT;
union diskbuf disk1 INIT;

#define	bytebuf		disk0.bytes
#define	objbuf		disk0.obj
#define	indirbuf	disk1.indir

#define	inodebuf	disk0.ibuf
#define	dirbuf		disk1.dir

char inputbuf[15] INIT;

int (*loadbase)() = 0x1000;
int loadptr = 0x1000;
int loadsize = 0;

char filecount = 0;
int found = 0;

exit()
{
	int (*loadbase)();
	loadbase = 0;
	(*loadbase)();
}

/*
 * given an inode that has been read, read the executable into memory
 * we require that the inode be IFREG and ILARG, > 4k
 * buffers that need to be valid: indirbuf and objbuf.
 */
load()
{
	register int *bnum;

	iget();

	if (!readblock(inode->d_addr[0], indirbuf)) {
		outstr("read indir failed");
		exit();
	}
	bnum = indirbuf;

	if (!readblock(*bnum, &objbuf)) {
		outstr("read header failed");
		exit();
	}
	if (objbuf.ident == OBJECT) {
		loadbase = objbuf.textoff;
		loadptr = objbuf.textoff - 0x10;
		loadsize = objbuf.text + objbuf.data + 0x10;
	} else {
		loadsize = inode->d_size1;
	}
	outstr("Loading\n");
	while (loadsize > 0) {
		if (!readblock(*bnum, loadptr)) {
			outstr("read object failed");
			exit();
		}
		bnum++;
		loadptr += 512;
		loadsize -= 512;
	}
	outstr("Entering\n");
	out(0x41, 0);
	(*loadbase)();
}

#ifdef __STDC__
outstr(char *s)
#else
outstr(s)
register char *s;
#endif
{
	while (*s) {
		if (*s == '\n') {
			conout('\r');
		}
		conout(*s++);
	}
}

/*
 * set inumber to the file we want to boot.  if there is only one,
 * use it.
 *
 * XXX - only support first 32 files in the root directory
 * wouldn't be hard to fix, but probably not worth it
 * buffers in use:  directory and inode
 */
select()
{
	register struct dir *dirp;

    iget();

    if (!readblock(inode->d_addr[0], dirbuf)) {
		outstr("read directory failed\n");
		exit();
    }

    outstr("Files:\n");
	for (dirp = dirbuf; dirp < &dirbuf[32]; dirp++) {
		if ((dirp->name[0] != '.') && (dirp->ino != 0)) {
			inumber = dirp->ino;
			iget();
			if ((inode->d_mode & (IALLOC|ILARG|IFMT)) != (IALLOC|ILARG))
				continue;
			outstr(dirp->name);
			outstr("\n");
			filecount++;
			found = inumber;
		}
   	} 
	inumber = found;

    if (filecount == 0) {
        outstr("No bootable files\n");
        exit();
    }

    if (filecount != 1) {
        while (1) {
            outstr("File to boot: ");
            readline();
            dirp = dirbuf;
    		for (dirp = dirbuf; dirp < &dirbuf[32]; dirp++) {
				if (strcmp(dirp->name, inputbuf) == 0) {
					inumber = dirp->ino;
					return;
				}
            }
            outstr("File not found\n");
        }
    }
}

/*
 * read the inode inumber, and point at it.
 */
iget()
{
	if (!readblock(2 + (inumber / 32), inodebuf)) {
		outstr("inode read failed\n");
	}
	inode = &inodebuf[inumber % 32];
}

#ifdef __STDC__
conout(UINT8 a)
#else
conout(a)
UINT8 a;
#endif
{
    out(0x4f, 1);
	/* wait for txempty */
	while (!(in(0x4d) & 0x20))
		;
    out(0x48, a);
}

UINT8
conin()
{
	/* wait for rxready */
	while (!(in(0x4d) & 0x1))
		;
	return in(0x48);
}

readline()
{
	register char *s;
	char c;

top:
    s = &inputbuf;

	while (s < (&inputbuf[sizeof(inputbuf)] - 1)) {
		*s = '\0';
		c = conin();
		if (c == '\b') {
			if (s != &inputbuf) {
				s--;
				outstr("\b \b");
			}
			continue;
		}
		if (c == '\n' || c == '\r') {
			return;
		}
		if (c == 0x18) {
			outstr("\n");
			goto top;
		}
        if ((c == 0x3) || (c == 0x7f)) {
        	exit();
        }
        conout(c);
        *s++ = c;
    }
}

main()
{
    reset();
    select();
    load();
}
