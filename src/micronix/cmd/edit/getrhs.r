include common
# getrhs _ get substitution string for "s" command
	integer function getrhs(lin, i, sub, gflag)
	character lin(MAXLINE), sub(MAXPAT)
	integer maksub
	integer gflag, i

	getrhs = ERR
	if (lin(i) == EOS)
		return
	if (lin(i + 1) == EOS)
		return
	i = maksub(lin, i + 1, lin(i), sub)
	if (i == ERR)
		return
	if (lin(i + 1) == GLOBAL) {
		i = i + 1
		gflag = YES
		}
	else
		gflag = NO
	getrhs = OK
	return
	end
