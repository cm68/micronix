#
/*
 * system shims the kernel does not provide
 */

#include	"defs.h"
#include	"dup.h"

INT		shmask;

/* dup2 semantics: the shell wants dup(fa|DUPFLG, fb) */
VOID	dup2(fa, fb)
	REG INT		fa, fb;
{
	REG INT		fd;
	REG INT		n = 0;
	INT		held[16];

	IF fa==fb THEN return FI
	close(fb);
	WHILE (fd=dup(fa))<fb
	DO	IF fd<0 THEN return FI
		IF n<16 THEN held[n++]=fd FI
	OD
	IF fd<0 THEN return FI
	/* close only what this call took: the range below fb can hold
	   descriptors the shell already owns */
	WHILE n>0 DO close(held[--n]) OD
}

/* there is no umask syscall; keep the value for creat() */
INT	umask(m)
	INT		m;
{
	INT		old = shmask;
	IF m>=0 THEN shmask = m FI
	return(old);
}
