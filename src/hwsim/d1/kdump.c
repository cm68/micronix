/*
 * kdump - dump the guest kernel's tables, host side
 *
 * hwsim/d1/kdump.c
 *
 * The same dump cmd/kstat does from inside the guest, but done here in
 * the simulator: read the kernel's tables out of physical memory and
 * print them.  It runs on SIGUSR2 (a signal the sim does not otherwise
 * use), through a latch the main loop notices, so the dump happens
 * between instructions and not in the middle of the emulation.
 *
 * The addresses come from the kernel symbol table loaded by -S; the
 * bytes come from physread(), because the kernel's low 64k is
 * identity-mapped and physread reads physical memory no matter which
 * task is mapped at the moment the signal lands.
 *
 * The struct layouts are the kernel's, spelled out with fixed-width
 * host types and packed, because a host compiler's 32-bit pointers and
 * alignment would shift every field.  The offsets below match the
 * kernel's headers (sys/proc.h, sys/tty.h, ...) one for one.
 */

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include "sim.h"
#include "hwsim.h"

#ifndef NODEBUG

typedef unsigned char  k8;
typedef unsigned short k16;
typedef unsigned int   k32;

/*
 * kernel constants (sys/sys.h, sys/multio.c, sys/signal.h)
 */
#define K_NPROC 20
#define K_NINODE 50
#define K_NMOUNT 4
#define K_NFILE 60
#define K_NMIO 4
#define K_NSIG 16

#define TTY_RAWQUE 21
#define TTY_COKQUE 26
#define TTY_OUTQUE 31
#define TTY_MSTATE 37

struct k_que {
	k8 count;
	k16 first, last;
} __attribute__((packed));

struct k_tty {
	k8 ispeed, ospeed, erase, kill;
	k16 mode;
	k8 state, col;
	k16 dev;
	k8 line;
	k16 start, stop, put, set;
	k8 nextc, nbreak;
	struct k_que rawque, cokque, outque;
	k8 count, mstate;
} __attribute__((packed));

struct k_proc {
	char args[8];
	k8 mode, uid;
	k16 event, status, frmptr, stkptr, parent, tty, swap;
	k8 nsegs;
	struct { k8 seg, per; } mem[17];
	k16 slist[K_NSIG];
	k8 nice, pri;
	k16 time, alarm;
	k16 pid;
} __attribute__((packed));

struct k_buf {
	k8 flags;
	k16 dev, blk, count, data;
	k8 xmem;
	k16 forw, back, cyl;
	k8 error;
	k16 time, b_hash;
} __attribute__((packed));

struct k_mount {
	k16 dev, inode;
	k8 ronly;
	k16 fsize, isize;
} __attribute__((packed));

struct k_file {
	k8 mode, count;
	k16 inode;
	k32 rwptr;
} __attribute__((packed));

struct k_inode {
	k16 dev, inum, mode;
	k8 nlink, uid, gid, size0;
	k16 size1;
	k16 addr[8];
	k32 atime, mtime;
	k8 flags, count;
	k16 mount;
	k32 size;
	k16 time;
} __attribute__((packed));

struct k_cblock {
	k16 next;
	k8 block[14];
} __attribute__((packed));

struct bitname {
	unsigned mask;
	char *name;
};

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

extern int find_symbol(char *ls);

static int
rd16(paddr addr)
{
	return physread(addr) | (physread(addr + 1) << 8);
}

static void
rdmem(paddr addr, void *dst, int n)
{
	unsigned char *p = dst;
	int i;

	for (i = 0; i < n; i++)
		p[i] = physread(addr + i);
}

static void
printbits(unsigned v, struct bitname *t, int n)
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

static void
dump_proc(int addr)
{
	struct k_proc p;
	int i;

	printf("\n== process table (_plist @ 0x%04x, %d x %d) ==\n",
	    addr, K_NPROC, (int)sizeof(struct k_proc));
	for (i = 0; i < K_NPROC; i++) {
		rdmem((paddr)(addr + i * (int)sizeof(struct k_proc)), &p, sizeof p);
		if (!(p.mode & 0x01))	/* ALLOC */
			continue;
		printf(" %2d @%04x pid=%u %-8.8s mode=%02x(", i,
		    addr + i * (int)sizeof(struct k_proc), p.pid, p.args, p.mode);
		printbits(p.mode, pmode_bits, 8);
		printf(") uid=%u pri=%u nice=%u event=%04x tty=%04x parent=%04x\n",
		    p.uid, p.pri, p.nice, p.event, p.tty, p.parent);
		printf("      status=%u nsegs=%u time=%u alarm=%u swap=%u\n",
		    p.status, p.nsegs, p.time, p.alarm, p.swap);
	}
}

