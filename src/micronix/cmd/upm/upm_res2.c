/*
 * upm_res2.c - the rest of upm's resident half.
 *
 * upm_res.c is split here because the whole file overflows c0's 64K
 * on the machine that has to rebuild upm.  Each unit is a separate c0
 * run; the makefile folds both into the resident text segment in this
 * order, so the functions keep the order the disassembly gave them.
 * upm.h carries the prototypes and the resident globals both units
 * name.
 */

#include "upm.h"

/*
 * cname - a micronix name into the FCB's name and type.
 *
 * The name up to the dot goes into the eight-byte name field, the three
 * letters after it into the type field, both uppercased and space-padded.
 * The drive byte is left alone - search writes it after this returns.
 */
cname(src, dst)
char *src;
char *dst;
{
	int i;
	char *dot;

	dot = src + strlen(src) - 1;
	while (dot >= src && *dot != '.')
		dot--;
	if (dot < src)
		dot = 0;

	memset(dst + 1, ' ', 11);

	i = 0;
	while (i < 8 && *src && src != dot) {
		dst[1 + i] = uc(*src & 0x7F);
		src++;
		i++;
	}

	if (dot) {
		src = dot + 1;
		i = 0;
		while (i < 3 && *src) {
			dst[9 + i] = uc(*src & 0x7F);
			src++;
			i++;
		}
	}
}

/*
 * lc / uc - the two case folds.  Whitesmith's has both, and upm calls
 * them by name, so they are reproduced as the two functions they are.
 */
lc(c)
int c;
{
	if (c >= 'A' && c <= 'Z')
		return c + 0x20;
	return c;
}

uc(c)
int c;
{
	if (c >= 'a' && c <= 'z')
		return c - 0x20;
	return c;
}

/*
 * clean - strip the high bit off every byte up to the nul.
 *
 * The CP/M R/O attribute and the wildcard marker ride the high bit of
 * the name and type characters, so they are cleared before the name is
 * used as a micronix pathname.
 */
clean(p)
char *p;
{
	while (*p) {
		*p &= 0x7F;
		p++;
	}
}

/*
 * isunique - does the FCB's name hold no '?' wildcard.  The thirteen
 * bytes from dr through ex are the ones a name can put a '?' in.
 */
isunique(fcb)
char *fcb;
{
	int i;

	for (i = 13; i != 0; i--) {
		if ((*fcb & 0x7F) == '?')
			return 0;
		fcb++;
	}
	return 1;
}

/*
 * fexists - does this pathname name a file that is there.
 */
fexists(name)
char *name;
{
	if (stat(name, statbuf) == 0)
		return 1;
	return 0;
}

/*
 * match - the CP/M wildcard match.
 *
 * The pattern is an FCB whose name has been turned into a micronix
 * pathname by name - lowercased, with '?' wildcards and the type after
 * the dot.  The entry is a directory entry, whose name sits two bytes in
 * after the inode.  '?' matches one character but not the dot and not
 * the end; an uppercase letter in the entry cannot match, because name
 * lowercased the pattern.  The two strings have to end together - a
 * match is exact, not a prefix of one side.
 *
 * strcmp returns 0 on equal, so the test below reads it that way.
 */
match(entry, pattern)
char *entry;
char *pattern;
{
	char *e;
	char *p;
	char ec, pc;

	e = entry + 2;
	name(pattern, buf);
	p = buf;

	while (strchr(p, '/'))
		p++;

	for (;;) {
		ec = *e;
		if (ec >= 'A' && ec <= 'Z')
			return 0;

		pc = *p;
		if (pc == '?') {
			if (ec != 0 && ec != '.')
				e++;
			p++;
			continue;
		}

		if (ec == 0) {
			if (strcmp(p, "") == 0)
				return 1;	/* both ended: a match */
			return 0;		/* entry ended, pattern continues */
		}

		if (ec != pc)
			return 0;

		e++;
		p++;
	}
}

