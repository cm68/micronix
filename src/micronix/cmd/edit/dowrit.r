include common
# dowrit _ write "from" through "to" into file
	integer function dowrit(from, to, file)
	character file(MAXLINE)
	integer create, gettxt
	integer fd, from, k, line, to
	include ctxt

	fd = create(file, WRITE)
	if (fd == ERR)
		dowrit = ERR
	else {
		for (line = from; line <= to; line = line + 1) {
			k = gettxt(line)
			call putlin(txt, fd)
			}
		call close(fd)
		call putdec(to-from+1, 1)
		call putc(NEWLINE)
		dowrit = OK
		}
	return
	end
