include common
# relink _ rewrite two half links
	subroutine relink(a, x, y, b)
	integer a, b, x, y
	include cbuf

	buf(x + PREV) = a
	buf(y + NEXT) = b
	return
	end
