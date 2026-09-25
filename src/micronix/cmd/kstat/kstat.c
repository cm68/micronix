/*
 * kstat - dump the kernel's tables out of /dev/mem
 *
 * cmd/kstat/kstat.c
 *
 * ps with teeth.  ps shows the process table; kstat shows everything the
 * kernel keeps that is not the user structure - the process table, the
 * terminal structures (the thing a hung login is usually about), the
 * buffer cache headers, the mount table, the open-file table, the
 * in-core inode table, the clist, and the device switch tables.
 *
 * The addresses come from the kernel's own symbol table, read out of
 * /micronix - the same object file the boot loader loads - rather than
 * from a fixed hook like ps's 0x1003.  Each table is then read out of
 * /dev/mem at the address its symbol gives.
 *
 * The struct definitions come from the same headers the kernel is built
 * against (sys/proc.h, sys/tty.h, ...), so the field offsets are the
 * kernel's by construction, not by reproduction.
 */

#include <stdio.h>
#include <string.h>
#include <types.h>
#include <sys/sys.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/buf.h>
#include <sys/mount.h>
#include <sys/file.h>
#include <sys/inode.h>
#include <obj.h>

#define KERNEL  "/micronix"
#define MEMORY  "/dev/mem"

/*
 * number of mult I/O ports.  The kernel keeps this in sys/multio.c, not
 * in a header, so it is repeated here.
 */
#define NMIO    4

static int memfd;

/*
 * kstat is not linked with libu - it is a /micronix reader that the
 * kernel build and the tools share - so it says for itself what it
 * wants of lseek.  The declaration matters as much as the call: without
 * it the compiler assumes an int return and keeps only the low word of
 * a file position.
 */
extern long lseek(unsigned char fd, long offset, int whence);

/*
 * a bit-name table for printbits().
 */
struct bitname {
	unsigned mask;
	char *name;
};

/*
 * a 32-bit value read as two 16-bit words, for fields like a file's
 * read/write pointer whose byte order is the Z280's word-swapped long.
 */
union word32 {
	UINT32 l;
	unsigned w[2];
};

/*
 * look up a symbol's absolute address in the kernel object file
 * /micronix.  Returns -1 (0xffff) if the file cannot be read or the
 * name is not there.  The symbol's value in a linked kernel is the
 * absolute address, so it is returned as is.
 */
static unsigned short
symaddr(name)
	char *name;
{
	unsigned char hdr[16];
	unsigned char entry[18];	/* 2 value + 1 type + 15 name */
	unsigned short symtab, text, data, val;
	unsigned char conf, type;
	char nb[16];
	int symlen, nsym, i, k;
	int fd;
	long off;

	fd = open(KERNEL, 0);
	if (fd < 0) {
		printf("kstat: open %s -> %d\n", KERNEL, fd);
		return (unsigned short)-1;
	}
	if (read(fd, hdr, 16) != 16) {
		printf("kstat: header read failed (fd %d)\n", fd);
		close(fd);
		return (unsigned short)-1;
	}

	conf = hdr[1];
	symlen = (conf & CONF_SYMASK) * 2 + 1;
	symtab = hdr[2] | (hdr[3] << 8);
	text = hdr[4] | (hdr[5] << 8);
	data = hdr[6] | (hdr[7] << 8);
	nsym = symtab / (symlen + 3);
	off = 16L + (long)text + (long)data;	/* symbol table starts here */

	for (i = 0; i < nsym; i++) {
		/*
		 * The symbol table sits past the 64K a word-sized seek
		 * reaches, and lseek splits the offset for that - the
		 * split this loop used to make itself.
		 */
		if (lseek(fd, off, 0) < 0) {
			printf("kstat: seek to %u failed at sym %d\n",
			    (unsigned)off, i);
			break;
		}
		if (read(fd, entry, symlen + 3) != symlen + 3) {
			printf("kstat: read at %u failed at sym %d\n",
			    (unsigned)off, i);
			break;
		}
		val = entry[0] | (entry[1] << 8);
		type = entry[2];
		if (!(type & SF_DEF))
			goto next;
		for (k = 0; k < symlen; k++)
			nb[k] = entry[3 + k];
		nb[symlen] = '\0';
		if (strcmp(nb, name) == 0) {
			close(fd);
			return val;
		}
next:
		off += symlen + 3;
	}
	close(fd);
	return (unsigned short)-1;
}

/*
 * read n bytes of kernel memory at addr into buf.
 */
static void
kread(addr, buf, n)
	unsigned short addr;
	char *buf;
	unsigned n;
{
	seek(memfd, addr, 0);
	read(memfd, buf, n);
}

