;
; multiply hl by a.    18 bytes
;
; 41 setup
; 57 for each set bit
; 42 for each clear bit
; 83 - 497 clocks - (21 - 130 mikes at 4mhz)

imul168:
	push	de			; 11
	ld		de,0		; 10	- zero accumulator	
	jr		test		; 12	- jump into loop
top:
	rra					; 4		- shift into carry
	jr		nc,shift	; 7/12	- if one's bit not set, no add
	add		hl,de		; 11	- multiply step
shift:
	ex		de,hl		; 4		- de now accu, hl now addend
	add		hl,hl		; 11	- double addend
test:
	ex		de,hl		; 4		- hl now accumulator
	or		a			; 4		- check if done, zero carry
	jr		nz,top		; 7/12	- more adding
done:
	pop		de			; 10
	ret					; 10

; vim: tabstop=4 shiftwidth=4 noexpandtab:
