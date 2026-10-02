include common
define NOSTATUS	1
# append _ append lines after "line"
	integer function append(line, glob)
	character lin(MAXLINE)
	integer getlin, inject
	integer line, glob
	include clines

	if (glob == YES)
		append = ERR
	else {
		curln = line
		for (append = NOSTATUS; append == NOSTATUS; )
			if (getlin(lin, STDIN) == EOF)
				append = EOF
			else if (lin(1) == PERIOD & lin(2) == NEWLINE)
				append = OK
			else if (inject(lin) == ERR)
				append = ERR
		}
	return
	end
