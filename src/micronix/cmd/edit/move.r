include common
# move _ move line1 through line2 after line3
	integer function move(line3)
	integer getind, nextln, prevln
	integer k0, k1, k2, k3, k4, k5, line3
	include clines

	if (line1 <= 0 | (line1 <= line3 & line3 <= line2))
		move = ERR
	else {
		k0 = getind(prevln(line1))
		k3 = getind(nextln(line2))
		k1 = getind(line1)
		k2 = getind(line2)
		call relink(k0, k3, k0, k3)
		if (line3 > line1) {
			curln = line3
			line3 = line3 - (line2 - line1 + 1)
			}
		else
			curln = line3 + (line2 - line1 + 1)
		k4 = getind(line3)
		k5 = getind(nextln(line3))
		call relink(k4, k1, k2, k5)
		call relink(k2, k5, k4, k1)
		move = OK
		}
	return
	end