/*
 * cread - the one read path, sequential and random both.
 *
 * cseek positions the file, then a 128-byte record is read into the
 * DMA buffer.  A short read is padded with ^Z and the random record is
 * reset, which is how CP/M marks the end of a text file.  Returns 0 for
 * a record, 1 at end of file, -1 on error - and closes the file when the
 * read runs dry or has already given the last record.
 */
cread(fn, fcb)
int fn;
struct fcb *fcb;
{
	int n;

	cseek(fn, fcb);
	n = read(fcb->fd, dma, 0x80);

	if (n <= 0 || fcb->size - 1 == fcb->currec) {
		closefile(fcb->fd);
		fcb->fd = -1;
	}

	if (n < 0)
		return -1;
	if (n == 0)
		return 1;

	advance(fn, fcb);
	if (n != 0x80) {
		memset(dma + n, 0x1A, 0x80 - n);	/* pad with ^Z */
		fcb->currec = 0;
	}
	return 0;
}

/*
 * advance - advance the record pointer after a read or write.
 *
 * Sequential mode walks cr up and carries into ex every 128 records;
 * random mode derives ex and cr from currec.  Either way currec advances,
 * which is what keeps the random record in step with the sequential one.
 */
advance(fn, fcb)
int fn;
struct fcb *fcb;
{
	if (fn == 'S') {
		fcb->cr++;
		if (fcb->cr == 0x80) {
			fcb->cr = 0;		/* moving into a new extent */
			fcb->ex++;
			setrc(fcb);
		}
	} else {				/* random I/O */
		fcb->ex = fcb->currec >> 7;	/* next bits */
		fcb->cr = fcb->currec & 0x7F;	/* low 7 bits */
		setrc(fcb);
	}
	fcb->currec++;
}

/*
 * flushpending - write the deferred record back to its file as a full
 * 128-byte record.  Called before the next record write or read, and by
 * cclose for the final - possibly short - record.
 */
void
flushpending()
{
	if (!pending_valid)
		return;
	seek(pending_fcb->fd, pending_rec >> 2, 3);
	if (pending_rec & 3)
		seek(pending_fcb->fd, (pending_rec & 3) << 7, 1);
	write(pending_fcb->fd, pending, 0x80);
	pending_valid = 0;
}

/*
 * flushbyte - flush fcb's deferred record with its last-record byte count.
 *
 * Called by cclose and before a size query.  bc is initialized from stat
 * at open, cleared to "full" when a write passes the old end, and set from
 * cr by setatt when the F6' bit is set; 0 means the record is full.
 */
void
flushbyte(fcb)
struct fcb *fcb;
{
	int nbytes;

	if (!pending_valid || pending_fcb != fcb)
		return;
	seek(fcb->fd, pending_rec >> 2, 3);
	if (pending_rec & 3)
		seek(fcb->fd, (pending_rec & 3) << 7, 1);
	nbytes = fcb->bc ? fcb->bc : 0x80;
	write(fcb->fd, pending, nbytes);
	pending_valid = 0;
}

/*
 * cwrite - the one write path, sequential and random both.
 *
 * Like cread but in the other direction: seek, write a full 128-byte
 * record from the DMA buffer, advance, and grow the size when the write
 * went past the old end.  A short write is an error.  The size field
 * tracks currec rather than bytes here, which is the odd thing the binary
 * does and is reproduced as such.
 */
cwrite(fn, fcb)
int fn;
struct fcb *fcb;
{
	int rec;

	rec = (fn == 'S') ? ((fcb->ex << 7) | fcb->cr)
			  : ((fcb->r1 << 8) | fcb->r0);

	if (rec >= fcb->size)		/* past the old end: new full record */
		fcb->bc = 0;

	flushpending();
	memcpy(pending, dma, 0x80);
	pending_valid = 1;
	pending_fcb = fcb;
	pending_rec = rec;

	advance(fn, fcb);

	if (fcb->currec > fcb->size) {
		fcb->size = fcb->currec;
		fcb->nrec = fcb->size >> 7;
		if (fcb->size & 0x7F)
			fcb->nrec++;
		setrc(fcb);
	}
	return 0;
}