static void
dump_tty(int addr)
{
	struct k_tty t;
	int i;

	printf("\n== tty structures (_mttys @ 0x%04x, %d x %d) ==\n",
	    addr, K_NMIO, (int)sizeof(struct k_tty));
	for (i = 0; i < K_NMIO; i++) {
		rdmem((paddr)(addr + i * (int)sizeof(struct k_tty)), &t, sizeof t);
		printf(" [%d] @%04x dev=%04x ispeed=%u ospeed=%u erase=0x%02x kill=0x%02x\n",
		    i, addr + i * (int)sizeof(struct k_tty), t.dev, t.ispeed,
		    t.ospeed, t.erase, t.kill);
		printf("     mode=%04x(", t.mode);
		printbits(t.mode, tmode_bits, 9);
		printf(") state=%02x(", t.state);
		printbits(t.state, tstate_bits, 8);
		printf(") col=%u line=%u nextc=%u nbreak=%u\n",
		    t.col, t.line, t.nextc, t.nbreak);
		printf("     raw@%04x cnt=%u first=%04x last=%04x\n",
		    addr + i * (int)sizeof(struct k_tty) + TTY_RAWQUE,
		    t.rawque.count, t.rawque.first, t.rawque.last);
		printf("     cok@%04x cnt=%u first=%04x last=%04x\n",
		    addr + i * (int)sizeof(struct k_tty) + TTY_COKQUE,
		    t.cokque.count, t.cokque.first, t.cokque.last);
		printf("     out@%04x cnt=%u first=%04x last=%04x\n",
		    addr + i * (int)sizeof(struct k_tty) + TTY_OUTQUE,
		    t.outque.count, t.outque.first, t.outque.last);
		printf("     opencount=%u mstate@%04x=%02x(",
		    t.count, addr + i * (int)sizeof(struct k_tty) + TTY_MSTATE,
		    t.mstate);
		printbits(t.mstate, tmstate_bits, 4);
		printf(")\n");
		printf("     start=%04x stop=%04x put=%04x set=%04x\n",
		    t.start, t.stop, t.put, t.set);
	}
}

static void
dump_buf(int addr, int nbuf)
{
	struct k_buf b;
	int i;

	printf("\n== buffer headers (_blist @ 0x%04x, %d) ==\n", addr, nbuf);
	for (i = 0; i < nbuf; i++) {
		rdmem((paddr)(addr + i * (int)sizeof(struct k_buf)), &b, sizeof b);
		printf(" %3d @%04x dev=%04x blk=%u flags=%02x(", i,
		    addr + i * (int)sizeof(struct k_buf), b.dev, b.blk, b.flags);
		printbits(b.flags, bflag_bits, 8);
		printf(") count=%u data=%04x xmem=%u err=%u time=%u\n",
		    b.count, b.data, b.xmem, b.error, b.time);
		printf("      forw=%04x back=%04x cyl=%u hash=%04x\n",
		    b.forw, b.back, b.cyl, b.b_hash);
	}
}

static void
dump_mount(int addr)
{
	struct k_mount m;
	int i;

	printf("\n== mount table (_mlist @ 0x%04x, %d x %d) ==\n",
	    addr, K_NMOUNT, (int)sizeof(struct k_mount));
	for (i = 0; i < K_NMOUNT; i++) {
		rdmem((paddr)(addr + i * (int)sizeof(struct k_mount)), &m, sizeof m);
		if (m.dev == 0 && m.inode == 0)
			continue;
		printf(" [%d] @%04x dev=%04x inode=%04x ronly=%u fsize=%u isize=%u\n",
		    i, addr + i * (int)sizeof(struct k_mount), m.dev, m.inode,
		    m.ronly, m.fsize, m.isize);
	}
}

static void
dump_file(int addr)
{
	struct k_file f;
	int i;

	printf("\n== file table (_flist @ 0x%04x, %d x %d) ==\n",
	    addr, K_NFILE, (int)sizeof(struct k_file));
	for (i = 0; i < K_NFILE; i++) {
		rdmem((paddr)(addr + i * (int)sizeof(struct k_file)), &f, sizeof f);
		if (f.count == 0)
			continue;
		printf(" %2d @%04x mode=%02x count=%u inode=%04x rwptr=%04x:%04x\n",
		    i, addr + i * (int)sizeof(struct k_file), f.mode, f.count,
		    f.inode, (unsigned)(f.rwptr >> 16), (unsigned)(f.rwptr & 0xffff));
	}
}

