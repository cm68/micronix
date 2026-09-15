/*
 * The second level of the hard disk boot.
 *
 *
 * 4k is a hard limit, not a preference
 * ------------------------------------
 *
 * Everything this file has - text, data and bss together - must end
 * below 0ff0, and there is nowhere else for it to go.
 *
 * The kernel loads at 1000 and needs the whole of memory from there up.
 * It cannot start any lower: the supervisor's first physical 4k page is
 * the rom and the I/O space, so 0000 through 0fff is not memory the
 * kernel can have.  That leaves the loader the same 4k the rom occupies
 * once the map is switched, and nothing above it, because everything
 * above it is about to be kernel.
 *
 * There is no way around this and no bigger number to move to.  The
 * limit is not the loader's size, it is where memory begins for the
 * supervisor and where the kernel has to start - neither of which this
 * code gets a say in.  Anything that will not fit in the 4k has to be
 * taken out of the loader, not moved somewhere else.
 *
 * So the loader is not merely tidier when it is small.  Anything of it
 * that reaches 0ff0 is written over by the first block of the load, and
 * whatever it was doing with that memory stops working part way through
 * - which is how it presents: not a failure to load, but a load that
 * seems to go fine and then enters nothing.  Both of the faults that
 * this file has had were that, wearing different clothes:
 *
 *	the block list left in the buffer, which straddles 0ff0, so the
 *	load overwrote the list of blocks it was reading
 *
 *	the buffers declared "= 0", which reserved two bytes each and so
 *	hid the problem by not being big enough to reach
 *
 * The current build ends at 0fba, with 54 bytes to spare.  If that runs
 * out, the things worth taking are in this order: the library, which is
 * why sexit.s exists at all; then readline and the file chooser, which
 * only earn their keep when a disk has more than one bootable file.
 *
 * The global and register variables are here for the same reason.
 */
#include <types.h>
#include <sys/fs.h>
#include <sys/dir.h>
#include <obj.h>

/*
 * Whitesmith's will not link a bss symbol without an explicit
 * initialiser, and ccc is happy either way, so everything here says
 * what it starts as.
 */
#define	INIT = 0
int inumber = 1;
struct dsknod *inode INIT;

/*
 * Two buffers, pointed at, not in the loader's own 4k.
 *
 * The loader's code and its block list have to stay below 0ff0 - the
 * kernel loads there and everything above is about to be kernel.  The
 * buffers are only needed while the loader runs, though: by the time
 * the kernel load writes over them they are done with.  So they live
 * above the kernel's load address, at fixed addresses, and cost the
 * loader nothing.
 *
 * buf0 is the working buffer - the directory, then the indirect block,
 * then the object header.  buf1 is the inode buffer: iget() reads into
 * it while the directory is still in buf0.
 */
union diskbuf {
	struct dsknod ibuf[16];
	struct obj obj;
	UINT16 indir[256];
	struct dir dir[32];
	char bytes[512];
};

#define BUF0	0x1000
#define BUF1	0x1200
#define buf0	(*(union diskbuf *) BUF0)
#define buf1	(*(union diskbuf *) BUF1)

/*
 * The block list, in the image - it must survive the load, so it
 * cannot live in a buffer the kernel writes over.  select() fills it
 * and load() walks it.  An indirect block holds 256 numbers, but a
 * file that fits in a 64k address space cannot be more than 128
 * blocks, so that is the ceiling and one too big is refused rather
 * than quietly truncated.
 */
#define NBLIST	128
UINT16 blist[NBLIST];

/*
 * readline's buffer, above the load address too: it is used only while
 * select() is choosing a file, before the load.
 */
#define INPUTSIZE 15
#define inputbuf ((char *) 0x1400)

int (*loadbase)() = 0x1000;

/*
 * Unsigned, both of them.  A kernel of 45013 text and 14604 data comes
 * to 59633, which does not fit in a signed 16 bit int - it reads as
 * -5903, "while (loadsize > 0)" is false at once, and the loader
 * announced "Loading" and then "Entering" having loaded nothing.
 */
char *loadptr = (char *) 0x1000;
UINT16 loadsize = 0;

char filecount = 0;
int found = 0;

