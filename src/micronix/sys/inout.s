

INTOC	:= &0x48ED		/Z80 input port (c) to c
OUTA	:= &0x79ED		/Z80 output a to port (c)

/in(port)
_in:
	/
	sp => hl => bc <= bc <= hl;
	INTOC			/Z80 input port (c) to c
	b = 0
	ret;
	/


/out(port, data)
_out:
	/
	c = *(hl = 2 + sp);
	a = *(hl +1+1)		/a = data
	OUTA			/Z80 output a to port (c)
	ret;
	/
