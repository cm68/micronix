include common
# subst _ substitute "sub" for occurrences of pattern
	integer function subst(sub, gflag)
	character new(MAXLINE), sub(MAXPAT)
	integer addset, amatch, gettxt, inject
	integer gflag, j, junk, k, lastm, line, m, status, subbed
	include clines
	include cpat
	include ctxt

	subst = ERR
	if (line1 <= 0)
		return
	for (line = line1; line <= line2; line = line + 1) {
		j = 1
		subbed = NO
		junk = gettxt(line)
		lastm = 0
		for (k = 1; txt(k) != EOS; ) {
			if (gflag == YES | subbed == NO)
				m = amatch(txt, k, pat)
			else
				m = 0
			if (m > 0 & lastm != m) {	# replace matched text
				subbed = YES
				call catsub(txt, k, m, sub, new, j, MAXLINE)
				lastm = m
				}
			if (m == 0 | m == k) {	# no match or null match
				junk = addset(txt(k), new, j, MAXLINE)
				k = k + 1
				}
			else				# skip matched text
				k = m
			}
		if (subbed == YES) {
			if (addset(EOS, new, j, MAXLINE) == NO) {
				subst = ERR
				break
				}
			call delete(line, line, status)	# remembers dot
			subst = inject(new)
line2 = line2+curln-line
line = curln
			if (subst == ERR)
				break
			subst = OK
			}
		}
	return
	end
