/*
 * low level memory access
 *
 * sys/mem.s
 * Changed: <2022-01-04 10:36:15 curt>
 */

public	_getbyte
public	_getword
public	_putbyte
public	_putword
public	_copyin
public	_copyout
public	_memrw
public	_segcopy
public	_meminit

MAP0	:= &0x0600		/task zero window registers
IMAGE0	:= &0x0200		/readable version of MAP0

MAP1	:= &0x0620		/task ones map registers
IMAGE1	:= &0x0220		/task ones readable map registers

LDIR	:= &0xB0ED		/z80 do *de++ = *hl++ while --bc != 0


/ getbyte(addr) -- Get a byte from the active _task

_getbyte:
	hl = 2 + sp
	hl =a^ hl		/hl = addr

	call getw		/bc = word at address hl

	b = 0
	ret


/ getword(addr) -- Get a word from the active _task

_getword:
	hl = 2 + sp
	hl =a^ hl		/hl = addr
				/drop thru to getw

/ getw	  bc = word at address hl
/ Copy the appropriate user map registers to the windows,
/ replace the high nibble of addr with the window nibble,
/ get the data, and restore the _windows

getw:
	a = h & 0xF0		/a = high nibble of addr (times 16)
	a <*> -1		/rotate a right 3 times w/o carry
	a <*> -1
	a <*> -1		/now a = 2 * high nibble of addr

	bc = IMAGE1		/task ones map
	a + c -> c		/bc points to map reg for desired word

	a = h & 0x0F		/remove high nibble from addr
	a | 0x10 -> h		/and replace it with window nibble

	call _di
	a = *bc -> *MAP0[2]	/now task zero can see desired word
	bc +1 +1		/Set up MAP0[4] in case the word
	a = *bc -> *MAP0[4]	/crosses a segment boundary

	bc =^ hl		/get word
	a = *IMAGE0[2]		/restore task 0s windows
	a -> *MAP0[2]
	a = *IMAGE0[4]
	a -> *MAP0[4]

	jmp _ei


/ putbyte(data, addr) -- Put a byte to the active _task

_putbyte:
	hl = 2 + sp
	bc =^ hl		/bc = data
	hl =a^ (hl+1)		/hl = addr
				/drop thru to putb

/ putb	  put byte c to address hl
/ Copy the appropriate user map register to the window,
/ replace the high nibble of addr with the window nibble,
/ put the data, and restore the _window

putb:
	sp <= de		/save de

	a = h & 0xF0		/a = high nibble of addr (times 16)
	a <*> -1		/rotate a right 3 times w/o carry
	a <*> -1
	a <*> -1		/now a = 2 * high nibble of addr

	de = IMAGE1		/task ones map
	a + e -> e		/de points to map reg for desired byte

	a = h & 0x0F		/remove high nibble from addr
	a | 0x10 -> h		/and replace it with window nibble

	call _di
	a = *de -> *MAP0[2]	/now task zero can see desired address
	c -> *hl		/put data
	a = *IMAGE0[2] -> *MAP0[2]	/restore task zeros window

	sp => de
	jmp _ei


/ putword(data, addr) -- Put a word to the active _task

_putword:

	hl = 2 + sp		/get arguments from stack
	bc =^ hl		/bc = data
	hl =a^ (hl+1)		/hl = addr

	sp <= bc <= hl
	call putb		/put c

	sp => hl => bc
	hl +1; c = b
	jmp putb		/put b


/ copyin(from, to, count)
/ Copy count bytes from from in the active task
/ to to in the kernel. Limited to 4K bytes per crack.

_copyin:
	de => sp			/save C frame pointer
	a = *IMAGE0[2]; af => sp	/save images so interrupt code
	a = *IMAGE0[4]; af => sp	/ can use the windows

	hl = 13 + sp		/get arguments from stack
	b = *hl
	c = *(hl-1)		/bc = count
	d = *(hl-1)
	e = *(hl-1)		/de = to
	a = *(hl-1)
	l = *(hl-1)
	h = a			/hl = from

	a = h & 0xF0		/a = high nibble of from (times 16)
	a <*> -1		/rotate a right 3 times w/o carry
	a <*> -1
	a <*> -1		/now a = 2 * high nibble of from

	bc => sp		/save count

	bc = IMAGE1		/task ones map
	a + c -> c		/bc points to map reg for source

	call _di
	a = *bc
	a -> *IMAGE0[2] -> *MAP0[2]	/now task zero can see user space
	a = *(bc +1 +1)			/Set up MAP0[4] in case copy crosses
	a -> *IMAGE0[4] -> *MAP0[4]	/ a segment boundary

	bc <= sp			/restore count to bc

	a = h & 0x0F			/remove high nibble from from
	a | 0x10 -> h			/and replace it with window nibble

	a = b | c			/test for zero count
	jz .4

	LDIR				/z80 do *de++ = *hl++ while --bc != 0

.4:
	af <= sp			/restore windows
	a -> *IMAGE0[4] -> *MAP0[4]
	af <= sp
	a -> *IMAGE0[2] -> *MAP0[2]

	de <= sp		/recover C frame pointer
	jmp _ei

/ copyout(from, to, count)
/ Copy count bytes from from in the kernel
/ to to in the active task. Limited to 4K bytes per crack.

