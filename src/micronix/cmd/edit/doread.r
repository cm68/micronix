include common
# doread _ read "file" after "line"
	integer function doread(line, file)
	character file(MAXLINE), lin(MAXLINE)
	integer getlin, inject, open
	integer count, fd, line
	include clines

	fd = open(file, READ)
	if (fd == ERR)
		doread = ERR
	else {
		curln = line
		doread = OK
		for (count = 0; getlin(lin, fd) != EOF; count = count + 1) {
			doread = inject(lin)
			if (doread == ERR)
				break
			}
		call close(fd)
		call putdec(count, 1)
		call putc(NEWLINE)
		}
	return
	end
