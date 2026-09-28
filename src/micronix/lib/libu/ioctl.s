;
; ioctl system call
;
; ioctl(fd, cmd, req)
; int fd, cmd;
; struct cdb *req;
;
; Runs a raw command block against a block device.  The descriptor
; names the device, so the call reaches whichever driver serves that
; device's major number, and cmd says what to do with req - today the
; only request is CDBCMD, which hands req's command block to the bus
; the device sits on and moves req's data phase for it.
;
; The command block is the bus's own: a SCSI CDB, or the seven task
; file registers of an ATA command.  What the bytes mean is between
; the caller and the drive, and neither the kernel nor this stub looks
; at them - see include/sys/ioctl.h for the structure and for why it
; is shaped the way it is.
;
; This is the block-device half of a call that has been a character
; device's since the sixth edition: stty and gtty (see stty.s) are
; what ioctl(2) meant then, and they are still the way a terminal is
; asked anything.  A disk is not a terminal and is not reached through
; the character switch, so a command block for one needs its own
; request code on its own side of that split.
;
; passes fd in hl
;
; returns 0 on success, -1 on failure
;
	.extern _errno
	.global _ioctl

	.text
_ioctl:
	ld 	a,l		; fd arrives in hl
	pop 	hl		; discard ret addr
	pop 	hl		; cmd
	ld 	(cmd),hl
	pop 	hl		; req
	ld 	(req),hl

;
; Six: three pops moved sp up by that much, and the kernel is entered
; with sp where it was on entry so the return address it finds is this
; call's caller.  An off-by-one-word here returns to the wrong place -
; see kill.s, where the same arithmetic carries the same warning.
;
	ld	hl,-6		; restore stack
	add	hl,sp
	ld	sp,hl

	ld 	l,a		; fd in hl
	ld 	h,0
	rst 	08h
	.db 	000h
	.dw 	scall
	ex 	de,hl		; the error, if there is one, is in hl
	ld 	hl,0
	ret 	nc
	ld 	(_errno),de
	dec 	hl
	ret

	.data
scall:	.db 	0cfh
	.db 	036h
cmd:	.dw 	0
req:	.dw 	0

; vim: tabstop=8 shiftwidth=8 noexpandtab:
