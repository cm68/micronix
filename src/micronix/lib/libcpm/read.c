#include	"cpm.h"

extern long	_fsize();

read(fd, buf, nbytes)
uchar	fd;
ushort	nbytes;
char *	buf;
{
	register struct fcb *	fc;
	uchar	size, offs, luid;
	ushort	cnt;
	long	fsz;
	char	buffer[SECSIZE+2];

	cnt = 0;
	if(fd >= MAXFILE)
		return -1;
	fc = &_fcb[fd];
	switch(fc->use) {

	case U_RDR:
		cnt = nbytes;
		while(nbytes) {
			nbytes--;
			if((*buf++ = (bdos(CPMRRDR) & 0x7f)) == '\n')
				break;
		}
		return cnt - nbytes;

	case U_CON:
		if(nbytes > SECSIZE)
			nbytes = SECSIZE;
		buffer[0] = nbytes;
		bdos(CPMRCOB, buffer);
		cnt = (uchar)buffer[1];
		if(cnt < nbytes) {
			bdos(CPMWCON, '\n');
			buffer[cnt+2] = '\n';
			cnt++;
		}
		bmove(&buffer[2], buf, cnt);
		return cnt;

	case U_READ:
	case U_RDWR:
		luid = getuid();
		cnt = nbytes;
		/*
		 * How long the file actually is.  Records alone are not
		 * enough: the last one is usually part full, and reading
		 * all 128 bytes of it hands the caller whatever followed
		 * the data.  _fsize is exact under CP/M 3 - see seek.c.
		 */
		fsz = _fsize(fd);
		while(nbytes) {
			_sigchk();
			if(fc->rwp >= fsz)
				break;
			setuid(fc->uid);
			/*
			 * SECSIZE is 128, so the record is a shift and
			 * the offset within it a mask.  Written as / and
			 * %% on a long these called qdiv, two hundred
			 * bytes of general division to do what an and
			 * and a shift do, in a runtime whose whole
			 * problem is size.
			 */
			offs = (int)fc->rwp & (SECSIZE - 1);
			if((size = SECSIZE - offs) > nbytes)
				size = nbytes;
			if(fc->rwp + size > fsz)
				size = (uchar)(fsz - fc->rwp);
			_putrno(fc->ranrec, fc->rwp >> 7);
			if(size == SECSIZE) {
				bdos(CPMSDMA, buf);
#ifdef	LARGE_MODEL
				bdos(CPMDSEG, (int)((long)buf >> 16));	/* set DMA segment */
#endif
				if(bdos(CPMRRAN, fc))
					break;
			} else {
				bdos(CPMSDMA, buffer);
#ifdef	LARGE_MODEL
				bdos(CPMDSEG, (int)((long)buffer >> 16));	/* set DMA segment */
#endif
				if(bdos(CPMRRAN, fc))
					break;
				bmove(buffer+offs, buf, size);
			}
			buf += size;
			fc->rwp += size;
			nbytes -= size;
			setuid(luid);
		}
		setuid(luid);
		return cnt - nbytes;

	default:
		return -1;
	}
}
