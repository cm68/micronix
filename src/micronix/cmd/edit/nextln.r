# nextln _ get line after "line"
	integer function nextln(line)
	integer line
	include clines

	nextln = line + 1
	if (nextln > lastln)
		nextln = 0
	return
	end
