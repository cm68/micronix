;
; upm syscall stubs
;
; cmd/upm/upmsys.s
;
; upm cannot use the ordinary libu stubs: those reach the simulator's
; syscall trap through RST 08h (0xcf), and upm's page zero belongs to
; the CP/M program.  So upm has its own, read out of the .dis, which
; reach the trap through a CALL to the shared HALT below.  The syscall
; number is the byte after the CALL - the opcode of the instruction that
; follows it - and the arguments sit in the next two words, right where
; the simulator reads them (sc+2 and sc+4).  Each stub fills those two
; words before it CALLs, which is the self-modifying bit the original
; does with its Hf874/Hf876 slots.
;
; The calling convention is the tree's own: the first argument arrives
; in hl and the rest on the stack.  The simulator reads the descriptor
; argument from hl and the other two from the inline slots, so a stub
; with arguments pops them, stores them into the slots, and restores
; the stack before the CALL.
;
	.extern _errno
	.extern _stab
	.global __exit
	.global _fork
	.global _read
	.global _write
	.global _open
	.global _close
	.global _wait
	.global _creat
	.global _link
	.global _unlink
	.global _exec
	.global _stat
	.global _seek
	.global _stty
	.global _gtty
	.global _access
	.global _dup
	.global _pipe
	.global __signal
	.global _jtab

	.text

;
; unix - the syscall trap.  Each stub CALLs here; the HALT is what the
; simulator catches, and the syscall number is the byte after the CALL.
;
unix:
	halt
	ret

;
; __exit(status) - the status is in hl, and this never returns.
;
__exit:
	call	unix
	.db	1
	ret

;
; _fork()
;
_fork:
	call	unix
	.db	2
	ret	nc
	ld	(_errno),hl
	ld	hl,-1
	ret

;
; _read(fd, buf, n)
;
_read:
	ld	(fdtmp),hl	; save fd
	pop	hl		; return address
	pop	de		; buf
	ld	(r_arg1),de
	pop	de		; n
	ld	(r_arg2),de
	ld	hl,-6		; restore the stack
	add	hl,sp
	ld	sp,hl
	ld	hl,(fdtmp)	; fd back into hl
	call	unix
	.db	3
r_arg1:	.ds	2
r_arg2:	.ds	2
	ret	nc
	ld	(_errno),hl
	ld	hl,-1
	ret

;
; _write(fd, buf, n)
;
_write:
	ld	(fdtmp),hl
	pop	hl
	pop	de
	ld	(w_arg1),de
	pop	de
	ld	(w_arg2),de
	ld	hl,-6
	add	hl,sp
	ld	sp,hl
	ld	hl,(fdtmp)
	call	unix
	.db	4
w_arg1:	.ds	2
w_arg2:	.ds	2
	ret	nc
	ld	(_errno),hl
	ld	hl,-1
	ret

;
; _open(name, mode)
;
_open:
	pop	de		; return address
	pop	de		; mode
	ld	(o_arg2),de
	ld	(o_arg1),hl	; name
	ld	hl,-4
	add	hl,sp
	ld	sp,hl
	call	unix
	.db	5
o_arg1:	.ds	2
o_arg2:	.ds	2
	ret	nc
	ld	(_errno),hl
	ld	hl,-1
	ret

;
; _close(fd)
;
_close:
	call	unix
	.db	6
	ret	nc
	ld	(_errno),hl
	ld	hl,-1
	ret

;
; _wait(pstat) - returns the child's pid in hl and its status in de,
; which is written to pstat when pstat is not null.
;
_wait:
	push	bc		; the caller's register variable
	push	hl		; pstat
	call	unix
	.db	7
	pop	bc		; pstat
	jr	c,w_err
	ld	a,b
	or	c
	jr	z,w_done	; pstat is null
	ld	a,e		; status low
	ld	(bc),a
	inc	bc
	ld	a,d		; status high
	ld	(bc),a
w_done:
	pop	bc
	ret
w_err:
	ld	(_errno),hl
	ld	hl,-1
	pop	bc
	ret

;
; _creat(name, mode)
;
_creat:
	pop	de
	pop	de
	ld	(c_arg2),de
	ld	(c_arg1),hl
	ld	hl,-4
	add	hl,sp
	ld	sp,hl
	call	unix
	.db	8
c_arg1:	.ds	2
c_arg2:	.ds	2
	ret	nc
	ld	(_errno),hl
	ld	hl,-1
	ret

;
; _link(name1, name2)
;
_link:
	pop	de
	pop	de
	ld	(l_arg2),de
	ld	(l_arg1),hl
	ld	hl,-4
	add	hl,sp
	ld	sp,hl
	call	unix
	.db	9
l_arg1:	.ds	2
l_arg2:	.ds	2
	ret	nc
	ld	(_errno),hl
	ld	hl,-1
	ret

;
; _unlink(name)
;
_unlink:
	pop	de		; return address
	ld	(u_arg1),hl	; name
	ld	hl,-2		; restore the stack (one pop)
	add	hl,sp
	ld	sp,hl
	call	unix
	.db	10
u_arg1:	.ds	2
	ret	nc
	ld	(_errno),hl
	ld	hl,-1
	ret

;
; _exec(name, argv)
;
_exec:
	pop	de
	pop	de
	ld	(e_arg2),de
	ld	(e_arg1),hl
	ld	hl,-4
	add	hl,sp
	ld	sp,hl
	call	unix
	.db	11
e_arg1:	.ds	2
e_arg2:	.ds	2
	ret	nc
	ld	(_errno),hl
	ld	hl,-1
	ret