/*
 * cseek - position the file for the record about to be read or written.
 *
 * Flushes both output buffers first, so the disk sees a seek in the same
 * order the console saw the bytes.  A closed FCB is opened.  The record
 * number is worked out of the sequential ex/cr or the random r0/r1, and
 * turned into a micronix seek: the micronix seek counts 512-byte blocks
 * from the start and bytes from where that leaves you, which is why the
 * record number is split into >> 2 and (& 3) << 7.
 */
cseek(fn, fcb)
int fn;
struct fcb *fcb;
{
	int off;

	flushpending();

	if (oleft)
		cflush();
	if (lleft)
		lflush();

	if (fcb->fd < 0) {
		name(fcb, buf);
		fcb->fd = openfile(buf);
		fcb->currec = 0;
		setrc(fcb);
	}

	if (fn == 'S')
		off = (fcb->ex << 7) | fcb->cr;
	else {
		if (fcb->r2)
			return 0;	/* the high byte is out of this TPA's range */
		off = (fcb->r1 << 8) | fcb->r0;
	}

	if (off != 0 && off == fcb->currec)
		return 0;		/* already there */

	seek(fcb->fd, off >> 2, 3);
	if (off & 3)
		seek(fcb->fd, (off & 3) << 7, 1);
	fcb->currec = off;
	return 0;
}

/*
 * csize - BDOS function 35, compute the file size.
 *
 * Puts the file's size, in 128-byte records, into the three CP/M random
 * record bytes.  fsize does the stat and the division; this just names
 * the file and copies the answer into r0/r1.
 */
csize(fcb)
struct fcb *fcb;
{
	int size;

	flushbyte(fcb);		/* a deferred record is part of the size */
	name(fcb, buf);
	size = fsize(buf);
	fcb->r0 = size;
	fcb->r1 = size >> 8;
	fcb->r2 = 0;
}

/*
 * fsize - the size of a file in records.
 *
 * Stats the file and turns the byte count into a record count, rounded
 * up.  The size is a 24-bit little-endian count in the stat buffer at
 * offset 9; the record count is the top byte times 512 plus the bottom
 * two bytes over 128, rounded up when the low seven bits are set.
 */
fsize(name)
char *name;
{
	int nrec;

	if (stat(name, statbuf) < 0)
		return 0;

	nrec = (statbuf[9] << 9) | ((statbuf[10] | (statbuf[11] << 8)) >> 7);
	if ((statbuf[10] | (statbuf[11] << 8)) & 0x7F)
		nrec++;
	return nrec;
}

/*
 * setrand - BDOS function 36, set the random record.
 *
 * The sequential position ex/cr is folded into the 16-bit random record
 * r0/r1.  This is the inverse of what cseek's sequential branch does
 * when it spreads the same record number back out.
 */
setrand(fcb)
struct fcb *fcb;
{
	int rec;

	rec = (fcb->ex << 7) | (fcb->cr & 0x7F);
	fcb->r0 = rec;
	fcb->r1 = rec >> 8;
}

/*
 * cmake - BDOS function 22, make a file.
 *
 * A file that already exists is refused; a new one is created, closed,
 * and then opened again through copen so the FCB comes away with the
 * descriptor, the size and the record count filled in.
 */
cmake(fcb)
struct fcb *fcb;
{
	int fd;

	if (isunique(fcb) == 0)
		return -1;

	name(fcb, buf);
	if (fexists(buf))
		return -1;

	fd = creat(buf, 0x1FF);
	if (fd < 0)
		return -1;
	close(fd);

	return copen(fcb);
}

/*
 * rename - BDOS function 23, rename a file.
 *
 * The FCB carries both names: the old one in dr/name/ft, the new one
 * packed into the disk map at offset 0x10.  A rename is a link to the
 * new name and an unlink of the old.  Both names must be plain files,
 * and both are closed by name first so no descriptor holds the inode.
 */
rename(fcb)
struct fcb *fcb;
{
	char newname[32];
	struct fcb *nfcb;

	nfcb = (struct fcb *)((char *)fcb + 0x10);
	nfcb->dr = fcb->dr;

