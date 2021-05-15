/*
 *	SCCS id	@(#)rkboot.s	1.0 (2.11BSD)	12/26/2008
 */
#include "localopts.h"

/  The boot options and device are placed in the last SZFLAGS bytes
/  at the end of core for the bootstrap.
ENDCORE=	160000		/ end of core, mem. management off
SZFLAGS=	6		/ size of boot flags
BOOTOPTS=	2		/ location of options, bytes below ENDCORE
BOOTDEV=	4		/ boot unit
CHECKWORD=	6

.globl	_doboot, hardboot, _bootcsr
.text
_doboot:
	mov	4(sp),r4	/ boot options
	mov	2(sp),r3	/ boot device

#ifndef	KERN_NONSEP
/  If running separate I/D, need to turn off memory management.
/  Call the routine unmap in low text, after setting up a jump
/  in low data where the PC will be pointing.
.globl	unmap
	mov	$137,*$unmap+2		/ jmp *$hardboot
	mov	$hardboot,*$unmap+4
	jmp	unmap
	/ "return" from unmap will be to hardboot in data
.data
#else
/  Reset to turn off memory management
	reset
#endif

/  On power fail, hardboot is the entry point (map is already off)
/  and the args are in r4 (RB_POWRFAIL), r3 (rootdev)

hardboot:
	mov	r4, ENDCORE-BOOTOPTS
	ash	$-3,r3		/ shift out the partition number
	bic	$!7,r3		/ save only the drive number
	mov	r3, ENDCORE-BOOTDEV
	com	r4		/ if CHECKWORD == ~bootopts, flags are believed
	mov	r4, ENDCORE-CHECKWORD
1:
	reset

/  The remainder of the code is dependent on the boot device.
/  If you have a bootstrap ROM, just jump to the correct entry.
/  Otherwise, use a BOOT opcode, if available;
/  if necessary, read in block 0 to location 0 "by hand".

/ Bootstrap for rk05 drive - wfjm

WC = -256.

rkcs =  4	/ offset from base csr: control & status
rkda = 12	/ desired disk address

/ RK05 constants.
iocom = 005	/ read + go

/ initialize rk

	mov	_bootcsr,r1	/ bootcsr points to rkcs
	add	$rkda-rkcs,r1	/ r1 -> rkda
	mov	ENDCORE-BOOTDEV,r2	/ drive number
	ash	$13,r2		/ drsel is in bits 15:13
	mov	r2,(r1)		/ setup rkda (disk address)
	clr	-(r1)		/ clear rkba (memory address)
	mov	$WC,-(r1)	/ setup rkwc (transfer length)
	mov	$iocom,-(r1)	/ issue read+go

1:	tstb	(r1)
	bge	1b		/ wait for iocom to complete
	mov	ENDCORE-BOOTDEV,r0
	clr	pc
