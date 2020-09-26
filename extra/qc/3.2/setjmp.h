/*
 * setjmp.h - header file for setjmp()/longjmp()
 *
 *	format of buffer to hold environment:
 *
 *		*--------------------*
 *		* frame pointer (BC) *
 *		*--------------------*
 *		*   RETurn address   *
 *		*--------------------*
 *		*  contents of cell  *
 *		* above stack pointer*
 *		*--------------------*
 *		* stack pointer (SP) *
 *		*--------------------*
 */

typedef char *jmp_buf[4];

/* end setjmp.h */