/*
 * print the set bits in v against a table of bit names.
 */
static void
printbits(v, t, n)
	unsigned v;
	struct bitname *t;
	int n;
{
	int i, any = 0;

	for (i = 0; i < n; i++) {
		if (v & t[i].mask) {
			printf("%s%s", any ? "|" : "", t[i].name);
			any = 1;
		}
	}
	if (!any)
		printf("-");
}

static struct bitname tstate_bits[] = {
	{ 0x01, "LOSTOP" }, { 0x02, "HOSLEEP" }, { 0x04, "HISLEEP" },
	{ 0x08, "STOPIN" }, { 0x10, "INSTOP" }, { 0x20, "STARTIN" },
	{ 0x40, "OPEN" },   { 0x80, "ERROR" },
};

static struct bitname tmstate_bits[] = {
	{ 0x01, "WOPEN" }, { 0x02, "CD" }, { 0x04, "DTR" }, { 0x20, "DIALER" },
};

static struct bitname tmode_bits[] = {
	{ 0x8000, "SHAKE" }, { 0x4000, "ALL8" }, { 0x2000, "CBREAK" },
	{ 0x1000, "MORE" },  { 0x0020, "RAW" },  { 0x0010, "MAPCR" },
	{ 0x0008, "ECHO" },  { 0x0004, "OLDTTY" }, { 0x0002, "XTABS" },
};

static struct bitname pmode_bits[] = {
	{ 0x01, "ALLOC" }, { 0x02, "ALIVE" }, { 0x04, "AWAKE" },
	{ 0x08, "LOADED" }, { 0x10, "SWAPPED" }, { 0x20, "LOCKED" },
	{ 0x40, "SYS" },    { 0x80, "BACK" },
};

static struct bitname bflag_bits[] = {
	{ 0x01, "BREAD" }, { 0x02, "BBUSY" }, { 0x04, "BSYNC" },
	{ 0x08, "BDELWRI" }, { 0x10, "BDONE" }, { 0x20, "BLOCK" },
	{ 0x40, "BWANT" }, { 0x80, "BERROR" },
};

static struct bitname iflag_bits[] = {
	{ 0x01, "IBUSY" }, { 0x02, "IMOD" }, { 0x04, "IWANT" },
	{ 0x08, "IRONLY" }, { 0x10, "IWRLOCK" }, { 0x20, "IPIPE" },
};

/*
 * process table
 */
static void
dump_proc(addr)
	unsigned short addr;
{
	struct proc p;
	int i;
	unsigned ba;

	printf("\n== process table (_plist @ 0x%x, %d x %d) ==\n",
	    addr, NPROC, sizeof(struct proc));
	seek(memfd, addr, 0);
	for (i = 0; i < NPROC; i++) {
		ba = (unsigned)addr + i * (unsigned)sizeof(struct proc);
		if (read(memfd, &p, sizeof p) != sizeof p)
			break;
		if (!(p.mode & ALLOC))
			continue;
		printf(" %2d @%04x pid=%u %-8.8s mode=%02x(", i, ba, p.pid, p.args, p.mode);
		printbits(p.mode, pmode_bits, 8);
		printf(") uid=%u pri=%u nice=%u event=%04x tty=%04x parent=%04x\n",
		    p.uid, p.pri, p.nice, p.event, p.tty, (unsigned)p.parent);
		printf("      status=%u nsegs=%u time=%u alarm=%u swap=%u\n",
		    p.status, p.nsegs, p.time, p.alarm, p.swap);
	}
}

/*
 * terminal structures - the point of this program for a hung tty.
 */
