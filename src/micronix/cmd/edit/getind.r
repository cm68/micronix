include common
# getind _ locate line index in buffer
	integer function getind(line)
	integer j, k, line
	include cbuf

	k = LINE0
	for (j = 0; j < line; j = j + 1)
		k = buf(k + NEXT)
	getind = k
	return
	end
