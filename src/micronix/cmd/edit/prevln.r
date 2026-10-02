# prevln _ get line before "line"
	integer function prevln(line)
	integer line
	include clines

	prevln = line - 1
	if (prevln < 0)
		prevln = lastln
	return
	end