bail()
{
	int (*loadbase)();
	loadbase = 0;
	(*loadbase)();
}

/*
 * load the file select() chose, now that its block list is in blist.
 * The first block is the object header; it says how big the file is
 * and where it wants to go.  The rest are the kernel.
 */
load()
{
	register int i;
	int nblk;

	if (!readblock(blist[0], buf0.bytes)) {
		outstr("read header failed");
		bail();
	}
	if (buf0.obj.ident == OBJECT) {
		loadbase = buf0.obj.textoff;
		loadptr = (char *) (buf0.obj.textoff - 0x10);
		loadsize = buf0.obj.text + buf0.obj.data + 0x10;
	} else {
		loadsize = inode->d_size1;
	}

	/*
	 * Count the blocks up front - counting down through loadsize
	 * would have to go negative to stop, which is what it cannot do.
	 */
	nblk = (loadsize + 511) / 512;
	if (nblk > NBLIST) {
		outstr("boot file is too big\n");
		bail();
	}

	outstr("Loading\n");
	for (i = 0; i < nblk; i++) {
		if (!readblock(blist[i], loadptr)) {
			outstr("read object failed");
			bail();
		}
		loadptr += 512;
	}
	outstr("Entering\n");
	out(0x41, 0);
	(*loadbase)();
}

outstr(s)
register char *s;
{
	while (*s) {
		if (*s == '\n') {
			conout('\r');
		}
		conout(*s++);
	}
}

/*
 * set inumber to the file we want to boot, and build its block list.
 * if there is only one, use it.
 *
 * XXX - only support first 32 files in the root directory
 * wouldn't be hard to fix, but probably not worth it
 *
 * The directory is read into buf0 and stays there while each
 * candidate's inode is read into buf1 to test for a bootable file.
 * Once one is chosen, its inode and first indirect block are read and
 * the block list copied into blist for load().
 */
select()
{
	register struct dir *dirp;
	register int i;

	iget();

	if (!readblock(inode->d_addr[0], buf0.bytes)) {
		outstr("read directory failed\n");
		bail();
	}

	outstr("Files:\n");
	for (dirp = buf0.dir; dirp < &buf0.dir[32]; dirp++) {
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
		bail();
	}

	if (filecount != 1) {
		for (;;) {
			outstr("File to boot: ");
			readline();
			for (dirp = buf0.dir; dirp < &buf0.dir[32]; dirp++) {
				if (strcmp(dirp->name, inputbuf) == 0) {
					inumber = dirp->ino;
					goto picked;
				}
			}
			outstr("File not found\n");
		}
	}

picked:
	/*
	 * Build the block list.  The chosen inode is read again (buf1),
	 * its first indirect block is read into buf0 - the directory is
	 * done with - and the first 128 block numbers are copied into
	 * blist, which the load will not reach.
	 */
	iget();
	if (!readblock(inode->d_addr[0], buf0.bytes)) {
		outstr("read indir failed");
		bail();
	}
	for (i = 0; i < NBLIST; i++)
		blist[i] = buf0.indir[i];
}

/*
 * read the inode inumber, and point at it.
 */
iget()
{
	/*
	 * Sixteen inodes to a block, not thirty two: struct dsknod is 32
	 * bytes and a block is 512, which is why the buffer above is
	 * declared ibuf[16].  With 32 this read the wrong block for any
	 * inode past the fifteenth and then indexed off the end of the
	 * buffer.
	 *
	 * And inodes are numbered from one, so the root is the first
	 * entry of block 2 and inode n is n-1 into the ilist.  Without
	 * the -1 every inode read the one after it: asking for the root
	 * returned /boot, whose block list then led somewhere that is not
	 * a directory.  mkfs computes it the same way, in getdsk.
	 */
	if (!readblock(2 + ((inumber - 1) / 16), buf1.bytes)) {
		outstr("inode read failed\n");
	}
	inode = &buf1.ibuf[(inumber - 1) % 16];
}

conout(a)
UINT8 a;
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
    s = inputbuf;

	while (s < (&inputbuf[INPUTSIZE] - 1)) {
		*s = '\0';
		c = conin();
		if (c == '\b') {
			if (s != inputbuf) {
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
        	bail();
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
