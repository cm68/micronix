include common
define CURLINE PERIOD
define LASTLINE DOLLAR
define SCAN SLASH
define BACKSCAN BACKSLASH
# getnum _ convert one term to line number
	integer function getnum(lin, i, pnum, status)
	character lin(MAXLINE)
	integer ctoi, index, optpat, ptscan
	integer i, pnum, status
	include clines
	include cpat
#	string digits "0123456789"
	integer digits(11)
	data digits(01)/DIG0/
	data digits(02)/DIG1/
	data digits(03)/DIG2/
	data digits(04)/DIG3/
	data digits(05)/DIG4/
	data digits(06)/DIG5/
	data digits(07)/DIG6/
	data digits(08)/DIG7/
	data digits(09)/DIG8/
	data digits(10)/DIG9/
	data digits(11)/EOS/

	getnum = OK
	if (index(digits, lin(i)) > 0) {
		pnum = ctoi(lin, i)
		i = i - 1	# move back; to be advanced at the end
		}
	else if (lin(i) == CURLINE)
		pnum = curln
	else if (lin(i) == LASTLINE)
		pnum = lastln
	else if (lin(i) == SCAN | lin(i) == BACKSCAN) {
		if (optpat(lin, i) == ERR)	# build the pattern
			getnum = ERR
		else if (lin(i) == SCAN)
			getnum = ptscan(FORWARD, pnum)
		else
			getnum = ptscan(BACKWARD, pnum)
		}
	else
		getnum = EOF
	if (getnum == OK)
		i = i + 1	# point at next character to be examined
	status = getnum
	return
	end
