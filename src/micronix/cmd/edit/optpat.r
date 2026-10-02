include common
# optpat _ make pattern if specified at lin(i)
	integer function optpat(lin, i)
	character lin(MAXLINE)
	integer makpat
	integer i
	include cpat

	if (lin(i) == EOS)
		i = ERR
	else if (lin(i + 1) == EOS)
		i = ERR
	else if (lin(i + 1) == lin(i))	# repeated delimiter
		i = i + 1			# leave existing pattern alone
	else
		i = makpat(lin, i + 1, lin(i), pat)
	if (pat(1) == EOS)
		i = ERR
	if (i == ERR) {
		pat(1) = EOS
		optpat = ERR
		}
	else
		optpat = OK
	return
	end