;
; _stat(name, buf)
;
_stat:
	pop	de
	pop	de
	ld	(st_arg2),de
	ld	(st_arg1),hl
	ld	hl,-4
	add	hl,sp
	ld	sp,hl
	call	unix
	.db	18
st_arg1: .ds	2
st_arg2: .ds	2
	ret	nc
	ld	(_errno),hl
	ld	hl,-1
	ret

;
; _seek(fd, off, whence)
;
_seek:
	ld	(fdtmp),hl
	pop	hl
	pop	de
	ld	(sk_arg1),de
	pop	de
	ld	(sk_arg2),de
	ld	hl,-6
	add	hl,sp
	ld	sp,hl
	ld	hl,(fdtmp)
	call	unix
	.db	19
sk_arg1: .ds	2
sk_arg2: .ds	2
	ret	nc
	ld	(_errno),hl
	ld	hl,-1
	ret

;
; _stty(fd, buf)
;
_stty:
	ld	(fdtmp),hl
	pop	hl
	pop	de
	ld	(sy_arg1),de
	ld	hl,-4
	add	hl,sp
	ld	sp,hl
	ld	hl,(fdtmp)
	call	unix
	.db	31
sy_arg1: .ds	2
	ret	nc
	ld	(_errno),hl
	ld	hl,-1
	ret

;
; _gtty(fd, buf)
;
_gtty:
	ld	(fdtmp),hl
	pop	hl
	pop	de
	ld	(g_arg1),de
	ld	hl,-4
	add	hl,sp
	ld	sp,hl
	ld	hl,(fdtmp)
	call	unix
	.db	32
g_arg1:	.ds	2
	ret	nc
	ld	(_errno),hl
	ld	hl,-1
	ret

;
; _access(name, mode)
;
_access:
	pop	de
	pop	de
	ld	(a_arg2),de
	ld	(a_arg1),hl
	ld	hl,-4
	add	hl,sp
	ld	sp,hl
	call	unix
	.db	33
a_arg1:	.ds	2
a_arg2:	.ds	2
	ret	nc
	ld	(_errno),hl
	ld	hl,-1
	ret

;
; _dup(fd)
;
_dup:
	call	unix
	.db	41
	ret	nc
	ld	(_errno),hl
	ld	hl,-1
	ret

;
; _pipe(fds) - the syscall returns the read fd in hl and the write fd
; in de; write both into fds[2].
;
_pipe:
	push	hl		; save fds
	call	unix
	.db	42
	jr	c,p_err
	ex	de,hl		; read fd in de, write fd in hl
	ex	(sp),hl		; fds in hl, write fd to the stack
	ld	(hl),e
	inc	hl
	ld	(hl),d		; fds[0] = read fd
	inc	hl
	ex	(sp),hl		; write fd in hl, fds+1 to the stack
	ex	de,hl		; write fd in de, fds+1 in hl
	pop	hl		; fds+1 in hl
	ld	(hl),e
	inc	hl
	ld	(hl),d		; fds[1] = write fd
	ld	hl,0
	ret
p_err:
	pop	de		; drop the saved fds
	ld	(_errno),hl
	ld	hl,-1
	ret

;
; __signal(sig, handler) - the handler is a _jtab trampoline address.
;
__signal:
	pop	de		; return address
	pop	de		; handler
	ld	(sg_arg2),de
	ld	(sg_arg1),hl	; sig
	ld	hl,-4
	add	hl,sp
	ld	sp,hl
	call	unix
	.db	48
sg_arg1: .ds	2
sg_arg2: .ds	2
	ret	nc
	ld	(_errno),hl
	ld	hl,-1
	ret

;
; _jtab - fifteen six-byte trampolines, one per signal, each loading its
; handler out of _stab and falling into the common save-and-call tail.
; This is libu's, moved here so that _signal.o is not pulled in and its
; RST-based __signal does not fight this one.
;
_jtab:
	push	hl
	ld	hl,(_stab)
	jr	sigcall
	push	hl
	ld	hl,(_stab+2)
	jr	sigcall
	push	hl
	ld	hl,(_stab+4)
	jr	sigcall
	push	hl
	ld	hl,(_stab+6)
	jr	sigcall
	push	hl
	ld	hl,(_stab+8)
	jr	sigcall
	push	hl
	ld	hl,(_stab+10)
	jr	sigcall
	push	hl
	ld	hl,(_stab+12)
	jr	sigcall
	push	hl
	ld	hl,(_stab+14)
	jr	sigcall
	push	hl
	ld	hl,(_stab+16)
	jr	sigcall
	push	hl
	ld	hl,(_stab+18)
	jr	sigcall
	push	hl
	ld	hl,(_stab+20)
	jr	sigcall
	push	hl
	ld	hl,(_stab+22)
	jr	sigcall
	push	hl
	ld	hl,(_stab+24)
	jr	sigcall
	push	hl
	ld	hl,(_stab+26)
	jr	sigcall
	push	hl
	ld	hl,(_stab+28)
	jr	sigcall
sigcall:
	push	de
	push	bc
	exx
	push	hl
	push	de
	push	bc
	exx
	ex	af,af'
	push	af
	ex	af,af'
	push	af
	push	ix
	push	iy
	call	sjmp
	pop	iy
	pop	ix
	pop	af
	ex	af,af'
	pop	af
	ex	af,af'
	pop	af
	exx
	pop	bc
	pop	de
	pop	hl
	exx
	pop	bc
	pop	de
	pop	hl
	ret
sjmp:
	jp	(hl)

	.text
fdtmp:	.ds	2

; vim: tabstop=8 shiftwidth=8 noexpandtab:
