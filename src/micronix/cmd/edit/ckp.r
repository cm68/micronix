include common
# ckp _ check for "p" after command
	integer function ckp(lin, i, pflag, status)
	character lin(MAXLINE)
	integer i, j, pflag, status

	j = i
	if (lin(j) == PRINT) {
		j = j + 1
		pflag = YES
		}
	else
		pflag = NO
	if (lin(j) == NEWLINE)
		status = OK
	else
		status = ERR
	ckp = status
	return
	end
