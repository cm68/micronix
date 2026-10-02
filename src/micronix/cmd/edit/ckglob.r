include common
define EXCLUDE LETX
# ckglob _ if global prefix, mark lines to be affected
	integer function ckglob(lin, i, status)
	character lin(MAXLINE)
	integer defalt, getind, gettxt, match, nextln, optpat
	integer gflag, i, k, line, status
	include cbuf
	include clines
	include cpat
	include ctxt

	if (lin(i) != GLOBAL & lin(i) != EXCLUDE)
		status = EOF
	else {
		if (lin(i) == GLOBAL)
			gflag = YES
		else
			gflag = NO
		i = i + 1
		if (optpat(lin, i) == ERR | defalt(1, lastln, status) == ERR)
			status = ERR
		else {
			i = i + 1
			for (line = line1; line <= line2; line = line + 1) {
				k = gettxt(line)
				if (match(txt, pat) == gflag)
					buf(k+MARK) = YES
				else
					buf(k+MARK) = NO
				}
			for (line = nextln(line2); line != line1; line = nextln(line)) {
				k = getind(line)
				buf(k+MARK) = NO
				}
			status = OK
			}
		}
	ckglob = status
	return
	end
