include common
# ptscan _ scan for next occurrence of pattern
	integer function ptscan(way, num)
	integer gettxt, match, nextln, prevln
	integer k, num, way
	include clines
	include cpat
	include ctxt

	num = curln
	repeat {
		if (way == FORWARD)
			num = nextln(num)
		else
			num = prevln(num)
		k = gettxt(num)
		if (match(txt, pat) == YES) {
			ptscan = OK
			return
			}
		} until (num == curln)
	ptscan = ERR
	return
	end