	if (uniqize(fcb) == 0)
		return -1;

	name(fcb, buf);
	name(nfcb, newname);

	if (isdir(buf) || isdir(newname))
		return -1;

	closename(buf);
	closename(newname);

	if (link(buf, newname) < 0)
		return -1;
	if (unlink(buf) < 0)
		return -1;

	fcb->fd = -1;
	return 0;
}

/*
 * uniqize - make the FCB's name a concrete one.
 *
 * A name with a '?' wildcard cannot be opened; this searches the
 * directory for the first match and writes the real name back over the
 * wildcard one.  Returns 1 when there is a unique name - either it was
 * already unique, or a match was found - and 0 when nothing matched.
 */
uniqize(fcb)
struct fcb *fcb;
{
	if (isunique(fcb))
		return 1;

	if (fcb->dr == '?')
		fcb->dr = 0;
	if (fcb->ex == '?')
		fcb->ex = 0;

	if (search(0x11, fcb, fcb) != -1)
		return 1;
	return 0;
}

/*
 * isdir - is this pathname a directory.
 *
 * Stats it and tests the mode's file-type bits against SIFDIR.  A
 * failed stat is not a directory.
 */
isdir(name)
char *name;
{
	if (stat(name, statbuf) < 0)
		return 0;
	if ((statbuf[5] & 0x60) == 0x40)	/* (stmode & 0x6000) == SIFDIR */
		return 1;
	return 0;
}

/*
 * select - BDOS function 14, select a disk.
 *
 * A drive with no entry in disktab is a selerr.  Otherwise the drive's
 * bit goes into the logged-in vector and it becomes the current drive.
 */
select(drive)
int drive;
{
	drive &= 0x0F;

	if (disktab[drive] == 0)
		selerr(drive);

	/*
	 * The two stores below are in the binary's order the other way
	 * round - logvector first, then curdrive - but c1 has no rule for
	 * a shift whose count is a local that is also stored as a value,
	 * so curdrive is stored first and the shift reads it back out of
	 * the global.  The two globals are independent, so nothing else
	 * can tell.
	 */
	curdrive = drive;
	logvector |= 1 << curdrive;
}

/*
 * The console and list output path.  Both buffer 32 bytes and flush when
 * the buffer is full; the console writes to the terminal and the list to
 * the list descriptor.
 */
putch(c)
int c;
{
	*op++ = c;
	oleft++;
	if (oleft == 0x20)
		cflush();
}

lput(c)
int c;
{
	*lp++ = c;
	lleft++;
	if (lleft == 0x20)
		lflush();
}

lflush()
{
	write(lstdesc, lbuf, lleft);
	lp = lbuf;
	lleft = 0;
}

/*
 * cflush - flush the console output buffer.  The odd thing here is the
 * flow control: if an interrupt has left a character waiting and the
 * program is not in direct console I/O, suspend hands control back until
 * the character is taken, which is upm's ^S/^Q.
 */
cflush()
{
	if (recavail && call != 0x06)
		suspend();

	write(1, obuf, oleft);
	op = obuf;
	oleft = 0;
}

/*
 * getch - a character from the console, buffered.  Flushes any pending
 * output first so a prompt is not left unwritten, refills the input
 * buffer when it is empty, and returns the next character.
 */
getch()
{
	if (oleft)
		cflush();
	if (lleft)
		lflush();
	if (ileft == 0)
		cfill();
	ileft--;
	return *ip++;
}

/*
 * cfill - refill the console input buffer.  Reads up to sixteen bytes
 * into ibuf, and if the read comes back empty or short, decides between
 * a transient error (retry) and a dead terminal (gtty fails, so exit).
 */
cfill()
{
	int n;

	for (;;) {
		n = read(0, ibuf, 0x10);
		recavail = 0;

		if (n > 0) {
			ileft = n;
			ip = ibuf;
			return;
		}

		if (n < 0)
			continue;		/* transient error, read again */

		if (gtty(0, sgtty) >= 0)
			continue;		/* EOF but the terminal lives */

		cexit();			/* the terminal is gone */
	}
}