static void
dump_inode(int addr)
{
	struct k_inode in;
	int i;

	printf("\n== inode table (_ilist @ 0x%04x, %d x %d) ==\n",
	    addr, K_NINODE, (int)sizeof(struct k_inode));
	for (i = 0; i < K_NINODE; i++) {
		rdmem((paddr)(addr + i * (int)sizeof(struct k_inode)), &in, sizeof in);
		if (in.count == 0 && !(in.flags & 0x01))	/* IBUSY */
			continue;
		printf(" %2d @%04x dev=%04x inum=%u mode=%x nlink=%u uid=%u gid=%u "
		    "size=%u flags=%02x(", i, addr + i * (int)sizeof(struct k_inode),
		    in.dev, in.inum, in.mode, in.nlink, in.uid, in.gid,
		    in.size1, in.flags);
		printbits(in.flags, iflag_bits, 6);
		printf(") count=%u mount=%04x time=%u\n",
		    in.count, in.mount, in.time);
	}
}

static void
dump_clist(int clist, int cfree)
{
	struct k_cblock cb;
	int p, n = 0;

	printf("\n== clist (_clist @ 0x%04x, _cfree @ 0x%04x) ==\n", clist, cfree);
	p = cfree;
	while (p != 0) {
		rdmem((paddr)p, &cb, sizeof cb);
		n++;
		p = cb.next;
		if (n > 900) {
			printf("  (free list loops at %d blocks)\n", n);
			break;
		}
	}
	printf("  free cblocks: %d\n", n);
}

static void
dump_devsw(int addr, int count, int words, char *name)
{
	int i, j;

	printf("\n== %s (@ 0x%04x, %d) ==\n", name, addr, count);
	for (i = 0; i < count; i++) {
		printf(" %2d:", i);
		for (j = 0; j < words; j++)
			printf(" %04x", rd16((paddr)(addr + (i * words + j) * 2)));
		printf("\n");
	}
}

/*
 * the whole dump.  Called from the main loop when the SIGUSR2 latch is
 * set, so printf is safe here.
 */
void
dump_kernel_tables(void)
{
	int a;

	a = find_symbol("_rootdev");
	printf("\n== config ==");
	if (a >= 0)
		printf("\n rootdev=%04x swapdev=%04x swapsize=%u swapaddr=%u",
		    rd16((paddr)a), rd16((paddr)find_symbol("_swapdev")),
		    rd16((paddr)find_symbol("_swapsize")),
		    rd16((paddr)find_symbol("_swapaddr")));
	printf("\n");

	a = find_symbol("_plist");
	if (a >= 0)
		dump_proc(a);

	a = find_symbol("_mttys");
	if (a >= 0)
		dump_tty(a);

	a = find_symbol("_mlist");
	if (a >= 0)
		dump_mount(a);

	a = find_symbol("_flist");
	if (a >= 0)
		dump_file(a);

	a = find_symbol("_ilist");
	if (a >= 0)
		dump_inode(a);

	a = find_symbol("_clist");
	if (a >= 0) {
		int cf = find_symbol("_cfree");
		dump_clist(a, cf >= 0 ? rd16((paddr)cf) : 0);
	}

	a = find_symbol("_biosw");
	if (a >= 0)
		dump_devsw(a, rd16((paddr)find_symbol("_nbdev")), 3,
		    "block device switch (_biosw)");

	a = find_symbol("_ciosw");
	if (a >= 0)
		dump_devsw(a, rd16((paddr)find_symbol("_ncdev")), 5,
		    "character device switch (_ciosw)");

	printf("\n== uart registers (sim ACE state) ==\n");
	multio_ace_dump();

	a = find_symbol("_blist");
	if (a >= 0) {
		int nbuf = find_symbol("_nbuf");
		dump_buf(a, nbuf >= 0 ? physread((paddr)nbuf) : 0);
	}
}

/*
 * SIGUSR2 handler and the latch the main loop polls.
 */
static volatile sig_atomic_t dump_request;

static void
dump_handler(int sig)
{
	dump_request = 1;
}

void
kdump_init(void)
{
	mysignal(SIGUSR2, dump_handler);
}

void
kdump_poll(void)
{
	if (dump_request) {
		dump_request = 0;
		printf("kernel tables (SIGUSR2):\n");
		dump_kernel_tables();
	}
}

#else /* NODEBUG: no symbols loaded, nothing to dump */

void
kdump_init(void)
{
}

void
kdump_poll(void)
{
}

#endif /* NODEBUG */
