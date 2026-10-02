include common
# getfn _ get file name from lin(i)...
	integer function getfn(lin, i, file)
	character lin(MAXLINE), file(MAXLINE)
	integer i, j, k
	include cfile

	getfn = ERR
	if (lin(i + 1) == BLANK) {
		j = i + 2		# get new file name
		call skipbl(lin, j)
		for (k = 1; lin(j) != NEWLINE; k = k + 1) {
			file(k) = lin(j)
			j = j + 1
			}
		file(k) = EOS
		if (k > 1)
			getfn = OK
		}
	else if (lin(i + 1) == NEWLINE & savfil(1) != EOS) {
		call scopy(savfil, 1, file, 1)	# or old name
		getfn = OK
		}
	# else error
	if (getfn == OK & savfil(1) == EOS)
		call scopy(file, 1, savfil, 1)	# save if no old one
	return
	end