/*
 * suspend - upm's ^S/^Q.  Reads one character directly from the
 * terminal while the output is waiting.  A ^S means the user wanted to
 * stop, so it reads again for the ^Q that lets go and reports the ^S;
 * any other character is pushed back onto the input buffer so it is not
 * lost, and returned.
 */
suspend()
{
	char ch;

	read(0, &ch, 1);
	recavail = 0;

	if (ch == 0x13) {
		read(0, &ch, 1);
		recavail = 0;
		return 0x13;
	}

	if (ileft == 0)
		ip = ibuf;
	if (ip + ileft < lbuf) {
		ip[ileft] = ch;
		ileft++;
	}
	return ch;
}

/*
 * openfile - open a file by name, and remember its identity.
 *
 * Closes any descriptor already on the inode, then opens read/write and
 * falls back to read-only - setting ro so the FCB gets the read-only
 * flag.  The dev and inode are recorded in files for closename.
 */
openfile(name)
char *name;
{
	int fd;

	if (closename(name) == 0)
		return -1;

	ro = 0;
	fd = open(name, 2);
	if (fd < 0) {
		ro = 1;
		fd = open(name, 0);
		if (fd < 0)
			return -1;
	}

	if (fd < 0x10) {
		files[fd].dev = statbuf[0] | (statbuf[1] << 8);
		files[fd].ino = statbuf[2] | (statbuf[3] << 8);
	}

	return fd;
}

/*
 * closefile - close a descriptor.  The descriptors 0 and 1 are the
 * console and not in files, so only 2-15 are touched, and the inode is
 * cleared so closename will not find it again.
 */
closefile(fd)
int fd;
{
	if (fd < 2 || fd > 15)
		return;
	close(fd);
	files[fd].ino = 0;
}

/*
 * closename - close every descriptor on the file of this name.
 *
 * Stats the name, and for each entry in files whose dev and inode
 * match, closes it.  Returns 0 when the name cannot be stat'ed.
 */
closename(name)
char *name;
{
	int i;

	if (stat(name, statbuf) < 0)
		return 0;

	for (i = 0; i < 0x10; i++) {
		if (files[i].dev == (statbuf[0] | (statbuf[1] << 8)) &&
		    files[i].ino == (statbuf[2] | (statbuf[3] << 8))) {
			close(i);
			files[i].ino = 0;
		}
	}
	return 1;
}

/*
 * echo - a character to the console, with the two CP/M specials the
 * console layer cares about.  Tab is expanded by tabecho; \r resets the
 * column and \b backs it up; a printable advances it.
 */
echo(c)
int c;
{
	if (c == '\t') {
		tabecho();
		return;
	}

	putch(c);

	if (c == '\r') {
		col = 0;
		return;
	}
	if (c == '\b') {
		if (col)
			col--;
		return;
	}
	if (c < 0x20 || c > 0x7E)
		return;
	col++;
}

/*
 * cdirio - BDOS function 6, direct console I/O.  A character other than
 * FF is output, un-echoed; FF is a poll - a character if one is ready,
 * else 0 after flushing any pending output.
 */
cdirio()
{
	int c;

	c = arg2 & 0xFF;

	if (c != 0xFF) {
		putch(c);
		return c;
	}

	if (recavail || ileft)
		return getch();

	if (oleft)
		cflush();

	return 0;
}

/*
 * cprs - BDOS function 9, print a '$'-terminated string.  Each
 * character goes through echo so tabs expand and the column tracks.
 */
cprs()
{
	char *s;

	s = (char *)arg2;
	while (*s != '$') {
		echo(*s);
		s++;
	}
}

/*
 * readbuf - BDOS function 10, a line with editing.  The first byte is
 * the buffer size, the second the count, and the characters start at +2.
 * The editing characters are CP/M's: ^C cancels (a warm boot at the
 * start of a line), ^H backs up, ^M/^J end, ^R retypes, ^X clears the
 * line, and ^E is a bare newline that does not end the input.
 */
