
/Phototypesetter interpreter

	mov	(sp)+,argc
	dec	argc
	beq	loop
	tst	(sp)+
	mov	(sp),r1
	cmpb	(r1)+,$'-
	bne	1f
	jsr	pc,atoi
	mov	r0,0f
	dec	argc
	beq	loop0
	tst	(sp)+
1:
	mov	(sp),r0
	jsr	r5,fopen; ibuf
	bec	loop0
ex:
	jsr	r5,str;mlt
	jsr	r5,numb;leadtot
	jsr	r5,str;nl
	clr	r0
	sys	exit
mlt:<Lead total \0>
.even

loop0:
	mov	ibuf,r0
	sys	seek; 0:..; 0
loop:
	jsr	r5,getc; ibuf
	bes	ex
	tstb	r0
	bpl	1f
	jsr	pc,prn
	com	r0
	bic	$!177,r0
	add	r0,esc
	jmp	loop
1:
	tst	esc
	beq	1f
	tst	escd
	bne	2f
	jsr	r5,str;mescf
	br	3f
2:
	jsr	r5,str;mescb
3:
	jsr	r5,numb;esc
	jsr	r5,str;nl
	tst	escd
	beq	0f
	neg	esc
0:
	add	esc,esct
	clr	esc
1:
	jsr	pc,prn
	cmpb	r0,$100
	bne	1f
	jsr	r5,str;minit
	jmp	loop
1:
	cmpb	r0,$101
	bne	1f
	jsr	r5,str;mlr
	jmp	loop
1:
	cmpb	r0,$102
	bne	1f
	jsr	r5,str;mur
	jmp	loop
1:
	cmpb	r0,$103
	bne	1f
	jsr	r5,str;mum
	jmp	loop
1:
	cmpb	r0,$104
	bne	1f
	jsr	r5,str;mlm
	jmp	loop
1:
	cmpb	r0,$105
	bne	1f
	jsr	r5,str;mlc
	clr	case
	jmp	loop
1:
	cmpb	r0,$106
	bne	1f
	jsr	r5,str;muc
	mov	$100,case
	jmp	loop
1:
	cmpb	r0,$107
	bne	1f
	jsr	r5,str;mef
/	jsr	r5,str;mesct
	jsr	r5,numb;esct
	jsr	r5,str;nl
	clr	escd
	jmp	loop
1:
	cmpb	r0,$110
	bne	1f
	jsr	r5,str;meb
/	jsr	r5,str;mesct
	jsr	r5,numb;esct
	jsr	r5,str;nl
	inc	escd
	jmp	loop
1:
	cmpb	r0,$111
	bne	1f
	jsr	r5,str;mstop
	jmp	loop
1:
	cmpb	r0,$112
	bne	1f
	jsr	r5,str;mlf
	jsr	r5,numb;leadtot
	jsr	r5,str;nl
	clr	leadmode
	jmp	loop
1:
	cmpb	r0,$114
	bne	1f
	jsr	r5,str;mlb
	jsr	r5,numb;leadtot
	jsr	r5,str;nl
	inc	leadmode
	jmp	loop
1:
	cmpb	r0,$100
	bne	1f
	jsr	r5,str;minit
	jmp	loop
1:
	mov	r0,r1
	bic	$!360,r1
	cmp	r1,$100
	bne	1f
	jsr	r5,str;milgl
	jmp	loop
1:
	mov	r0,r1
	bic	$!340,r1
	cmpb	r1,$140
	bne	1f
	com	r0
	bic	$!37,r0
	mov	r0,n
	jsr	r5,str;mlead
	jsr	r5,numb;n
	jsr	r5,str;nl
	tst	leadmode
	beq	0f
	neg	n
0:
	add	n,leadtot
	jmp	loop
1:
	mov	r0,r1
	bic	$!360,r1
	cmpb	$120,r1
	bne	1f
	bic	$!17,r0
	mov	r0,n
	jsr	r5,str;msize
	mov	$stab+1,r0
2:
	cmpb	n,(r0)+
	beq	2f
	tstb	(r0)+
	bne	2b
	br	3f
2:
	tst	-(r0)
	movb	(r0),pts
	jsr	r5,numb;pts
3:
	jsr	r5,str;nl
	jmp	loop
1:
	bitb	$300,r0
	bne	1f
	bic	$!77,r0
	add	case,r0
	mov	$wtab+1,r1
2:
	cmpb	r0,(r1)+
	beq	3f
	inc	r1
	cmp	r1,$wtab+192.
	blo	2b
	br	1f
3:
	tst	-(r1)
	movb	(r1),r3
	mpy	pts,r3
	sxt	r2
	dvd	$6.,r2
	mov	r2,n
	sub	$wtab,r1
	asr	r1
	add	$040,r1
	clr	x
	movb	r1,x
	jsr	r5,str;x
/	jsr	r5,str;space
/	jsr	r5,numb;n
	jsr	r5,str;nl
	jmp	loop
1:
	jsr	r5,str;nl
	jmp	loop

prn:
	mov	$8.,base
	mov	r0,n
	jsr	r5,numb;n
	mov	$10.,base
	jsr	r5,str;space
	rts	pc

str:
	mov	r0,-(sp)
	mov	(r5)+,r1
	mov	r1,r2
	mov	r1,0f
1:
	tstb	(r1)+
	bne	1b
	sub	r2,r1
	dec	r1
	mov	r1,1f
	mov	$1,r0
	sys	write; 0:..; 1:..
	mov	(sp)+,r0
	rts	r5

