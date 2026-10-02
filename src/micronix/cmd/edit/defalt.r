include common
# defalt _ set defaulted line numbers
	integer function defalt(def1, def2, status)
	integer def1, def2, status
	include clines

	if (nlines == 0) {
		line1 = def1
		line2 = def2
		}
	if (line1 > line2 | line1 <= 0)
		status = ERR
	else
		status = OK
	defalt = status
	return
	end
