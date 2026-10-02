include common
# doprnt _ print lines from through to
	integer function doprnt(from, to)
	integer gettxt
	integer from, i, j, to
	include clines
	include ctxt

	if (from <= 0)
		doprnt = ERR
	else {
		for (i = from; i <= to; i = i + 1) {
			j = gettxt(i)
			call putlin(txt, STDOUT)
			}
		curln = to
		doprnt = OK
		}
	return
	end
