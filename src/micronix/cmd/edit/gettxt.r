include common
# gettxt (scratch file) _ locate text for line, copy to txt
	integer function gettxt(line)
	integer getbuf, getind
	integer j, k, line
	include cbuf
	include cscrat
	include ctxt

	k = getind(line)
	call seek(buf(k + SEEKADR), scr)
	call readf(txt, buf(k + LENG), scr)
	j = buf(k + LENG) + 1
	txt(j) = EOS
	gettxt = k
	return
	end