_copyout:
	de => sp		/save C frame pointer
	a = *IMAGE0[2]; af => sp	/save images so interrupt code
	a = *IMAGE0[4]; af => sp	/ can use the windows

	hl = 13 + sp		/get arguments from stack
	b = *hl
	c = *(hl-1)		/bc = count
	d = *(hl-1)
	e = *(hl-1)		/de = to
	a = *(hl-1)
	l = *(hl-1)
	h = a			/hl = from

	a = d & 0xF0		/a = high nibble of to (times 16)
	a <*> -1		/rotate a right 3 times w/o carry
	a <*> -1
	a <*> -1		/now a = 2 * high nibble of to

	bc => sp		/save count

	bc = IMAGE1		/task ones map
	a + c -> c		/bc points to map reg for source

	call _di
	a = *bc
	a -> *IMAGE0[2] -> *MAP0[2]	/now task zero can see user space
	a = *(bc +1 +1)
	a -> *IMAGE0[4] -> *MAP0[4]

	bc <= sp		/restore count to bc

	a = d & 0x0F		/remove high nibble from to
	a | 0x10 -> d		/and replace it with window nibble

	a = b | c		/test for zero count
	jz .5

	LDIR			/z80 do *de++ = *hl++ while --bc != 0

.5:
	af <= sp		/restore windows
	a -> *IMAGE0[4] -> *MAP0[4]
	af <= sp
	a -> *IMAGE0[2] -> *MAP0[2]

	de <= sp		/recover C frame pointer
	jmp _ei

/ memrw(user, space, direction, count)
/ Copy count bytes in the specified direction between
/ user space and outer space (an absolute long address).
/ Neither source nor destination should cross a 4K boudary.

_memrw:
	de => sp			/save C frame pointer
	a = *IMAGE0[2]; af => sp	/save images so interrupt code
	a = *IMAGE0[4]; af => sp	/ can use the windows

	hl = 17 + sp		/get arguments from stack
	b = *hl
	c = *(hl-1)		/bc = count
	bc => sp		/save it

	     (hl-1)
	c = *(hl-1)		/c = direction
	bc => sp		/save it

	d = *(hl-1)
	e = *(hl-1)		/de = low word of space address
	     (hl-1)		/skip high byte of long
	b = *(hl-1)		/bde = 24 bit space address

	a = *(hl-1)
	l = *(hl-1)
	h = a			/hl = user address

	call _di		/set up space window
	a = b & 0x0F -> b	/keep low nibble of b
	a = d & 0xF0 | b
	a <*> -1		/rotate a right 4 times w/o carry
	a <*> -1
	a <*> -1
	a <*> -1		/now a = high 8 bits of 20-bit space addr
	a -> *IMAGE0[4] -> *MAP0[4]	/task0s window to space
	a = d & 0x0F		/remove high nibble from space addr
	a | 0x20 -> d		/replace it with window nibble

				/set up user window
	a = h & 0xF0		/a = high nibble of user addr (times 16)
	a <*> -1		/rotate a right 3 times w/o carry
	a <*> -1
	a <*> -1		/now a = 2 * high nibble of user addr
	bc = IMAGE1		/task ones map
	a + c -> c		/bc points to map reg for user addr
	a = *bc
	a -> *IMAGE0[2] -> *MAP0[2]	/task0s window to user
	a = h & 0x0F		/remove high nibble from user addr
	a | 0x10 -> h		/replace it with window nibble

	bc <= sp		/get direction
				/now hl -> user, de -> space
				/want hl -> source, de -> destination
	a = c | c		/non-0 means WRITE from user to space
	jnz .6			/ and we are set up for a write
	hl <> de

.6:				/now hl -> source, de -> destination
	bc <= sp		/get count
	a = b | c		/test for zero count
	jz .7

	call _ei
	LDIR			/z80 do *de++ = *hl++ while --bc != 0
	call _di

.7:
	af <= sp		/restore windows
	a -> *IMAGE0[4] -> *MAP0[4]
	af <= sp
	a -> *IMAGE0[2] -> *MAP0[2]

	de <= sp		/recover C frame pointer
	jmp _ei

/segcopy(from, to) -- Copy one 4K segment to another

_segcopy:

	sp <= de		/save C frame pointer
	hl = 4 + sp		/point to arguments
	a = *IMAGE0[2]		/save current map
	sp <= af
	a = *IMAGE0[4]
	sp <= af

	call _di
	a = *hl			/from segment
	a -> *IMAGE0[2]
	a -> *MAP0[2]		/task 0 can see source segment

	a = *(hl +1 +1)		/to segment
	a -> *IMAGE0[4]
	a -> *MAP0[4]		/task 0 can see destination segment
	call _ei

	hl = 0x1000		/hl looks thru MAP[2]
	de = 0x2000		/de looks thru MAP[4]
	bc = 4096		/segment size

	LDIR			/z80 do *de++ = *hl++ while --bc != 0

	call _di
	sp => af		/restore task 0s map
	a -> *MAP0[4]
	a -> *IMAGE0[4]
	sp => af
	a -> *MAP0[2]
	a -> *IMAGE0[2]

	sp => de	/restore C frame pointer
	jmp _ei