readbuf(buf)
char *buf;
{
	int c;

	col = 0;
	buf[1] = 0;

	for (;;) {
		if (buf[0] == buf[1])
			return;

		c = getch();

		switch (c) {
		case 3:			/* ^C */
			if (col == 0) {
				rbecho(3);
				puts("\r");
				wboot();
			}
			rbecho(c);
			buf[buf[1] + 2] = c;
			buf[1]++;
			break;

		case 5:			/* ^E */
			col = 0;
			puts("\r\n");
			break;

		case 8:			/* ^H */
			if (buf[1]) {
				buf[1]--;
				erase(buf[buf[1] + 2]);
			}
			break;

		case 10:		/* ^J */
		case 13:		/* ^M */
			col = 0;
			putch('\r');
			return;

		case 18:		/* ^R */
			puts("#\r\n");
			putb(buf + 2, buf[1]);
			break;

		case 24:		/* ^X */
			while (col)
				rub();
			buf[1] = 0;
			break;

		default:
			rbecho(c);
			buf[buf[1] + 2] = c;
			buf[1]++;
			break;
		}
	}
}

/*
 * erase - undo one echoed character, the backspace half of ^H.  A tab
 * is undone by taberase; a printable by two rubs; a control or a high
 * character by one, or none.
 */
erase(c)
int c;
{
	if (c == '\t') {
		taberase();
		return;
	}
	if (c >= 0 && c < 0x20) {
		rub();
		rub();
		return;
	}
	if (c < 0x20 || c > 0x7E)
		return;
	rub();
}

/*
 * The tab and rubout machinery.  A tab stop is every eight columns;
 * tabecho remembers where each tab landed in tabs so taberase can
 * erase back to it, and the rest is space, rub and tabpos.
 */
tabpos()
{
	return (col & 7) == 0;
}

space()
{
	putch(' ');
	col++;
}

rub()
{
	if (col == 0)
		return;
	puts("\b \b");
	col--;
}

tabecho()
{
	if (ntab != 0x0C) {
		tabs[ntab] = col;
		ntab++;
	}
	space();
	while (!tabpos())
		space();
}

taberase()
{
	int c;

	if (ntab != 0)
		c = tabs[--ntab];
	else
		c = 1;

	while (col != c)
		rub();
}

/*
 * rbecho - echo a character the way the console wants it.  A tab is
 * expanded; a control character becomes a caret and its letter; a
 * printable is itself.  echo calls putch and tracks the column; this
 * is the editing version.
 */
rbecho(c)
int c;
{
	if (c == '\t') {
		tabecho();
		return;
	}
	if (c >= 0 && c < 0x20) {
		putch('^');
		putch(c + 0x40);
		col += 2;
		return;
	}
	if (c >= 0x20 && c <= 0x7E) {
		putch(c);
		col++;
	}
}

/*
 * go - jump into the CP/M TPA and run the loaded program.  The entry
 * is always 0x100; when the program returns, cexit takes over.
 */
go()
{
	(*((void (*)())0x100))();
	cexit();
}

/*
 * selerr - a drive was selected that has no directory.  Ask for one,
 * verify it is a directory, and remember it in disktab, the same way
 * the drive-assignment command does.  It loops until the answer is a
 * directory.
 */
selerr(drive)
int drive;
{
	char buf[48];

	drive &= 0x0F;

	for (;;) {
		puts("\r\nSelect a directory for drive ");
		putch(drive + 'A');
		puts(": ");
		buf[0] = '\'';
		readbuf(buf);
		puts("\r\n");
		buf[buf[1] + 2] = 0;
		cflush();

		if (isdir(buf + 2))
			break;

		puts(buf + 2);
		puts(": Not a directory\r\n");
	}

	disktab[drive] = save(buf + 2);
}
struct node base;		/* eb33 */
struct node *allocp;		/* eb37 */
char	savebuf[512];		/* the SAVE allocator's free memory */

setalloc()
{
	if (allocp == 0) {
		allocp = &base;
		base.next = &base;
		base.size = 0;
	}
	return allocp;
}

