include common
# setbuf (scratch file) _ create scratch file, set up line 0
	subroutine setbuf
	integer create
	integer k
	include cbuf
	include clines
	include cscrat
#	string scrfil "scratch"
	integer scrfil(8)
	data scrfil(1)/LETS/
	data scrfil(2)/LETC/
	data scrfil(3)/LETR/
	data scrfil(4)/LETA/
	data scrfil(5)/LETT/
	data scrfil(6)/LETC/
	data scrfil(7)/LETH/
	data scrfil(8)/EOS/
#	string null ""
	integer null(1)
	data null(1) /EOS/

	scr = create(scrfil, READWRITE)
	if (scr == ERR)
		call cant(scrfil)
	scrend = 0
	lastbf = LINE0
	call maklin(null, 1, k)	# create empty line 0
	call relink(k, k, k, k)		# establish initial linked list
	curln = 0
	lastln = 0
	return
	end
