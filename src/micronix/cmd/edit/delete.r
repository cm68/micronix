include common
# delete _ delete lines from through to
	integer function delete(from, to, status)
	integer getind, nextln, prevln
	integer from, k1, k2, status, to
	include clines

	if (from <= 0)
		status = ERR
	else {
		k1 = getind(prevln(from))
		k2 = getind(nextln(to))
		lastln = lastln - (to - from + 1)
		curln = prevln(from)
		call relink(k1, k2, k1, k2)
		status = OK
		}
	delete = status
	return
	end
