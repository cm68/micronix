include common
# edit _ main routine
	character lin(MAXLINE)
	integer ckglob, docmd, doglob, doread, getarg, getlin, getlst
	integer cursav, i, status
	include cfile
	include clines
	include cpat

	call setbuf
	pat(1) = EOS
	savfil(1) = EOS
	if (getarg(1, savfil, MAXLINE) != EOF)
		if (doread(0, savfil) == ERR)
			call remark("?.")
	while (getlin(lin, STDIN) != EOF) {
		i = 1
		cursav = curln
		if (getlst(lin, i, status) == OK) {
			if (ckglob(lin, i, status) == OK)
				status = doglob(lin, i, cursav, status)
			else if (status != ERR)
				status = docmd(lin, i, NO, status)
			# else error, do nothing
			}
		if (status == ERR) {
			call remark("?.")
			curln = cursav
			}
		else if (status == EOF)
			break
		# else OK, loop
		}
	call clrbuf
	stop
	end
