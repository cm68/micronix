/*
 * this file is the hdc-dma boot code for micronix.
 *
 * it's a good idea to keep this below 4k, to prevent
 * any and all kinds of overlapping load insanity
 *
 * the global variables and register variables are to
 * get the footprint below 4k.
 */
#include <types.h>
#include <sys/mw.h>
#include <sys/fs.h>
#include <sys/dir.h>
#include <obj.h>

struct drivespec {
	UINT cylinders;
	UINT8 heads;
	UINT8 spt;			/* sectors per track */
	UINT limit;			/* max block number */
	UINT8 spc;			/* sectors per track */
} spec = {
	153, 4, 17, (153 * 4 * 17) -1, 4 * 17		/* st506 */
};

#define	STEPDELAY	30
#define	SETTLE		100

/*
 * whitesmith's stupidity means that BSS symbols don't link unless they
 * have an explicit initializer.
 */
#ifndef __STDC__
#define	INIT	= 0
#endif

char tries INIT;
int curcyl INIT;
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

struct hddma_cmd cmd INIT;

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
 * reset the drive.
 * we do something a bit clever here:
 * assume a slow drive.  assume 17 sectors/track.
 * do a read header on each of the 8 possible heads to
 * probe the possible heads.
 */
reset()
{
	register int i;

    outstr("Micronix loader for the HD-DMA\n");

	*(int *)0x5d = &cmd;
	*(char *)0x5f = 0;

    out(HDC_RESET, 0);

	cmd.steps = 0;
    cmd.byte0 = 0;
    cmd.byte1 = STEPDELAY;
    cmd.byte2 = SETTLE;
    cmd.byte3 = SEC512;
	cmd.link = &cmd;
	cmd.xlink = 0;

    hdc_command(OP_LOAD);

    cmd.drvsel = 0;
    cmd.headsel = 0;
    hdc_command(OP_SENSE);

    if (cmd.status & SENSE_READY) {
		outstr("Drive not ready\n");
		exit();
	}

	/* recalibrate */
	cmd.steps = 0xffff;
    cmd.drvsel |= STEPOUT;
    hdc_command(OP_NOP);

	/* let's see if we can find out how many heads we have */
	for (i = 0; i < 8; i++) {
		cmd.headsel = (~i & 7) << 2;
		cmd.dma = (UINT)bytebuf;
		cmd.xdma = 0;
		tries = 0;
		while (1) {
			bytebuf[4] = 0xff;
			if (!hdc_command(OP_HEADER)) {
				if (tries++ > 10) goto probedone;
			}
			if (bytebuf[4] == i) {
				break;
			}
		}
		outstr("head\n");
	}
probedone:
	spec.heads = i;
	spec.spc = i * spec.spt;
}

/*
 * given a block number, read the contents into the buffer
 * return 1 for success
 */
int
#ifndef __STDC__
readblock(blocknum, buffer)
int blocknum;
char *buffer;
#else
readblock(int blocknum, char *buffer)
#endif
{
	register int cyl;
	int secnum;
	int trknum;
	int head;

	if (blocknum > spec.limit) {
        outstr("Block out of range\n");
		return 0;
    }

	secnum = blocknum % spec.spc;
	trknum = blocknum / spec.spc;
	cyl = trknum / spec.heads;
	head = trknum % spec.heads;

	if (curcyl < cyl) {
		cmd.steps = cyl - curcyl;
		cmd.drvsel = 0;
	} else {
		cmd.steps = curcyl - cyl;
		cmd.drvsel = STEPOUT;
	}
	curcyl = cyl;
	cmd.word0 = cyl;
	cmd.byte2 = head;
	cmd.byte3 = secnum % spec.spt;
    cmd.headsel = (~cmd.byte2 & 7) << 2;
	cmd.dma = (UINT)buffer;
	cmd.xdma = 0;

	tries = 0;
	while (tries++ < 10) {
		if (hdc_command(OP_READ)) {
			return 1;
		}
        outstr("retry\n");
    }
    outstr("Read error\n");
    return 0;
}

/*
 * run a hdc command and wait for a response
 */
int 
#ifdef __STDC__
hdc_command(UINT8 opcode)
#else
hdc_command(opcode)
UINT8 opcode;
#endif
{
    cmd.opcode = opcode;
    cmd.status = 0;
    out(HDC_ATTN, 0);
	while (cmd.status == 0)
		;
    if (cmd.status == 0xff) {
		return 1;
	} else {
		return 0;
	}
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
