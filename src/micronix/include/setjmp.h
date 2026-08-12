#if	z80
/*
 * Five words: IY, IX, BC, the return address, and SP - everything
 * the calling convention makes a callee keep for its caller, which
 * is exactly what has to come back to life when setjmp returns the
 * second time.  See libc/longjmp.s for the layout.
 */
typedef	int	jmp_buf[5];
#endif

#if	i8086
typedef	int	jmp_buf[8];
#endif

#if	i8096
typedef	int	jmp_buf[10];
#endif

#if	m68k
typedef	int	jmp_buf[10];
#endif

extern	int	setjmp(jmp_buf);
extern void	longjmp(jmp_buf, int);

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
