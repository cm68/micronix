include common
# maklin (scratch file) _ make new line entry, copy text to scratch
	integer function maklin(lin, i, newind)
	character lin(MAXLINE)
	integer addset, length
	integer i, j, junk, newind, txtend
	include cbuf
	include cscrat
	include ctxt

	maklin = ERR
	if (lastbf + BUFENT > MAXBUF)
		return			# no room for new line entry
	txtend = 1
	for (j = i; lin(j) != EOS; ) {
		junk = addset(lin(j), txt, txtend, MAXLINE)
		j = j + 1
		if (lin(j - 1) == NEWLINE)
			break
		}
	if (addset(EOS, txt, txtend, MAXLINE) == NO)
		return
	call seek(scrend, scr)	# add line to end of scratch file
	buf(lastbf + SEEKADR) = scrend
	buf(lastbf + LENG) = length(txt)
	call putlin(txt, scr)
	scrend = scrend + buf(lastbf + LENG)
	buf(lastbf + MARK) = NO
	newind = lastbf
	lastbf = lastbf + BUFENT
	maklin = j			# next character to be examined in lin
	return
	end
