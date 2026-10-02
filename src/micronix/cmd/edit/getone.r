include common
# getone _ evaluate one line number expression
	integer function getone(lin, i, num, status)
	character lin(MAXLINE)
	integer getnum
	integer i, istart, mul, num, pnum, status
	include clines

	istart = i
	num = 0
	call skipbl(lin, i)
	if (getnum(lin, i, num, status) == OK)	# first term
		repeat {				# + or - terms
			call skipbl(lin, i)
			if (lin(i) != PLUS & lin(i) != MINUS) {
				status = EOF
				break
				}
			if (lin(i) == PLUS)
				mul = +1
			else
				mul = -1
			i = i + 1
			call skipbl(lin, i)
			if (getnum(lin, i, pnum, status) == OK)
				num = num + mul * pnum
			if (status == EOF)
				status = ERR
			} until (status != OK)
	if (num < 0 | num > lastln)
		status = ERR

	if (status == ERR)
		getone = ERR
	else if (i <= istart)
		getone = EOF
	else
		getone = OK

	status = getone
	return
	end
