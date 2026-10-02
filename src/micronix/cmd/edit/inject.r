include common
# inject  (scratch file) _ insert lin after curln, write scratch
	integer function inject(lin)
	character lin(MAXLINE)
	integer getind, maklin, nextln
	integer i, k1, k2, k3
	include clines

	for (i = 1; lin(i) != EOS; ) {
		i = maklin(lin, i, k3)
		if (i == ERR) {
			inject = ERR
			break
			}
		k1 = getind(curln)
		k2 = getind(nextln(curln))
		call relink(k1, k3, k3, k2)
		call relink(k3, k2, k1, k3)
		curln = curln + 1
		lastln = lastln + 1
		inject = OK
		}
	return
	end