static void
dump_tty(addr)
	unsigned short addr;
{
	struct tty t;
	int i;
	unsigned ba;

	/*
	 * queue/mstate addresses are the sleep channels (the WAIT column in
	 * ps).  These are their byte offsets into struct tty - see tty.h.
	 * Written out rather than computed, because c1 has no rule for
	 * pointer subtraction.
	 */
#define TTY_RAWQUE 21
#define TTY_COKQUE 26
#define TTY_OUTQUE 31
#define TTY_MSTATE 37

	printf("\n== tty structures (_mttys @ 0x%x, %d x %d) ==\n",
	    addr, NMIO, sizeof(struct tty));
	seek(memfd, addr, 0);
	for (i = 0; i < NMIO; i++) {
		ba = (unsigned)addr + i * (unsigned)sizeof(struct tty);
		if (read(memfd, &t, sizeof t) != sizeof t)
			break;
		printf(" [%d] @%04x dev=%04x ispeed=%u ospeed=%u erase=0x%02x kill=0x%02x\n",
		    i, ba, t.dev, t.ispeed, t.ospeed, t.erase, t.kill);
		printf("     mode=%04x(", t.mode);
		printbits(t.mode, tmode_bits, 9);
		printf(") state=%02x(", t.state);
		printbits(t.state, tstate_bits, 8);
		printf(") col=%u line=%u nextc=%u nbreak=%u\n",
		    t.col, t.line, t.nextc, t.nbreak);
		printf("     raw@%04x cnt=%u first=%04x last=%04x\n",
		    ba + TTY_RAWQUE, t.rawque.count,
		    (unsigned)t.rawque.first, (unsigned)t.rawque.last);
		printf("     cok@%04x cnt=%u first=%04x last=%04x\n",
		    ba + TTY_COKQUE, t.cokque.count,
		    (unsigned)t.cokque.first, (unsigned)t.cokque.last);
		printf("     out@%04x cnt=%u first=%04x last=%04x\n",
		    ba + TTY_OUTQUE, t.outque.count,
		    (unsigned)t.outque.first, (unsigned)t.outque.last);
		printf("     opencount=%u mstate@%04x=%02x(",
		    t.count, ba + TTY_MSTATE, t.mstate);
		printbits(t.mstate, tmstate_bits, 4);
		printf(")\n");
		printf("     start=%04x stop=%04x put=%04x set=%04x\n",
		    (unsigned)t.start, (unsigned)t.stop,
		    (unsigned)t.put, (unsigned)t.set);
	}
}

/*
 * buffer cache headers
 */
static void
dump_buf(addr, nbuf)
	unsigned short addr;
	unsigned nbuf;
{
	struct buf b;
	int i;
	unsigned ba;

	printf("\n== buffer headers (_blist @ 0x%x, %u) ==\n", addr, nbuf);
	seek(memfd, addr, 0);
	for (i = 0; i < nbuf; i++) {
		ba = (unsigned)addr + i * (unsigned)sizeof(struct buf);
		if (read(memfd, &b, sizeof b) != sizeof b)
			break;
		printf(" %3d @%04x dev=%04x blk=%u flags=%02x(", i, ba, b.dev, b.blk, b.flags);
		printbits(b.flags, bflag_bits, 8);
		printf(") count=%u data=%04x xmem=%u err=%u time=%u\n",
		    b.count, (unsigned)b.data, b.xmem, b.error, b.time);
		printf("      forw=%04x back=%04x cyl=%u hash=%04x\n",
		    (unsigned)b.forw, (unsigned)b.back, b.cyl, (unsigned)b.b_hash);
	}
}

/*
 * mount table
 */
static void
dump_mount(addr)
	unsigned short addr;
{
	struct mount m;
	int i;
	unsigned ba;

	printf("\n== mount table (_mlist @ 0x%x, %d x %d) ==\n",
	    addr, NMOUNT, sizeof(struct mount));
	seek(memfd, addr, 0);
	for (i = 0; i < NMOUNT; i++) {
		ba = (unsigned)addr + i * (unsigned)sizeof(struct mount);
		if (read(memfd, &m, sizeof m) != sizeof m)
			break;
		if (m.dev == 0 && m.inode == 0)
			continue;
		printf(" [%d] @%04x dev=%04x inode=%04x ronly=%u fsize=%u isize=%u\n",
		    i, ba, m.dev, (unsigned)m.inode, m.ronly, m.fsize, m.isize);
	}
}

/*
 * open file table
 */
static void
dump_file(addr)
	unsigned short addr;
{
	struct file f;
	union word32 rp;
	int i;
	unsigned ba;

	printf("\n== file table (_flist @ 0x%x, %d x %d) ==\n",
	    addr, NFILE, sizeof(struct file));
	seek(memfd, addr, 0);
	for (i = 0; i < NFILE; i++) {
		ba = (unsigned)addr + i * (unsigned)sizeof(struct file);
		if (read(memfd, &f, sizeof f) != sizeof f)
			break;
		if (f.count == 0)
			continue;
		rp.l = f.rwptr;
		printf(" %2d @%04x mode=%02x count=%u inode=%04x rwptr=%04x:%04x\n",
		    i, ba, f.mode, f.count, (unsigned)f.inode, rp.w[0], rp.w[1]);
	}
}

/*
 * in-core inode table
 */