alloc(n)
ushort n;
{
	ushort size;
	struct node *p;
	struct node *q;

	size = (n + 3) / 4 + 1;

	p = setalloc();
	q = p->next;

	for (;;) {
		if (q->size >= size)
			break;
		if (q == p) {
			puts("Out of memory\r\n");
			cexit();
		}
		p = q;
		q = q->next;
	}

	/* take the allocation from the end of the free block */
	q->size -= size;
	q = (struct node *)((char *)q + q->size * 4);
	q->size = size;

	allocp = p;
	return (char *)q + 4;
}

save(s)
char *s;
{
	char *p;

	p = alloc(strlen(s) + 1);
	if (p == 0) {
		puts("Out of memory\r\n");
		cexit();
	}
	cpystr(p, s, 0);
	return p;
}

/*
 * trestore - put the terminal back the way it was, the inverse of what
 * tset does when a program starts.  Clears the raw-mode bits.
 */
trestore()
{
	gtty(0, sgtty);
	sgtty[4] |= 0x1A;
	sgtty[4] &= ~0x20;
	stty(0, sgtty);
}

/*
 * load - read the .com named by loadfile into the TPA and jump to it.
 * Sixteen kilobytes at a time, from 0x100 up.
 */
load()
{
	int fd;
	int offset;
	int n;

	fd = open(loadfile, 0);
	if (fd < 0) {
		perror(loadfile);
		cexit();
	}

	offset = 0x100;
	for (;;) {
		n = read(fd, offset, 0x4000);
		if (n <= 0) {
			close(fd);
			close(2);
			loaded = 1;
			go();
		}
		offset += n;
	}
}

/*
 * wboot - the warm boot, BDOS function 0 and the BIOS's WBOOT.  If a
 * program is loaded, its finishing means upm finishes too.
 */
wboot()
{
	if (loaded == 0)
		return;
	cexit();
}

/*
 * badbios - a CP/M program called a BIOS entry that is not there.
 */
badbios()
{
	puts("Bad bios call\r\n");
	cexit();
}

/*
 * ---------------------------------------------------------------------
 * The helpers the micronix libc does not provide.
 *
 * The string and byte routines the original linked from Whitesmith's
 * library are the micronix libc's own now - strlen, strcmp, memset,
 * memcpy, strchr - and upm calls those directly.  What is left here are
 * the ones libc does not have: cpystr, the variadic concatenate; puts
 * and putb, which write through putch rather than stdio; itob; and
 * signal, libunix's, moved here so _stab stays resident where a running
 * .com cannot clobber it.
 *
 * The one that is not obvious is cpystr: it copies its source strings
 * into dst until it reads a null argument, then writes the terminating
 * nul and returns where it finished.  The null argument is what every
 * call ends in.
 */

/*
 * puts - print a string through putch, one character at a time, with no
 * trailing newline.  Not the stdio puts.
 */
puts(s)
char *s;
{
	while (*s)
		putch(*s++);
}

/*
 * putb - print n bytes of buf through putch.
 */
putb(buf, n)
char *buf;
int n;
{
	while (n--)
		putch(*buf++);
}

/*
 * raise - uppercase a string in place, the string form of uc.
 */
raise(s)
char *s;
{
	while (*s) {
		*s = uc(*s);
		s++;
	}
}

/*
 * cpystr - concatenate the source strings into dst.  The sources are a
 * variable number of char * arguments, ended by a null argument; each is
 * copied in turn, a single nul is written, and the position after it is
 * returned.
 */
char *
cpystr(dst, s)
char *dst;
char *s;
{
	char **ap;
	char *q;

	ap = &s;
	while ((q = *ap++) != 0) {
		while (*q)
			*dst++ = *q++;
	}
	*dst = 0;
	return dst;
}

/*
 * signal and stab - libunix's, moved here so that _stab is resident: the
 * trampolines in upmsys.s read it while a .com runs, and libunix's signal.o
 * would put it in the data/bss segment at 0x0100, where the .com clobbers
 * it.  _signal is the syscall stub in upmsys.s.
 */
short stab[15];
