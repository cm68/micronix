include common
# doglob _ do command at lin(i) on all marked lines
	integer function doglob(lin, i, cursav, status)
	character lin(MAXLINE)
	integer docmd, getind, getlst, nextln
	integer count, cursav, i, istart, k, line, status
	include cbuf
	include clines

	status = OK
	count = 0
	line = line1
	istart = i
	repeat {
		k = getind(line)
		if (buf(k+MARK) == YES) {
			buf(k+MARK) = NO
			curln = line
			cursav = curln
			i = istart
			if (getlst(lin, i, status) == OK)
			  andif (docmd(lin, i, YES, status) == OK)
				count = 0
			}
		else {
			line = nextln(line)
			count = count + 1
			}
		} until (count > lastln | status != OK)
	doglob = status
	return
	end
