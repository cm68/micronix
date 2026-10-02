include common
# getlst _ collect line numbers (if any) at lin(i), increment i
	integer function getlst(lin, i, status)
	character lin(MAXLINE)
	integer getone, min
	integer i, num, status
	include clines

	line2 = 0
	for (nlines = 0; getone(lin, i, num, status) == OK; ) {
		line1 = line2
		line2 = num
		nlines = nlines + 1
		if (lin(i) != COMMA & lin(i) != SEMICOL)
			break
		if (lin(i) == SEMICOL)
			curln = num
		i = i + 1
		}
	nlines = min(nlines, 2)
	if (nlines == 0)
		line2 = curln
	if (nlines <= 1)
		line1 = line2
	if (status != ERR)
		status = OK
	getlst = status
	return
	end
