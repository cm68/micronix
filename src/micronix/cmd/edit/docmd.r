include common
define APPENDCOM LETA
define CHANGE LETC
define DELCOM LETD
define ENTER LETE
define PRINTFIL LETF
define READCOM LETR
define WRITECOM LETW
define INSERT LETI
define PRINTCUR EQUALS
define MOVECOM LETM
define QUIT LETQ
define SUBSTITUTE LETS
# docmd _ handle all commands except globals
	integer function docmd(lin, i, glob, status)
	character file(MAXLINE), lin(MAXLINE), sub(MAXPAT)
	integer append, delete, doprnt, doread, dowrit, move, subst
	integer ckp, defalt, getfn, getone, getrhs, nextln, optpat, prevln
	integer gflag, glob, i, line3, pflag, status
	include cfile
	include clines
	include cpat

	pflag = NO		# may be set by d, m, s
	status = ERR
	if (lin(i) == APPENDCOM) {
		if (lin(i + 1) == NEWLINE)
			status = append(line2, glob)
		}
	else if (lin(i) == CHANGE) {
		if (lin(i + 1) == NEWLINE)
		  andif (defalt(curln, curln, status) == OK)
		  andif (delete(line1, line2, status) == OK)
			status = append(prevln(line1), glob)
		}
	else if (lin(i) == DELCOM) {
		if (ckp(lin, i + 1, pflag, status) == OK)
		  andif (defalt(curln, curln, status) == OK)
		  andif (delete(line1, line2, status) == OK)
		  andif (nextln(curln) != 0)
			curln = nextln(curln)
		}
	else if (lin(i) == INSERT) {
		if (lin(i + 1) == NEWLINE)
			status = append(prevln(line2), glob)
		}
	else if (lin(i) == PRINTCUR) {
		if (ckp(lin, i + 1, pflag, status) == OK) {
			call putdec(line2, 1)
			call putc(NEWLINE)
			}
		}
	else if (lin(i) == MOVECOM) {
		i = i + 1
		if (getone(lin, i, line3, status) == EOF)
			status = ERR
		if (status == OK)
		  andif (ckp(lin, i, pflag, status) == OK)
		  andif (defalt(curln, curln, status) == OK)
			status = move(line3)
		}
	else if (lin(i) == SUBSTITUTE) {
		i = i + 1
		if (optpat(lin, i) == OK)
		  andif (getrhs(lin, i, sub, gflag) == OK)
		  andif (ckp(lin, i + 1, pflag, status) == OK)
		  andif (defalt(curln, curln, status) == OK)
			status = subst(sub, gflag)
		}
	else if (lin(i) == ENTER) {
		if (nlines == 0)
		  andif (getfn(lin, i, file) == OK) {
			call scopy(file, 1, savfil, 1)
			call clrbuf
			call setbuf
			status = doread(0, file)
			}
		}
	else if (lin(i) == PRINTFIL) {
		if (nlines == 0)
		  andif (getfn(lin, i, file) == OK) {
			call scopy(file, 1, savfil, 1)
			call putlin(savfil, STDOUT)
			call putc(NEWLINE)
			status = OK
			}
		}
	else if (lin(i) == READCOM) {
		if (getfn(lin, i, file) == OK)
			status = doread(line2, file)
		}
	else if (lin(i) == WRITECOM) {
		if (getfn(lin, i, file) == OK)
		  andif (defalt(1, lastln, status) == OK)
			status = dowrit(line1, line2, file)
		}
	else if (lin(i) == PRINT) {
		if (lin(i + 1) == NEWLINE)
		  andif (defalt(curln, curln, status) == OK)
			status = doprnt(line1, line2)
		}
	else if (lin(i) == NEWLINE) {
		if (nlines == 0)
			line2 = nextln(curln)
		status = doprnt(line2, line2)
		}
	else if (lin(i) == QUIT) {
		if (lin(i + 1) == NEWLINE & nlines == 0 & glob == NO)
			status = EOF
		}
	# else status is ERR
	if (status == OK & pflag == YES)
		status = doprnt(curln, curln)
	docmd = status
	return
	end
