/*
 * rewind a stdio stream
 *
 */
#include	<stdio.h>

rewind(stream)
FILE *	stream;
{
	fseek(stream, 0L, 0);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
