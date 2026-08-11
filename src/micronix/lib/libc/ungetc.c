/*
 * ungetc - put a character back into stdio stream
 *
 */
#include	<stdio.h>

ungetc(c, stream)
int		c;
register FILE *	stream;
{
	if (c == EOF || !(stream->_flag & _IOREAD) ||
		stream->_base == (char *)NULL || stream->_cnt == BUFSIZ)
		return(EOF);
	if (stream->_ptr == stream->_base)
		stream->_ptr++;
	else
		stream->_cnt++;
	*--stream->_ptr = c;
	return(c);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
