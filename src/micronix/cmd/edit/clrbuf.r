include common
# clrbuf  (scratch file) _ dispose of scratch file
	subroutine clrbuf
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

	call close(scr)
	call remove(scrfil)
	return
	end