static void
dump_inode(addr)
	unsigned short addr;
{
	struct inode in;
	int i;
	unsigned ba;

	printf("\n== inode table (_ilist @ 0x%x, %d x %d) ==\n",
	    addr, NINODE, sizeof(struct inode));
	seek(memfd, addr, 0);
	for (i = 0; i < NINODE; i++) {
		ba = (unsigned)addr + i * (unsigned)sizeof(struct inode);
		if (read(memfd, &in, sizeof in) != sizeof in)
			break;
		if (in.i_count == 0 && !(in.i_flags & IBUSY))
			continue;
		printf(" %2d @%04x dev=%04x inum=%u mode=%x nlink=%u uid=%u gid=%u "
		    "size=%u flags=%02x(", i, ba, in.i_dev, in.i_inum,
		    in.i_mode, in.i_nlink, in.i_uid, in.i_gid,
		    in.i_size0, in.i_flags);
		printbits(in.i_flags, iflag_bits, 6);
		printf(") count=%u mount=%04x time=%u\n",
		    in.i_count, (unsigned)in.i_mount, in.i_time);
	}
}

/*
 * clist - count the free cblocks.
 */
static void
dump_clist(clist, cfree)
	unsigned short clist;
	unsigned short cfree;
{
	struct cblock cb;
	unsigned short p;
	int n = 0;

	printf("\n== clist (_clist @ 0x%x, _cfree @ 0x%x) ==\n", clist, cfree);
	p = cfree;
	while (p != 0) {
		kread(p, (char *)&cb, sizeof cb);
		n++;
		p = (unsigned short)cb.next;
		if (n > CSIZE) {
			printf("  (free list loops at %u blocks)\n", n);
			break;
		}
	}
	printf("  free cblocks: %d\n", n);
}

/*
 * device switch tables
 */
static void
dump_devsw(addr, count, words, name)
	unsigned short addr;
	unsigned count;
	int words;
	char *name;
{
	int i, j;
	unsigned short w;

	printf("\n== %s (@ 0x%x, %u) ==\n", name, addr, count);
	seek(memfd, addr, 0);
	for (i = 0; i < count; i++) {
		printf(" %2d:", i);
		for (j = 0; j < words; j++) {
			if (read(memfd, &w, sizeof w) != sizeof w)
				break;
			printf(" %04x", w);
		}
		printf("\n");
	}
}

/*
 * a handful of small config globals.
 */
static void
dump_config()
{
	unsigned short rootdev, swapdev, swapsize, swapaddr;
	unsigned short nbuf, nbdev, ncdev;

	kread(symaddr("_rootdev"), (char *)&rootdev, 2);
	kread(symaddr("_swapdev"), (char *)&swapdev, 2);
	kread(symaddr("_swapsize"), (char *)&swapsize, 2);
	kread(symaddr("_swapaddr"), (char *)&swapaddr, 2);
	nbuf = symaddr("_nbuf");
	nbdev = symaddr("_nbdev");
	ncdev = symaddr("_ncdev");

	printf("\n== config ==\n");
	printf(" rootdev=%04x swapdev=%04x swapsize=%u swapaddr=%u\n",
	    rootdev, swapdev, swapsize, swapaddr);
	printf(" _nbuf=%04x _nbdev=%04x _ncdev=%04x\n", nbuf, nbdev, ncdev);
}

int
main()
{
	unsigned short a;

	memfd = open(MEMORY, 0);
	if (memfd < 0) {
		printf("kstat: cannot open %s\n", MEMORY);
		return 1;
	}

	dump_config();

	a = symaddr("_plist");
	if (a != (unsigned short)-1)
		dump_proc(a);

	a = symaddr("_mttys");
	if (a != (unsigned short)-1)
		dump_tty(a);

	a = symaddr("_blist");
	if (a != (unsigned short)-1) {
		unsigned char nb = 0;
		kread(symaddr("_nbuf"), (char *)&nb, 1);
		dump_buf(a, nb);
	}

	a = symaddr("_mlist");
	if (a != (unsigned short)-1)
		dump_mount(a);

	a = symaddr("_flist");
	if (a != (unsigned short)-1)
		dump_file(a);

	a = symaddr("_ilist");
	if (a != (unsigned short)-1)
		dump_inode(a);

	a = symaddr("_clist");
	if (a != (unsigned short)-1) {
		unsigned short cf = 0;
		kread(symaddr("_cfree"), (char *)&cf, 2);
		dump_clist(a, cf);
	}

	a = symaddr("_biosw");
	if (a != (unsigned short)-1) {
		unsigned char nb = 0;
		kread(symaddr("_nbdev"), (char *)&nb, 1);
		dump_devsw(a, nb, 3, "block device switch (_biosw)");
	}

	a = symaddr("_ciosw");
	if (a != (unsigned short)-1) {
		unsigned char nc = 0;
		kread(symaddr("_ncdev"), (char *)&nc, 1);
		dump_devsw(a, nc, 5, "character device switch (_ciosw)");
	}

	return 0;
}