numb:
	mov	r0,-(sp)
	mov	r2,-(sp)
	mov	r3,-(sp)
	mov	*(r5)+,r3
	jsr	pc,numb1
	mov	(sp)+,r3
	mov	(sp)+,r2
	mov	(sp)+,r0
	rts	r5
numb1:
	clr	r2
	dvd	base,r2
	mov	r3,-(sp)
	mov	r2,r3
	beq	1f
	jsr	pc,numb1
1:
	add	$'0,(sp)
	mov	(sp)+,char
	mov	$1,r0
	sys	write; char; 1
	rts	pc

atoi:
	clr	-(sp)
	mov	r3,-(sp)
	clr	r3
	clr	-(sp)
	movb	(r1)+,r0
	cmpb	r0,$'-
	bne	2f
	inc	(sp)
1:
	movb	(r1)+,r0
2:
	sub	$'0,r0
	cmp	r0,$7
	bhi	1f
	inc	4(sp)
	mpy	$8.,r3
	add	r0,r3
	br	1b
1:
	tst	(sp)+
	beq	1f
	neg	r3
1:
	mov	r3,r0
	mov	(sp)+,r3
	tst	(sp)+
	rts	pc

space:	< \0>
minit:	<Initialize\n\0>
mesct:	<Total \0>
mesc:	<Escape \0>
mescf:	<\> \0>
mescb:	<\< \0>
nl:	<\n\0>
mlr:	<Lower Rail\n\0>
mur:	<Upper Rail\n\0>
mum:	<Upper Mag\n\0>
mlm:	<Lower Mag\n\0>
muc:	<Upper Case\n\0>
mlc:	<Lower Case\n\0>
mef:	<\> mode, \0>
meb:	<\< mode, \0>
mstop:	<*****Stop*****\n\0>
mlf:	<Lead forward, \0>
mlb:	<Lead backward, \0>
mlead:	<Lead \0>
milgl:	<Illegal control\n\0>
msize:	<Size \0>
.even
pts:	12.
stab:	/type size table
.byte	6.,10
.byte	7.,00
.byte	8.,01
.byte	9.,07
.byte	10.,02
.byte	11.,03
.byte	12.,04
.byte	14.,05
.byte	16.,11
.byte	18.,06
.byte	20.,12
.byte	22.,13
.byte	24.,14
.byte	28.,15
.byte	36.,16
.byte	0,0

.bss
leadtot:	.=.+2
leadmode:	.=.+2
argc:	.=.+2
base:	.=.+2
x:	.=.+2
char:	.=.+2
n:	.=.+2
case:	.=.+2
esc:	.=.+2
esct:	.=.+2
escd:	.=.+2
ibuf:	.=.+518.

.data
wtab:

/Graphic Systems Font Table

/Commercial Layout
/Times Roman Regular

/Extended Code, ASCII order
/Width (6 pt.), Code

.byte 15.,0	/space
.byte 9.,145	/!
.byte 0,0	/"
.byte 27.,153	/#
.byte 18.,155	/$
.byte 27.,53	/%
.byte 28.,50	/&
.byte 9.,150	/'	*
.byte 9.,132	/(
.byte 9.,133	/)
.byte 16.,122	/*
.byte 27.,143	/+
.byte 9.,47	/,
.byte 27.,123	/-
.byte 9.,44	/.
.byte 12.,43	//
.byte 18.,110	/0
.byte 18.,111	/1
.byte 18.,112	/2
.byte 18.,113	/3
.byte 18.,114	/4
.byte 18.,115	/5
.byte 18.,116	/6
.byte 18.,117	/7
.byte 18.,120	/8
.byte 18.,121	/9
.byte 9.,142	/:
.byte 9.,23	/;
.byte 0,0	/<
.byte 27.,140	/=
.byte 0,0	/>
.byte 16.,147	/?
.byte 36.,131	/@
.byte 29.,103	/A
.byte 23.,75	/B
.byte 26.,70	/C
.byte 29.,74	/D
.byte 25.,72	/E
.byte 24.,101	/F
.byte 30.,65	/G
.byte 29.,60	/H
.byte 13.,66	/I
.byte 16.,105	/J
.byte 29.,107	/K
.byte 24.,63	/L
.byte 35.,62	/M
.byte 30.,61	/N
.byte 27.,57	/O
.byte 22.,67	/P
.byte 27.,55	/Q
.byte 28.,64	/R
.byte 18.,76	/S
.byte 24.,56	/T
.byte 29.,106	/U
.byte 28.,71	/V
.byte 36.,104	/W
.byte 28.,102	/X
.byte 28.,77	/Y
.byte 24.,73	/Z
.byte 9.,134	/[
.byte 0,0	/\
.byte 9.,135	/]
.byte 0,0	/^
.byte 0,0	/_
.byte 0,0	/`
.byte 17.,25	/a
.byte 19.,12	/b
.byte 16.,27	/c
.byte 20.,11	/d
.byte 17.,31	/e
.byte 13.,14	/f
.byte 17.,45	/g
.byte 21.,01	/h
.byte 10.,06	/i
.byte 10.,15	/j
.byte 20.,17	/k
.byte 10.,05	/l
.byte 32.,04	/m
.byte 21.,03	/n
.byte 19.,33	/o
.byte 20.,21	/p
.byte 19.,42	/q
.byte 14.,35	/r
.byte 15.,10	/s
.byte 12.,02	/t
.byte 20.,16	/u
.byte 20.,37	/v
.byte 27.,41	/w
.byte 20.,13	/x
.byte 19.,51	/y
.byte 16.,07	/z
.byte 0,0	/{
.byte 5.,151	/|
.byte 0,0	/}
/.byte 11.,40	/~
.byte 0,0	/~
.byte 0,0	/del
