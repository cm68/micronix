.:=.data
_nlset:
&0
_nolook:
&0
_nstate:
&0
_memp:
&_amem
_lineno:
&01
_fatfl:
&01
_nerrors:
&0
.:=.text
_main:
call c.ent
hl=&06+de
bc=^hl=>sp
hl=&04+de
bc=^hl=>sp
call _setup
af<=sp<=sp
hl=_ntokens
hl+(bc=&020)
sp<=hl
hl=&020=>sp
call c.idiv
hl<=sp
hl->_tbitset
call _cpres
call _cempty
call _cpfir
call _stagen
call _output
call _go2out
call _hidepro
call _summary
call _callopt
call _others
hl=&0=>sp
call _exit
af<=sp
jmp c.ret
L531:
0171,0141,0143,0143,056,0141,0143,0164
0163,0
L121:
0143,0141,0156,0156,0157,0164,040,0162
0145,0157,0160,0145,0156,040,0141,0143
0164,0151,0157,0156,040,0164,0145,0155
0160,0146,0151,0154,0145,0
L511:
0162,0
L311:
0171,0141,0143,0143,056,0141,0143,0164
0163,0
L17:
0171,0171,0144,0145,0146,0
L76:
0171,0171,0143,0150,0153,0
L52:
0171,0171,0162,062,0
L31:
0171,0171,0162,061,0
L11:
056,057,0171,0141,0143,0143,0160,0141
0162,0
L7:
0143,0141,0156,0156,0157,0164,040,0146
0151,0156,0144,040,0160,0141,0162,0163
0145,0162,040,045,0163,0
L3:
0162,0
L1:
056,057,0171,0141,0143,0143,0160,0141
0162,0
_others:
call c.ents
hl=&L3
sp<=hl
hl=&L1
sp<=hl
call _fopen
af<=sp<=sp
hl=bc
hl->_finput
hl=&_finput
a=*hl|*(hl+1)
jnz L5
hl=&L11
sp<=hl
hl=&L7
sp<=hl
call _error
af<=sp<=sp
L5: /69
hl=_nprod=>sp
hl=&_levprd
sp<=hl
hl=&L31
sp<=hl
call _warray
af<=sp<=sp<=sp
hl=&0=>sp
hl=_nprod=>sp
hl=&_temp1
sp<=hl
call _aryfil
af<=sp<=sp<=sp
a=0401->c.r2+a-^a->c.r2[01]
L51: / 72
hl=&_nprod
a=c.r2-*hl=c.r2[01]-^*(hl+1)
jp L71
hl=c.r2
hl+hl
bc=&_temp1
hl+bc
sp<=hl
hl=c.r2+(bc=&01)
hl+hl
bc=&_prdptr
hl+bc
hl=a^hl
sp<=hl
hl=c.r2
hl+hl
bc=&_prdptr
hl+bc
bc<=sp
a=c-*hl->c=b-^*(hl+1)->b
sp<=bc
hl=&02=>sp
call c.udiv
hl<=sp
hl+(bc=&0177776)
bc<=sp
a=l->*bc=h->*(bc+1)
hl=c.r2
hl+1
hl->c.r2
jmp L51
L71: /72
hl=_nprod=>sp
hl=&_temp1
sp<=hl
hl=&L52
sp<=hl
call _warray
af<=sp<=sp<=sp
hl=&0176030=>sp
hl=_nstate=>sp
hl=&_temp1
sp<=hl
call _aryfil
af<=sp<=sp<=sp
a=0401->c.r2+a-^a->c.r2[01]
L72: / 76
hl=&c.r2
a=_ntokens-*hl=_ntokens[01]-^*(hl+1)
jm L13
hl=c.r2
hl+hl
bc=&_tstates
hl+bc
hl=a^hl
hl->c.r3
L73: / 77
hl=&c.r3
a=*hl|*(hl+1)
jz L33
hl=c.r3
hl+hl
bc=&_temp1
hl+bc
sp<=hl
hl=c.r2
hl+hl+hl
bc=&_tokset[02]
hl+bc
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
hl=c.r3
hl+hl
bc=&_mstates
hl+bc
hl=a^hl
hl->c.r3
jmp L73
L13: /80
a-a->c.r2->c.r2[01]
L74: / 81
hl=&c.r2
a=_nnonter-*hl=_nnonter[01]-^*(hl+1)
jm L15
hl=c.r2
hl+hl
bc=&_ntstate
hl+bc
hl=a^hl
hl->c.r3
L75: / 82
hl=&c.r3
a=*hl|*(hl+1)
jz L35
hl=c.r3
hl+hl
bc=&_temp1
hl+bc
sp<=hl
hl=c.r2
a=0-l->l=0-^h->h
bc<=sp
a=l->*bc=h->*(bc+1)
hl=c.r3
hl+hl
bc=&_mstates
hl+bc
hl=a^hl
hl->c.r3
jmp L75
L33: /76
hl=c.r2
hl+1
hl->c.r2
jmp L72
L15: /85
hl=_nstate=>sp
hl=&_temp1
sp<=hl
hl=&L76
sp<=hl
call _warray
af<=sp<=sp<=sp
hl=_nstate=>sp
hl=&_defact
sp<=hl
hl=&L17
sp<=hl
call _warray
af<=sp<=sp<=sp
L37: / 92
hl=_finput
hl+1+1
a=*hl-0401->*hl=*(hl+1)-^0->*hl
hl-1
jm L2
hl=_finput
bc=^hl=>sp
hl=_finput
a=*hl+0401->*hl=*(hl+1)+^0->*hl
hl<=sp
a=*hl->c+a-^a->b
a=c&0377->c-a->b
jmp L4
L2:
hl=_finput=>sp
call __fillbu
af<=sp
L4:
hl=bc
hl->c.r4
a=c.r4::0377 jnz L6=c.r4[01]::0377
L6:
jz L57
a=c.r4::044 jnz L01=c.r4[01]::0
L01:
jnz L77
hl=_finput
hl+1+1
a=*hl-0401->*hl=*(hl+1)-^0->*hl
hl-1
jm L21
hl=_finput
bc=^hl=>sp
hl=_finput
a=*hl+0401->*hl=*(hl+1)+^0->*hl
hl<=sp
a=*hl->c+a-^a->b
a=c&0377->c-a->b
jmp L41
L21:
hl=_finput=>sp
call __fillbu
af<=sp
L41:
hl=bc
hl->c.r4
a=c.r4::0101 jnz L61=c.r4[01]::0
L61:
jz L101
hl=_ftable
hl+1+1
a=*hl-0401->*hl=*(hl+1)-^0->*hl
hl-1
jm L301
hl=_ftable
bc=^hl=>sp
hl=_ftable
a=*hl+0401->*hl=*(hl+1)+^0->*hl
hl<=sp
*hl=044
jmp L501
L35: /81
hl=c.r2
hl+1
hl->c.r2
jmp L74
L57: /106
hl=_ftable=>sp
call _fclose
af<=sp
jmp c.rets
L77: /105
hl=_ftable
hl+1+1
a=*hl-0401->*hl=*(hl+1)-^0->*hl
hl-1
jm L731
hl=_ftable
bc=^hl=>sp
hl=_ftable
a=*hl+0401->*hl=*(hl+1)+^0->*hl
hl<=sp
a=c.r4->*hl
jmp L141
L101: /96
hl=&L511
sp<=hl
hl=&L311
sp<=hl
call _fopen
af<=sp<=sp
hl=bc
hl->_faction
hl=&_faction
a=*hl|*(hl+1)
jnz L321
hl=&L121
sp<=hl
call _error
af<=sp
jmp L321
L301: /95
hl=_ftable=>sp
hl=&044=>sp
call __flushb
af<=sp<=sp
L501: / 95
jmp L77
L321: /99
hl=_faction
hl+1+1
a=*hl-0401->*hl=*(hl+1)-^0->*hl
hl-1
jm L02
hl=_faction
bc=^hl=>sp
hl=_faction
a=*hl+0401->*hl=*(hl+1)+^0->*hl
hl<=sp
a=*hl->c+a-^a->b
a=c&0377->c-a->b
jmp L22
L02:
hl=_faction=>sp
call __fillbu
af<=sp
L22:
hl=bc
hl->c.r4
a=c.r4::0377 jnz L42=c.r4[01]::0377
L42:
jz L521
hl=_ftable
hl+1+1
a=*hl-0401->*hl=*(hl+1)-^0->*hl
hl-1
jm L721
hl=_ftable
bc=^hl=>sp
hl=_ftable
a=*hl+0401->*hl=*(hl+1)+^0->*hl
hl<=sp
a=c.r4->*hl
jmp L131
L521: /99
hl=_faction=>sp
call _fclose
af<=sp
hl=&L531
sp<=hl
call _unlink
af<=sp
hl=_finput
hl+1+1
a=*hl-0401->*hl=*(hl+1)-^0->*hl
hl-1
jm L03
hl=_finput
bc=^hl=>sp
hl=_finput
a=*hl+0401->*hl=*(hl+1)+^0->*hl
hl<=sp
a=*hl->c+a-^a->b
a=c&0377->c-a->b
jmp L23
L03:
hl=_finput=>sp
call __fillbu
af<=sp
L23:
hl=bc
hl->c.r4
jmp L77
L721: /99
hl=_ftable=>sp
hl=c.r4
a=l&0377->l-a->h
sp<=hl
call __flushb
af<=sp<=sp
L131: / 99
a=c.r4::012 jnz L62=c.r4[01]::0
L62:
jnz L321
hl=_ftable
hl+(bc=&06)
hl=a^hl
a=l&0100->l=h&0401->h
a=l|h
jz L321
hl=_ftable=>sp
hl=&0177777=>sp
call __flushb
af<=sp<=sp
jmp L321
L731: /105
hl=_ftable=>sp
hl=c.r4
a=l&0377->l-a->h
sp<=hl
call __flushb
af<=sp<=sp
L141: / 105
a=c.r4::012 jnz L43=c.r4[01]::0
L43:
jnz L37
hl=_ftable
hl+(bc=&06)
hl=a^hl
a=l&0100->l=h&0401->h
a=l|h
jz L37
hl=_ftable=>sp
hl=&0177777=>sp
call __flushb
af<=sp<=sp
jmp L37
_chcopy:
call c.ent
L541: / 113
hl=&04+de
hl=a^hl
sp<=hl
hl=&06+de
bc=^hl=>sp
hl=&06+de
a=*hl+0401->*hl=*(hl+1)+^0->*hl
hl<=sp
bc<=sp
a=*hl->*bc
a=*bc|a
jz L741
hl=&04+de
a=*hl+0401->*hl=*(hl+1)+^0->*hl
jmp L541
L741: /113
hl=&04+de
hl=a^hl
bc=hl
jmp c.ret
.:=.data
L151:
.:=.[0620]
.:=.text
L302:
045,0144,051,0
L102:
040,040,040,040,050,0
L571:
0151,0164,0145,0155,040,0164,0157,0157
040,0142,0151,0147,0
L361:
040,072,040,0
_writem:
call c.ent
hl=0177764+sp->sp
hl=&0177766+de
sp<=hl
hl=&04+de
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
L351: / 123
hl=&0177766+de
hl=a^hl
a-a-*hl=0-^*(hl+1)
jp L551
L751: / 123
hl=&0177766+de
a=*hl+02->*hl=*(hl+1)+^0->*hl
jmp L351
L551: /123
hl=&0177766+de
sp<=hl
hl=&0177766+de
hl=a^hl
hl=a^hl
a=0-l->l=0-^h->h
hl+hl
bc=&_prdptr
hl+bc
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
hl=&0177764+de
sp<=hl
hl=&0177766+de
hl=a^hl
hl=a^hl
hl+hl+hl
bc=&_nontrst[0140000]
hl+bc
bc=^hl=>sp
hl=&L151
sp<=hl
call _chcopy
af<=sp<=sp
hl<=sp
a=c->*hl=b->*(hl+1)
hl=&0177764+de
sp<=hl
hl=&L361
sp<=hl
hl=&0177764+de
bc=^hl=>sp
call _chcopy
af<=sp<=sp
hl<=sp
a=c->*hl=b->*(hl+1)
L561: / 128
hl=&0177764+de
bc=^hl=>sp
hl=&0177764+de
a=*hl+0401->*hl=*(hl+1)+^0->*hl
hl<=sp
sp<=hl
hl=&0177766+de
a=*hl+02->*hl=*(hl+1)+^0->*hl
hl-1
sp<=hl
hl=&04+de
bc<=sp
a=*bc::*hl jnz L04=*(bc+1)::*(hl+1)
L04:
jnz L63
bc=0137
jmp L24
L63:
bc=040
L24:
hl<=sp
a=c->*hl
hl=&0177764+de
hl=a^hl
*hl=0
hl=&0177770+de
sp<=hl
hl=&0177766+de
hl=a^hl
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
bc-1
hl=bc
a-a-*hl=0-^*(hl+1)
jm L171
hl=&0177770+de
sp<=hl
hl=&04+de
hl=a^hl
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
bc-1
a=*(bc+1)|a
jp L771
hl=&0177764+de
sp<=hl
hl=&L102
sp<=hl
hl=&0177764+de
bc=^hl=>sp
call _chcopy
af<=sp<=sp
hl<=sp
a=c->*hl=b->*(hl+1)
hl=&0177770+de
hl=a^hl
a=0-l->l=0-^h->h
sp<=hl
hl=&L302
sp<=hl
hl=&0177764+de
bc=^hl=>sp
call _sprintf
af<=sp<=sp<=sp
jmp L771
L171: /132
hl=&0177764+de
sp<=hl
hl=&0177770+de
bc=^hl=>sp
call _symnam
af<=sp
sp<=bc
hl=&0177764+de
bc=^hl=>sp
call _chcopy
af<=sp<=sp
hl<=sp
a=c->*hl=b->*(hl+1)
hl=&0177764+de
bc=&L151[0562]
a=c-*hl=b-^*(hl+1)
jnc L561
hl=&L571
sp<=hl
call _error
af<=sp
jmp L561
L771: /141
hl=&L151
bc=hl
jmp c.ret
.:=.data
_zzcwp:
&_wsets
_zzgoent:
&0
_zzgobes:
&0
_zzacent:
&0
_zzexcp:
&0
_zzclose:
&0
_zzsrcon:
&0
_zzmemsz:
&_mem0
_zzrrcon:
&0
.:=.text
_symnam:
call c.ent
af=>sp=>sp=>sp=>sp
hl=&0177770+de
sp<=hl
hl=&04+de
a=*hl-0=*(hl+1)-^020
jm L44
hl=&04+de
hl=a^hl
hl+hl+hl
bc=&_nontrst[0140000]
hl+bc
bc=^hl
jmp L64
L44:
hl=&04+de
hl=a^hl
hl+hl+hl
bc=&_tokset
hl+bc
bc=^hl
L64:
hl<=sp
a=c->*hl=b->*(hl+1)
hl=&0177770+de
hl=a^hl
a=*hl::040
jnz L502
hl=&0177770+de
a=*hl+0401->*hl=*(hl+1)+^0->*hl
L502: /149
hl=&0177770+de
hl=a^hl
bc=hl
jmp c.ret
L752:
012,0
L552:
045,0144,040,0162,0145,0144,0165,0143
0145,057,0162,0145,0144,0165,0143,0145
0
L152:
054,040,0
L542:
045,0144,040,0163,0150,0151,0146,0164
057,0162,0145,0144,0165,0143,0145,0
L142:
012,0143,0157,0156,0146,0154,0151,0143
0164,0163,072,040,0
L332:
045,0144,040,0145,0156,0164,0162,0151
0145,0163,040,0163,0141,0166,0145,0144
040,0142,0171,040,0147,0157,0164,0157
040,0144,0145,0146,0141,0165,0154,0164
012,0
L132:
045,0144,040,0147,0157,0164,0157,040
0145,0156,0164,0162,0151,0145,0163,012
0
L722:
045,0144,040,0163,0150,0151,0146,0164
040,0145,0156,0164,0162,0151,0145,0163
054,040,045,0144,040,0145,0170,0143
0145,0160,0164,0151,0157,0156,0163,012
0
L522:
045,0144,040,0145,0170,0164,0162,0141
040,0143,0154,0157,0163,0165,0162,0145
0163,012,0
L322:
045,0144,057,045,0144,040,0144,0151
0163,0164,0151,0156,0143,0164,040,0154
0157,0157,0153,0141,0150,0145,0141,0144
040,0163,0145,0164,0163,012,0
L122:
0155,0145,0155,0157,0162,0171,072,040
0163,0164,0141,0164,0145,0163,054,0145
0164,0143,056,040,045,0144,057,045
0144,054,040,0160,0141,0162,0163,0145
0162,040,045,0144,057,045,0144,012
0
L712:
045,0144,057,045,0144,040,0167,0157
0162,0153,0151,0156,0147,040,0163,0145
0164,0163,040,0165,0163,0145,0144,012
0
L512:
045,0144,040,0163,0150,0151,0146,0164
057,0162,0145,0144,0165,0143,0145,054
040,045,0144,040,0162,0145,0144,0165
0143,0145,057,0162,0145,0144,0165,0143
0145,040,0143,0157,0156,0146,0154,0151
0143,0164,0163,040,0162,0145,0160,0157
0162,0164,0145,0144,012,0
L312:
045,0144,057,045,0144,040,0147,0162
0141,0155,0155,0141,0162,040,0162,0165
0154,0145,0163,054,040,045,0144,057
045,0144,040,0163,0164,0141,0164,0145
0163,012,0
L112:
012,045,0144,057,045,0144,040,0164
0145,0162,0155,0151,0156,0141,0154,0163
054,040,045,0144,057,045,0144,040
0156,0157,0156,0164,0145,0162,0155,0151
0156,0141,0154,0163,012,0
_summary:
call c.ent
hl=&_foutput
a=*hl|*(hl+1)
jz L702
hl=&0310=>sp
hl=_nnonter=>sp
hl=&0177=>sp
hl=_ntokens=>sp
hl=&L112
sp<=hl
hl=_foutput=>sp
call _fprintf
hl=014+sp->sp
hl=&01130=>sp
hl=_nstate=>sp
hl=&0620=>sp
hl=_nprod=>sp
hl=&L312
sp<=hl
hl=_foutput=>sp
call _fprintf
hl=014+sp->sp
hl=_zzrrcon=>sp
hl=_zzsrcon=>sp
hl=&L512
sp<=hl
hl=_foutput=>sp
call _fprintf
af<=sp<=sp<=sp<=sp
hl=&0372=>sp
hl=_zzcwp
bc=&_wsets
a=l-c->l=h-^b->h
sp<=hl
hl=&024=>sp
call c.udiv
hl=&L712
sp<=hl
hl=_foutput=>sp
call _fprintf
af<=sp<=sp<=sp<=sp
hl=&07640=>sp
hl=_memp
bc=&_amem
a=l-c->l=h-^b->h
sp<=hl
hl=&02=>sp
call c.udiv
hl=&012120=>sp
hl=_zzmemsz
bc=&_mem0
a=l-c->l=h-^b->h
sp<=hl
hl=&02=>sp
call c.udiv
hl=&L122
sp<=hl
hl=_foutput=>sp
call _fprintf
hl=014+sp->sp
hl=&0702=>sp
hl=_nlset=>sp
hl=&L322
sp<=hl
hl=_foutput=>sp
call _fprintf
af<=sp<=sp<=sp<=sp
hl=_zzclose
sp<=hl
hl=_nstate
hl+hl
bc<=sp
a=c-l->c=b-^h->b
sp<=bc
hl=&L522
sp<=hl
hl=_foutput=>sp
call _fprintf
af<=sp<=sp<=sp
hl=_zzexcp=>sp
hl=_zzacent=>sp
hl=&L722
sp<=hl
hl=_foutput=>sp
call _fprintf
af<=sp<=sp<=sp<=sp
hl=_zzgoent=>sp
hl=&L132
sp<=hl
hl=_foutput=>sp
call _fprintf
af<=sp<=sp<=sp
hl=_zzgobes=>sp
hl=&L332
sp<=hl
hl=_foutput=>sp
call _fprintf
af<=sp<=sp<=sp
L702: /178
hl=&_zzsrcon
a=*hl|*(hl+1)
jnz L732
hl=&_zzrrcon
a=*hl|*(hl+1)
jz L532
L732: /178
hl=&L142
sp<=hl
hl=&__iob[020]
sp<=hl
call _fprintf
af<=sp<=sp
hl=&_zzsrcon
a=*hl|*(hl+1)
jz L342
hl=_zzsrcon=>sp
hl=&L542
sp<=hl
hl=&__iob[020]
sp<=hl
call _fprintf
af<=sp<=sp<=sp
L342: /181
hl=&_zzsrcon
a=*hl|*(hl+1)
jz L742
hl=&_zzrrcon
a=*hl|*(hl+1)
jz L742
hl=&L152
sp<=hl
hl=&__iob[020]
sp<=hl
call _fprintf
af<=sp<=sp
L742: /182
hl=&_zzrrcon
a=*hl|*(hl+1)
jz L352
hl=_zzrrcon=>sp
hl=&L552
sp<=hl
hl=&__iob[020]
sp<=hl
call _fprintf
af<=sp<=sp<=sp
L352: /183
hl=&L752
sp<=hl
hl=&__iob[020]
sp<=hl
call _fprintf
af<=sp<=sp
L532: / 186
hl=_ftemp=>sp
call _fclose
af<=sp
hl=&_fdefine
a=*hl|*(hl+1)
jz L162
hl=_fdefine=>sp
call _fclose
af<=sp
L162: /188
jmp c.ret
L562:
054,040,0154,0151,0156,0145,040,045
0144,012,0
L362:
012,040,0146,0141,0164,0141,0154,040
0145,0162,0162,0157,0162,072,040,0
_error:
call c.ent
hl=_nerrors
hl+1
hl->_nerrors
hl=&L362
sp<=hl
hl=&__iob[040]
sp<=hl
call _fprintf
af<=sp<=sp
hl=&06+de
bc=^hl=>sp
hl=&04+de
bc=^hl=>sp
hl=&__iob[040]
sp<=hl
call _fprintf
af<=sp<=sp<=sp
hl=_lineno=>sp
hl=&L562
sp<=hl
hl=&__iob[040]
sp<=hl
call _fprintf
af<=sp<=sp<=sp
hl=&_fatfl
a=*hl|*(hl+1)
jnz L762
jmp c.ret
L762: /198
call _summary
hl=&01=>sp
call _exit
af<=sp
jmp c.ret
_aryfil:
call c.ent
af=>sp=>sp=>sp=>sp
hl=&0177770+de
a-a->*hl->*(hl+1)
L172: / 204
hl=&0177770+de
sp<=hl
hl=&06+de
bc<=sp
a=*bc-*hl=*(bc+1)-^*(hl+1)
jp L372
hl=&0177770+de
hl=a^hl
hl+hl
sp<=hl
hl=&04+de
hl=a^hl
hl<>*sp;bc<=sp
hl+bc
sp<=hl
hl=&010+de
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
hl=&0177770+de
a=*hl+0401->*hl=*(hl+1)+^0->*hl
jmp L172
L372: /204
jmp c.ret
_setunio:
call c.ents
af=>sp=>sp
hl=&04+de
hl=a^hl
hl->c.r4
hl=&06+de
hl=a^hl
hl->c.r2
hl=&0177766+de
a-a->*hl->*(hl+1)
a-a->c.r3->c.r3[01]
L103: / 213
hl=&_tbitset
a=c.r3-*hl=c.r3[01]-^*(hl+1)
jp L303
hl=c.r4
sp<=hl
hl=&0177770+de
sp<=hl
hl=c.r4
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
bc-1
hl=bc=a^hl
sp<=hl
hl=c.r2=>sp
hl=c.r2
hl+1+1
hl->c.r2
hl<=sp
bc<=sp
a=c|*hl->c=b|*(hl+1)->b
hl<=sp
a=c->*hl=b->*(hl+1)
hl=c.r4=>sp
hl=c.r4
hl+1+1
hl->c.r4
hl=&0177770+de
bc<=sp
a=*bc::*hl jnz L05=*(bc+1)::*(hl+1)
L05:
jz L503
hl=&0177766+de
a=0401->*hl+a-^a->*(hl+1)
jmp L503
L303: /216
hl=&0177766+de
hl=a^hl
bc=hl
jmp c.rets
L503: /213
hl=c.r3
hl+1
hl->c.r3
jmp L103
L733:
0175,0
L533:
045,0163,040,0
L123:
040,0173,040,0
L513:
011,0116,0125,0114,0114,0
_prlook:
call c.ents
hl=&04+de
hl=a^hl
hl->c.r2
hl=&c.r2
a=*hl|*(hl+1)
jnz L313
hl=&L513
sp<=hl
hl=_foutput=>sp
call _fprintf
af<=sp<=sp
jmp L713
L313: /224
hl=&L123
sp<=hl
hl=_foutput=>sp
call _fprintf
af<=sp<=sp
a=0401->c.r4+a-^a->c.r4[01]
L323: / 226
hl=&c.r4
a=_ntokens-*hl=_ntokens[01]-^*(hl+1)
jm L523
hl=c.r4=>sp
hl=&04=>sp
call c.irsh
hl<=sp
hl+hl
sp<=hl
hl=c.r2
hl<>*sp;bc<=sp
hl+bc
hl=a^hl
sp<=hl
hl=&01=>sp
hl=c.r4
a=l&017->l-a->h
sp<=hl
call c.ilsh
hl<=sp
bc<=sp
a=c&l->c=b&h->b
a=c|b
jz L723
hl=c.r4=>sp
call _symnam
af<=sp
sp<=bc
hl=&L533
sp<=hl
hl=_foutput=>sp
call _fprintf
af<=sp<=sp<=sp
jmp L723
L523: /228
hl=&L733
sp<=hl
hl=_foutput=>sp
call _fprintf
af<=sp<=sp
L713: / 230
jmp c.rets
L723: /226
hl=c.r4
hl+1
hl->c.r4
jmp L323
.:=.data
L143:
.:=.[01440]
.:=.text
L573:
0151,0156,0164,0145,0162,0156,0141,0154
040,0131,0141,0143,0143,040,0145,0162
0162,0157,0162,072,040,0160,0171,0151
0145,0154,0144,040,045,0144,0
L763:
0156,0157,0156,0164,0145,0162,0155,0151
0156,0141,0154,040,045,0163,040,0156
0157,0164,040,0144,0145,0146,0151,0156
0145,0144,041,0
.:=.data
_indebug:
&0
.:=.text
_cpres:
call c.ents
af=>sp
hl=&L143
hl->c.r4
hl=&0177770+de
a-a->*hl->*(hl+1)
L343: / 242
hl=&0177770+de
a=_nnonter-*hl=_nnonter[01]-^*(hl+1)
jm L543
hl=&0177770+de
hl=a^hl
hl+(bc=&010000)
hl->c.r2
hl=&0177770+de
hl=a^hl
hl+hl
bc=&_pres
hl+bc
a=c.r4->*hl=c.r4[01]->*(hl+1)
a-a->_fatfl->_fatfl[01]
a-a->c.r3->c.r3[01]
L353: / 246
hl=&_nprod
a=c.r3-*hl=c.r3[01]-^*(hl+1)
jp L553
hl=c.r3
hl+hl
bc=&_prdptr
hl+bc
hl=a^hl
sp<=hl
hl=&c.r2
bc<=sp
a=*bc::*hl jnz L25=*(bc+1)::*(hl+1)
L25:
jnz L753
hl=c.r4=>sp
hl=c.r4
hl+1+1
hl->c.r4
hl<=sp
sp<=hl
hl=c.r3
hl+hl
bc=&_prdptr
hl+bc
hl=a^hl
hl+1+1
bc<=sp
a=l->*bc=h->*(bc+1)
jmp L753
L543: /252
hl=&0177770+de
hl=a^hl
hl+hl
bc=&_pres
hl+bc
a=c.r4->*hl=c.r4[01]->*(hl+1)
a=0401->_fatfl+a-^a->_fatfl[01]
hl=&_nerrors
a=*hl|*(hl+1)
jz L173
call _summary
hl=&01=>sp
call _exit
af<=sp
jmp L173
L743: /242
hl=&0177770+de
a=*hl+0401->*hl=*(hl+1)+^0->*hl
jmp L343
L553: /248
hl=&0177770+de
hl=a^hl
hl+hl
bc=&_pres
hl+bc
sp<=hl
hl=&c.r4
bc<=sp
a=*bc::*hl jnz L45=*(bc+1)::*(hl+1)
L45:
jnz L743
hl=&0177770+de
hl=a^hl
hl+hl+hl
bc=&_nontrst
hl+bc
bc=^hl=>sp
hl=&L763
sp<=hl
call _error
af<=sp<=sp
jmp L743
L753: /246
hl=c.r3
hl+1
hl->c.r3
jmp L353
L173: /259
hl=_nprod
hl+hl
bc=&L143
hl+bc
sp<=hl
hl=&c.r4
bc<=sp
a=c::*hl jnz L65=b::*(hl+1)
L65:
jz L373
hl=c.r4
sp<=hl
hl=_nprod
hl+hl
bc=&L143
hl+bc
bc<=sp
a=c-l->c=b-^h->b
sp<=bc
hl=&02=>sp
call c.udiv
hl=&L573
sp<=hl
call _error
af<=sp<=sp
L373: /260
jmp c.rets
L125:
040,045,0144,012,0
L715:
012,045,0163,072,040,0
_cpfir:
call c.ents
af=>sp=>sp=>sp
hl=_nnonter=>sp
hl=&024=>sp
call c.imul
hl<=sp
bc=&_wsets
hl+bc
hl->_zzcwp
a-a->c.r3->c.r3[01]
L773: / 268
hl=&c.r3
a=_nnonter-*hl=_nnonter[01]-^*(hl+1)
jm L104
hl=&0=>sp
hl=_tbitset=>sp
hl=c.r3=>sp
hl=&024=>sp
call c.imul
hl<=sp
bc=&_wsets[04]
hl+bc
sp<=hl
call _aryfil
af<=sp<=sp<=sp
hl=&0177770+de
sp<=hl
hl=c.r3+(bc=&01)
hl+hl
bc=&_pres
hl+bc
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
hl=c.r3
hl+hl
bc=&_pres
hl+bc
hl=a^hl
hl->c.r2
L704: / 271
hl=&0177770+de
a=c.r2-*hl=c.r2[01]-^*(hl+1)
jnc L304
hl=c.r2
hl=a^hl
hl->c.r4
L714: / 272
hl=&0177766+de
sp<=hl
hl=c.r4
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
bc-1
hl=bc
a-a-*hl=0-^*(hl+1)
jp L314
hl=&0177766+de
a=*hl-0=*(hl+1)-^020
jp L724
hl=c.r3=>sp
hl=&024=>sp
call c.imul
hl<=sp
bc=&_wsets[04]
hl+bc
sp<=hl
hl=&0177766+de
bc=^hl=>sp
hl=&04=>sp
call c.irsh
hl<=sp
hl+hl
hl<>*sp;bc<=sp
hl+bc
sp<=hl
hl=&01=>sp
hl=&0177766+de
hl=a^hl
a=l&017->l-a->h
sp<=hl
call c.ilsh
hl<=sp
bc<=sp
a=*bc|l->*bc=*(bc+1)|h->*bc
jmp L314
L104: /280
hl=&0177764+de
a=0401->*hl+a-^a->*(hl+1)
L534: / 285
hl=&0177764+de
a=*hl|*(hl+1)
jz L734
hl=&0177764+de
a-a->*hl->*(hl+1)
a-a->c.r3->c.r3[01]
L144: / 287
hl=&c.r3
a=_nnonter-*hl=_nnonter[01]-^*(hl+1)
jm L534
hl=&0177770+de
sp<=hl
hl=c.r3+(bc=&01)
hl+hl
bc=&_pres
hl+bc
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
hl=c.r3
hl+hl
bc=&_pres
hl+bc
hl=a^hl
hl->c.r2
L154: / 289
hl=&0177770+de
a=c.r2-*hl=c.r2[01]-^*(hl+1)
jnc L544
hl=c.r2
hl=a^hl
hl->c.r4
L164: / 290
hl=&0177766+de
sp<=hl
hl=c.r4
hl=a^hl
hl+(bc=&0170000)
bc<=sp
a=l->*bc=h->*(bc+1)
bc-1
a=*(bc+1)|a
jm L554
hl=&0177764+de
sp<=hl
hl=&0177766+de
bc=^hl=>sp
hl=&024=>sp
call c.imul
hl<=sp
bc=&_wsets[04]
hl+bc
sp<=hl
hl=c.r3=>sp
hl=&024=>sp
call c.imul
hl<=sp
bc=&_wsets[04]
hl+bc
sp<=hl
call _setunio
af<=sp<=sp
hl<=sp
a=*hl|c->*hl=*(hl+1)|b->*hl
hl=&0177766+de
hl=a^hl
hl+hl
bc=&_pempty
hl+bc
a=*hl|*(hl+1)
jnz L564
jmp L554
L304: /268
hl=c.r3
hl+1
hl->c.r3
jmp L773
L314: /271
hl=c.r2
hl+1+1
hl->c.r2
jmp L704
L324: /272
hl=c.r4
hl+1+1
hl->c.r4
jmp L714
L724: /277
hl=&0177766+de
hl=a^hl
hl+hl
bc=&_pempty[0160000]
hl+bc
a=*hl|*(hl+1)
jnz L324
jmp L314
L734: /296
a-a->c.r3->c.r3[01]
L374: / 298
hl=&c.r3
a=_nnonter-*hl=_nnonter[01]-^*(hl+1)
jm L574
hl=c.r3
hl+hl
bc=&_pfirst
hl+bc
sp<=hl
hl=c.r3=>sp
hl=&024=>sp
call c.imul
hl<=sp
bc=&_wsets[04]
hl+bc
sp<=hl
call _flset
af<=sp
hl<=sp
a=c->*hl=b->*(hl+1)
hl=c.r3
hl+1
hl->c.r3
jmp L374
L544: /287
hl=c.r3
hl+1
hl->c.r3
jmp L144
L554: /289
hl=c.r2
hl+1+1
hl->c.r2
jmp L154
L564: /290
hl=c.r4
hl+1+1
hl->c.r4
jmp L164
L574: /298
hl=&_indebug
a=*hl|*(hl+1)
jnz L305
jmp c.rets
L305: /300
hl=&_foutput
a=*hl|*(hl+1)
jz L505
a-a->c.r3->c.r3[01]
L705: / 301
hl=&c.r3
a=_nnonter-*hl=_nnonter[01]-^*(hl+1)
jm L505
hl=c.r3
hl+hl+hl
bc=&_nontrst
hl+bc
bc=^hl=>sp
hl=&L715
sp<=hl
hl=_foutput=>sp
call _fprintf
af<=sp<=sp<=sp
hl=c.r3
hl+hl
bc=&_pfirst
hl+bc
bc=^hl=>sp
call _prlook
af<=sp
hl=c.r3
hl+hl
bc=&_pempty
hl+bc
bc=^hl=>sp
hl=&L125
sp<=hl
hl=_foutput=>sp
call _fprintf
af<=sp<=sp<=sp
hl=c.r3
hl+1
hl->c.r3
jmp L705
L505: /307
jmp c.rets
L726:
0164,0157,0157,040,0155,0141,0156,0171
040,0163,0164,0141,0164,0145,0163,0
L326:
0171,0141,0143,0143,040,0163,0164,0141
0164,0145,057,0156,0157,0154,0157,0157
0153,040,0145,0162,0162,0157,0162,0
.:=.data
_pidebug:
&0
.:=.text
_state:
call c.ents
hl=0177754+sp->sp
hl=&0177764+de
sp<=hl
hl=_nstate
hl+hl
bc=&_pstate
hl+bc
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
hl=&0177762+de
sp<=hl
hl=_nstate
hl+hl
bc=&_pstate[02]
hl+bc
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
hl=&0177764+de
sp<=hl
hl=&0177762+de
bc<=sp
a=*bc::*hl jnz L06=*(bc+1)::*(hl+1)
L06:
jnz L325
bc=0
jmp c.rets
L325: /317
hl=&0177760+de
sp<=hl
hl=&0177762+de
hl=a^hl
hl+(bc=&0177774)
bc<=sp
a=l->*bc=h->*(bc+1)
L525: / 317
hl=&0177764+de
sp<=hl
hl=&0177760+de
bc<=sp
a=*bc-*hl=*(bc+1)-^*(hl+1)
jnc L725
hl=&0177756+de
sp<=hl
hl=&0177760+de
hl=a^hl
hl+(bc=&0177774)
bc<=sp
a=l->*bc=h->*(bc+1)
L535: / 318
hl=&0177756+de
sp<=hl
hl=&0177764+de
bc<=sp
a=*bc-*hl=*(bc+1)-^*(hl+1)
jc L135
hl=&0177760+de
hl=a^hl
sp<=hl
hl=&0177756+de
hl=a^hl
bc<=sp
a=*bc-*hl=*(bc+1)-^*(hl+1)
jnc L145
hl=&0177750+de
sp<=hl
hl=&0177760+de
hl=a^hl
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
hl=&0177760+de
hl=a^hl
sp<=hl
hl=&0177756+de
hl=a^hl
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
hl=&0177756+de
hl=a^hl
sp<=hl
hl=&0177750+de
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
hl=&0177746+de
sp<=hl
hl=&0177760+de
hl=a^hl
hl+1+1
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
hl=&0177760+de
hl=a^hl
hl+1+1
sp<=hl
hl=&0177756+de
hl=a^hl
hl+1+1
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
hl=&0177756+de
hl=a^hl
hl+1+1
sp<=hl
hl=&0177746+de
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
jmp L145
L725: /328
hl=&0177770+de
sp<=hl
hl=&0177762+de
hl=a^hl
sp<=hl
hl=&0177764+de
bc<=sp
a=c-*hl->c=b-^*(hl+1)->b
sp<=bc
hl=&04=>sp
call c.udiv
hl<=sp
bc<=sp
a=l->*bc=h->*(bc+1)
hl=&04+de
a=*hl-0=*(hl+1)-^020
jm L26
hl=&04+de
hl=a^hl
hl+hl
bc=&_ntstate[0160000]
hl+bc
bc=^hl
jmp L46
L26:
hl=&04+de
hl=a^hl
hl+hl
bc=&_tstates
hl+bc
bc=^hl
L46:
hl=bc
hl->c.r4
L745: / 331
hl=&c.r4
a=*hl|*(hl+1)
jz L155
hl=&0177754+de
sp<=hl
hl=c.r4
hl+hl
bc=&_pstate
hl+bc
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
hl=&0177752+de
sp<=hl
hl=c.r4+(bc=&01)
hl+hl
bc=&_pstate
hl+bc
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
hl=&0177766+de
sp<=hl
hl=&0177752+de
hl=a^hl
sp<=hl
hl=&0177754+de
bc<=sp
a=c-*hl->c=b-^*(hl+1)->b
sp<=bc
hl=&04=>sp
call c.udiv
hl<=sp
bc<=sp
a=l->*bc=h->*(bc+1)
hl=&0177770+de
sp<=hl
hl=&0177766+de
bc<=sp
a=*bc::*hl jnz L66=*(bc+1)::*(hl+1)
L66:
jz L755
jmp L355
L135: /317
hl=&0177760+de
a=*hl-04->*hl=*(hl+1)-^0->*hl
jmp L525
L145: /318
hl=&0177756+de
a=*hl-04->*hl=*(hl+1)-^0->*hl
jmp L535
L155: /357
hl=&_nolook
a=*hl|*(hl+1)
jz L126
hl=&L326
sp<=hl
call _error
af<=sp
jmp L126
L755: /337
hl=&0177760+de
sp<=hl
hl=&0177764+de
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
hl=&0177756+de
sp<=hl
hl=&0177754+de
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
L165: / 338
hl=&0177756+de
sp<=hl
hl=&0177752+de
bc<=sp
a=*bc-*hl=*(bc+1)-^*(hl+1)
jnc L365
hl=&0177756+de
hl=a^hl
sp<=hl
hl=&0177760+de
hl=a^hl
bc<=sp
a=*bc::*hl jnz L07=*(bc+1)::*(hl+1)
L07:
jz L175
L365: /341
hl=&0177756+de
sp<=hl
hl=&0177752+de
bc<=sp
a=*bc::*hl jnz L27=*(bc+1)::*(hl+1)
L27:
jz L375
L355: / 331
hl=c.r4
hl+hl
bc=&_mstates
hl+bc
hl=a^hl
hl->c.r4
jmp L745
L175: /340
hl=&0177760+de
a=*hl+04->*hl=*(hl+1)+^0->*hl
hl=&0177756+de
a=*hl+04->*hl=*(hl+1)+^0->*hl
jmp L165
L375: /344
hl=_nstate
hl+hl
bc=&_pstate[02]
hl+bc
sp<=hl
hl=_nstate
hl+hl
bc=&_pstate
hl+bc
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
hl=&_nolook
a=*hl|*(hl+1)
jz L575
hl=c.r4
bc=hl
jmp c.rets
L575: /347
hl=&0177756+de
sp<=hl
hl=&0177754+de
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
hl=&0177760+de
sp<=hl
hl=&0177764+de
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
L775: / 347
hl=&0177756+de
sp<=hl
hl=&0177752+de
bc<=sp
a=*bc-*hl=*(bc+1)-^*(hl+1)
jnc L106
hl=&0177750+de
a-a->*hl->*(hl+1)
L706: / 349
hl=&0177750+de
sp<=hl
hl=&_tbitset
bc<=sp
a=*bc-*hl=*(bc+1)-^*(hl+1)
jp L116
hl=&0177750+de
hl=a^hl
hl+hl
bc=&_clset
hl+bc
sp<=hl
hl=&0177756+de
hl=a^hl
hl+1+1
hl=a^hl
sp<=hl
hl=&0177750+de
hl=a^hl
hl+hl
hl<>*sp;bc<=sp
hl+bc
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
hl=&0177750+de
a=*hl+0401->*hl=*(hl+1)+^0->*hl
jmp L706
L106: /355
hl=c.r4
bc=hl
jmp c.rets
L306: /347
hl=&0177756+de
a=*hl+04->*hl=*(hl+1)+^0->*hl
hl=&0177760+de
a=*hl+04->*hl=*(hl+1)+^0->*hl
jmp L775
L116: /349
hl=&0177760+de
hl=a^hl
hl+1+1
bc=^hl=>sp
hl=&_clset
sp<=hl
call _setunio
af<=sp<=sp
a=c|b
jz L306
hl=c.r4
hl+hl
bc=&_tystate
hl+bc
a=0401->*hl+a-^a->*(hl+1)
hl=&0177756+de
hl=a^hl
hl+1+1
sp<=hl
hl=&_clset
sp<=hl
call _flset
af<=sp
hl<=sp
a=c->*hl=b->*(hl+1)
jmp L306
L126: /360
hl=_nstate
hl+hl
bc=&_pstate[04]
hl+bc
sp<=hl
hl=&0177762+de
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
hl=_nstate
hl+1
a=l-0130=h-^02
jm L526
hl=&L726
sp<=hl
call _error
af<=sp
L526: /362
hl=&04+de
a=*hl-0=*(hl+1)-^020
jm L136
hl=_nstate
hl+hl
bc=&_mstates
hl+bc
sp<=hl
hl=&04+de
hl=a^hl
hl+hl
bc=&_ntstate[0160000]
hl+bc
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
hl=&04+de
hl=a^hl
hl+hl
bc=&_ntstate[0160000]
hl+bc
a=_nstate->*hl=_nstate[01]->*(hl+1)
jmp L336
L136: /366
hl=_nstate
hl+hl
bc=&_mstates
hl+bc
sp<=hl
hl=&04+de
hl=a^hl
hl+hl
bc=&_tstates
hl+bc
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
hl=&04+de
hl=a^hl
hl+hl
bc=&_tstates
hl+bc
a=_nstate->*hl=_nstate[01]->*(hl+1)
L336: / 369
hl=_nstate
hl+hl
bc=&_tystate
hl+bc
a=0401->*hl+a-^a->*(hl+1)
hl=_nstate=>sp
hl=_nstate
hl+1
hl->_nstate
hl<=sp
bc=hl
jmp c.rets
L746:
0157,0165,0164,040,0157,0146,040,0163
0164,0141,0164,0145,040,0163,0160,0141
0143,0145,0
L736:
0160,0165,0164,0151,0164,0145,0155,050
045,0163,051,054,040,0163,0164,0141
0164,0145,040,045,0144,012,0
_putitem:
call c.ents
hl=&_pidebug
a=*hl|*(hl+1)
jz L536
hl=&_foutput
a=*hl|*(hl+1)
jz L536
hl=_nstate=>sp
hl=&04+de
bc=^hl=>sp
call _writem
af<=sp
sp<=bc
hl=&L736
sp<=hl
hl=_foutput=>sp
call _fprintf
af<=sp<=sp<=sp<=sp
L536: /381
hl=_nstate
hl+hl
bc=&_pstate[02]
hl+bc
hl=a^hl
hl->c.r4
hl=c.r4
sp<=hl
hl=&04+de
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
hl=&_nolook
a=*hl|*(hl+1)
jnz L146
hl=c.r4+(bc=&02)
sp<=hl
hl=&06+de
bc=^hl=>sp
call _flset
af<=sp
hl<=sp
a=c->*hl=b->*(hl+1)
L146: /384
hl=_nstate
hl+hl
bc=&_pstate[02]
hl+bc
sp<=hl
hl=c.r4
hl+1+1+1+1
hl->c.r4
hl<=sp
a=c.r4->*hl=c.r4[01]->*(hl+1)
hl=c.r4
a=_zzmemsz-l=_zzmemsz[01]-^h
jnc L346
hl=c.r4
hl->_zzmemsz
hl=&_mem0[024240]
a=_zzmemsz-l=_zzmemsz[01]-^h
jc L346
hl=&L746
sp<=hl
call _error
af<=sp
L346: /389
jmp c.rets
L517:
0156,0157,0156,0164,0145,0162,0155,0151
0156,0141,0154,040,045,0163,040,0156
0145,0166,0145,0162,040,0144,0145,0162
0151,0166,0145,0163,040,0141,0156,0171
040,0164,0157,0153,0145,0156,040,0163
0164,0162,0151,0156,0147,0
.:=.data
_gsdebug:
&0
.:=.text
_cempty:
call c.ents
hl=&0=>sp
hl=_nnonter
hl+1
sp<=hl
hl=&_pempty
sp<=hl
call _aryfil
af<=sp<=sp<=sp
L156: / 406
a-a->c.r4->c.r4[01]
L356: / 407
hl=&_nprod
a=c.r4-*hl=c.r4[01]-^*(hl+1)
jp L556
hl=c.r4
hl+hl
bc=&_prdptr
hl+bc
hl=a^hl
hl=a^hl
hl+hl
bc=&_pempty[0160000]
hl+bc
a=*hl|*(hl+1)
jz L366
jmp L756
L556: /416
a-a->c.r4->c.r4[01]
L107: / 420
hl=&c.r4
a=_nnonter-*hl=_nnonter[01]-^*(hl+1)
jm L307
hl=&c.r4
a=*hl|*(hl+1)
jnz L117
jmp L507
L756: /407
hl=c.r4
hl+1
hl->c.r4
jmp L356
L366: /409
hl=c.r4
hl+hl
bc=&_prdptr
hl+bc
hl=a^hl
hl+1+1
hl->c.r2
L566: / 409
hl=c.r2
a=*(hl+1)|a
jm L766
hl=c.r2
a=*hl-0=*(hl+1)-^020
jm L176
hl=c.r2
hl=a^hl
hl+hl
bc=&_pempty[0160000]
hl+bc
a=*hl|*(hl+1)
jnz L176
L766: /411
hl=c.r2
a=*(hl+1)|a
jp L756
hl=c.r4
hl+hl
bc=&_prdptr
hl+bc
hl=a^hl
hl=a^hl
hl+hl
bc=&_pempty[0160000]
hl+bc
a=0401->*hl+a-^a->*(hl+1)
jmp L156
L176: /409
hl=c.r2
hl+1+1
hl->c.r2
jmp L566
L307: /427
hl=&_nerrors
a=*hl|*(hl+1)
jz L717
call _summary
hl=&01=>sp
call _exit
af<=sp
jmp L717
L507: /420
hl=c.r4
hl+1
hl->c.r4
jmp L107
L117: /423
hl=c.r4
hl+hl
bc=&_pempty
hl+bc
a=*hl::0401 jnz L47=*(hl+1)::0
L47:
jz L507
a-a->_fatfl->_fatfl[01]
hl=c.r4
hl+hl+hl
bc=&_nontrst
hl+bc
bc=^hl=>sp
hl=&L517
sp<=hl
call _error
af<=sp<=sp
jmp L507
L717: /438
hl=&0=>sp
hl=_nnonter
hl+1
sp<=hl
hl=&_pempty
sp<=hl
call _aryfil
af<=sp<=sp<=sp
L127: / 442
a=0401->c.r4+a-^a->c.r4[01]
L327: / 443
hl=&_nprod
a=c.r4-*hl=c.r4[01]-^*(hl+1)
jp L527
hl=c.r4
hl+hl
bc=&_prdptr
hl+bc
hl=a^hl
hl=a^hl
hl+hl
bc=&_pempty[0160000]
hl+bc
a=*hl|*(hl+1)
jnz L727
hl=c.r4
hl+hl
bc=&_prdptr
hl+bc
hl=a^hl
hl+1+1
hl->c.r2
L537: / 445
hl=c.r2
a=*hl-0=*(hl+1)-^020
jm L737
hl=c.r2
hl=a^hl
hl+hl
bc=&_pempty[0160000]
hl+bc
a=*hl::0401 jnz L67=*(hl+1)::0
L67:
jnz L737
L147: / 445
hl=c.r2
hl+1+1
hl->c.r2
jmp L537
L527: /451
jmp c.rets
L727: /443
hl=c.r4
hl+1
hl->c.r4
jmp L327
L737: /445
hl=c.r2
a=*(hl+1)|a
jp L727
hl=c.r4
hl+hl
bc=&_prdptr
hl+bc
hl=a^hl
hl=a^hl
hl+hl
bc=&_pempty[0160000]
hl+bc
a=0401->*hl+a-^a->*(hl+1)
jmp L127
L7301:
012,0
L5301:
045,0163,040,045,0144,054,040,0
L1201:
045,0144,072,040,0
.:=.data
_cldebug:
&0
.:=.text
_stagen:
call c.ents
af=>sp=>sp
a-a->_nstate->_nstate[01]
hl=_mem
hl->_pstate[02]
hl=_pstate[02]
hl->_pstate
hl=&0=>sp
hl=_tbitset=>sp
hl=&_clset
sp<=hl
call _aryfil
af<=sp<=sp<=sp
hl=&_clset
sp<=hl
hl=_prdptr
hl+1+1
sp<=hl
call _putitem
af<=sp<=sp
a=0401->_tystate+a-^a->_tystate[01]
a=0401->_nstate+a-^a->_nstate[01]
hl=_pstate[02]
hl->_pstate[04]
hl=&0=>sp
hl=&07640=>sp
hl=&_amem
sp<=hl
call _aryfil
af<=sp<=sp<=sp
L747: / 483
hl=&0177770+de
a-a->*hl->*(hl+1)
L157: / 484
hl=&0177770+de
sp<=hl
hl=&_nstate
bc<=sp
a=*bc-*hl=*(bc+1)-^*(hl+1)
jp L357
hl=&0177770+de
hl=a^hl
hl+hl
bc=&_tystate
hl+bc
a=*hl::0401 jnz L001=*(hl+1)::0
L001:
jz L167
hl=&0177770+de
a=*hl+0401->*hl=*(hl+1)+^0->*hl
jmp L157
L357: /521
jmp c.rets
L167: /486
hl=&0177770+de
hl=a^hl
hl+hl
bc=&_tystate
hl+bc
a-a->*hl->*(hl+1)
hl=&0=>sp
hl=_nnonter
hl+1
sp<=hl
hl=&_temp1
sp<=hl
call _aryfil
af<=sp<=sp<=sp
hl=&0177770+de
bc=^hl=>sp
call _closure
af<=sp
hl=&_wsets
hl->c.r2
L367: / 490
hl=&_cwp
a=c.r2-*hl=c.r2[01]-^*(hl+1)
jnc L567
hl=c.r2+(bc=&02)
a=*hl|*(hl+1)
jz L377
jmp L767
L567: /511
hl=&_gsdebug
a=*hl|*(hl+1)
jz L7101
hl=&_foutput
a=*hl|*(hl+1)
jz L7101
hl=&0177770+de
bc=^hl=>sp
hl=&L1201
sp<=hl
hl=_foutput=>sp
call _fprintf
af<=sp<=sp<=sp
hl=&0177766+de
a-a->*hl->*(hl+1)
L3201: / 514
hl=&0177766+de
a=_nnonter-*hl=_nnonter[01]-^*(hl+1)
jm L5201
hl=&0177766+de
hl=a^hl
hl+hl
bc=&_temp1
hl+bc
a=*hl|*(hl+1)
jz L7201
hl=&0177766+de
hl=a^hl
hl+hl
bc=&_temp1
hl+bc
bc=^hl=>sp
hl=&0177766+de
hl=a^hl
hl+hl+hl
bc=&_nontrst
hl+bc
bc=^hl=>sp
hl=&L5301
sp<=hl
hl=_foutput=>sp
call _fprintf
af<=sp<=sp<=sp<=sp
jmp L7201
L377: /492
hl=c.r2+(bc=&02)
a=0401->*hl+a-^a->*(hl+1)
hl=c.r2
hl=a^hl
hl=a^hl
hl->c.r4
hl=&c.r4
a=0401-*hl=0-^*(hl+1)
jm L577
hl=c.r2
bc=&_wsets
a=l-c->l=h-^b->h
sp<=hl
hl=&024=>sp
call c.udiv
hl<=sp
sp<=hl
hl=&0177770+de
hl=a^hl
hl+hl
bc=&_pstate[02]
hl+bc
hl=a^hl
sp<=hl
hl=&0177770+de
hl=a^hl
hl+hl
bc=&_pstate
hl+bc
bc<=sp
a=c-*hl->c=b-^*(hl+1)->b
sp<=bc
hl=&04=>sp
call c.udiv
hl<=sp
bc<=sp
a=c-l=b-^h
jm L767
hl=&0177770+de
hl=a^hl
hl+hl
bc=&_tystate
hl+bc
a=02->*hl+a-^a->*(hl+1)
jmp L767
L577: /499
hl=c.r2
hl->c.r3
L1001: / 499
hl=&_cwp
a=c.r3-*hl=c.r3[01]-^*(hl+1)
jnc L3001
hl=c.r3
hl=a^hl
a=c.r4::*hl jnz L201=c.r4[01]::*(hl+1)
L201:
jnz L5001
hl=c.r3+(bc=&04)
sp<=hl
hl=c.r3
hl=a^hl
hl+1+1
sp<=hl
call _putitem
af<=sp<=sp
hl=c.r3+(bc=&02)
a=0401->*hl+a-^a->*(hl+1)
jmp L5001
L3001: /504
a=c.r4-0=c.r4[01]-^020
jp L3101
hl=c.r4=>sp
call _state
af<=sp
jmp L767
L5001: /499
hl=c.r3
a=l+024->l=h+^0->h
hl->c.r3
jmp L1001
L3101: /508
hl=c.r4+(bc=&0170000)
hl+hl
bc=&_temp1
hl+bc
sp<=hl
hl=c.r4=>sp
call _state
af<=sp
hl<=sp
a=c->*hl=b->*(hl+1)
L767: / 490
hl=c.r2
a=l+024->l=h+^0->h
hl->c.r2
jmp L367
L7101: /519
hl=&0177770+de
hl=a^hl
hl+hl
bc=&_indgo
hl+bc
sp<=hl
hl=_nnonter
hl+(bc=&0177777)
sp<=hl
hl=&_temp1[02]
sp<=hl
call _apack
af<=sp<=sp
hl=&0177777+bc
bc<=sp
a=l->*bc=h->*(bc+1)
jmp L747
L5201: /516
hl=&L7301
sp<=hl
hl=_foutput=>sp
call _fprintf
af<=sp<=sp
jmp L7101
L7201: /514
hl=&0177766+de
a=*hl+0401->*hl=*(hl+1)+^0->*hl
jmp L3201
L1221:
012,0
L7121:
011,045,0163,0
L5121:
0146,0154,0141,0147,040,0163,0145,0164
041,012,0
L1021:
012,0123,0164,0141,0164,0145,040,045
0144,054,040,0156,0157,0154,0157,0157
0153,040,075,040,045,0144,012,0
L1611:
0167,0157,0162,0153,0151,0156,0147,040
0163,0145,0164,040,0157,0166,0145,0162
0146,0154,0157,0167,0
_closure:
call c.ents
hl=0177760+sp->sp
hl=_zzclose
hl+1
hl->_zzclose
hl=&_wsets
hl->_cwp
hl=&0177752+de
sp<=hl
hl=&04+de
hl=a^hl
hl+hl
bc=&_pstate[02]
hl+bc
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
hl=&04+de
hl=a^hl
hl+hl
bc=&_pstate
hl+bc
hl=a^hl
hl->c.r3
L1401: / 540
hl=&0177752+de
a=c.r3-*hl=c.r3[01]-^*(hl+1)
jnc L3401
hl=_cwp
sp<=hl
hl=c.r3
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
hl=_cwp
hl+1+1
a=0401->*hl+a-^a->*(hl+1)
hl=&0177762+de
a-a->*hl->*(hl+1)
L1501: / 543
hl=&0177762+de
sp<=hl
hl=&_tbitset
bc<=sp
a=*bc-*hl=*(bc+1)-^*(hl+1)
jp L3501
hl=_cwp
hl+1+1+1+1
sp<=hl
hl=&0177762+de
hl=a^hl
hl+hl
hl<>*sp;bc<=sp
hl+bc
sp<=hl
hl=&0177762+de
hl=a^hl
hl+hl
sp<=hl
hl=c.r3+(bc=&02)
hl=a^hl
hl<>*sp;bc<=sp
hl+bc
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
hl=&0177762+de
a=*hl+0401->*hl=*(hl+1)+^0->*hl
jmp L1501
L3401: /545
hl=&0177764+de
a=0401->*hl+a-^a->*(hl+1)
L1601: / 550
hl=&0177764+de
a=*hl|*(hl+1)
jz L3601
hl=&0177764+de
a-a->*hl->*(hl+1)
hl=&_wsets
hl->c.r4
L5601: / 552
hl=&_cwp
a=c.r4-*hl=c.r4[01]-^*(hl+1)
jnc L1601
hl=c.r4+(bc=&02)
a=*hl|*(hl+1)
jnz L5701
jmp L1701
L3501: /543
hl=_cwp
a=l+024->l=h+^0->h
hl->_cwp
hl=c.r3
hl+1+1+1+1
hl->c.r3
jmp L1401
L3601: /613
hl=&_cwp
a=_zzcwp-*hl=_zzcwp[01]-^*(hl+1)
jnc L5711
hl=_cwp
hl->_zzcwp
jmp L5711
L1701: /552
hl=c.r4
a=l+024->l=h+^0->h
hl->c.r4
jmp L5601
L5701: /555
hl=&0177770+de
sp<=hl
hl=c.r4
hl=a^hl
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
hl=&0177770+de
a=*hl-0=*(hl+1)-^020
jp L7701
hl=c.r4+(bc=&02)
a-a->*hl->*(hl+1)
jmp L1701
L7701: /563
hl=&0=>sp
hl=_tbitset=>sp
hl=&_clset
sp<=hl
call _aryfil
af<=sp<=sp<=sp
hl=c.r4
hl->c.r2
L1011: / 567
hl=&_cwp
a=c.r2-*hl=c.r2[01]-^*(hl+1)
jnc L3011
hl=c.r2+(bc=&02)
a=*hl::0401 jnz L401=*(hl+1)::0
L401:
jnz L5011
hl=&0177760+de
sp<=hl
hl=c.r2
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
bc-1
hl=bc=a^hl
sp<=hl
hl=&0177770+de
bc<=sp
a=*bc::*hl jnz L601=*(bc+1)::*(hl+1)
L601:
jnz L5011
hl=c.r2+(bc=&02)
a-a->*hl->*(hl+1)
hl=&_nolook
a=*hl|*(hl+1)
jz L5111
jmp L5011
L3011: /582
hl=&0177770+de
a=*(hl+1)-020->*hl
hl=&0177754+de
sp<=hl
hl=&0177770+de
hl=a^hl
hl+hl
bc=&_pres[02]
hl+bc
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
hl=&0177756+de
sp<=hl
hl=&0177770+de
hl=a^hl
hl+hl
bc=&_pres
hl+bc
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
L7211: / 589
hl=&0177756+de
sp<=hl
hl=&0177754+de
bc<=sp
a=*bc-*hl=*(bc+1)-^*(hl+1)
jnc L1701
hl=&_wsets
hl->c.r2
L7311: / 591
hl=&_cwp
a=c.r2-*hl=c.r2[01]-^*(hl+1)
jnc L1411
hl=c.r2
sp<=hl
hl=&0177756+de
hl=a^hl
bc<=sp
a=*bc::*hl jnz L011=*(bc+1)::*(hl+1)
L011:
jnz L3411
hl=&_nolook
a=*hl|*(hl+1)
jz L1511
jmp L3311
L5011: /567
hl=c.r2
a=l+024->l=h+^0->h
hl->c.r2
jmp L1011
L5111: /571
hl=&0177766+de
sp<=hl
hl=&0177760+de
a=*hl+02->*hl=*(hl+1)+^0->*hl
hl-1
hl=a^hl
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
bc-1
hl=bc
a-a-*hl=0-^*(hl+1)
jp L7111
hl=&0177766+de
a=*hl-0=*(hl+1)-^020
jp L1211
hl=&0177766+de
bc=^hl=>sp
hl=&04=>sp
call c.irsh
hl<=sp
hl+hl
bc=&_clset
hl+bc
sp<=hl
hl=&01=>sp
hl=&0177766+de
hl=a^hl
a=l&017->l-a->h
sp<=hl
call c.ilsh
hl<=sp
bc<=sp
a=*bc|l->*bc=*(bc+1)|h->*bc
L7111: /579
hl=&0177766+de
a-a-*hl=0-^*(hl+1)
jm L5011
hl=c.r2+(bc=&04)
sp<=hl
hl=&_clset
sp<=hl
call _setunio
af<=sp<=sp
jmp L5011
L1211: /577
hl=&0177766+de
hl=a^hl
hl+hl
bc=&_pfirst[0160000]
hl+bc
bc=^hl=>sp
hl=&_clset
sp<=hl
call _setunio
af<=sp<=sp
hl=&0177766+de
hl=a^hl
hl+hl
bc=&_pempty[0160000]
hl+bc
a=*hl|*(hl+1)
jnz L5111
jmp L7111
L1411: /597
hl=_cwp
bc=&_wsets
a=l-c->l=h-^b->h
sp<=hl
hl=&024=>sp
call c.udiv
hl<=sp
hl+1
a=l-0372=h-^0
jm L7511
hl=&L1611
sp<=hl
call _error
af<=sp
jmp L7511
L3411: /591
hl=c.r2
a=l+024->l=h+^0->h
hl->c.r2
jmp L7311
L1511: /594
hl=&_clset
sp<=hl
hl=c.r2+(bc=&04)
sp<=hl
call _setunio
af<=sp<=sp
a=c|b
jz L3311
hl=c.r2+(bc=&02)
sp<=hl
hl=&0177764+de
a=0401->*hl+a-^a->*(hl+1)
hl-1
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
jmp L3311
L7511: /601
hl=_cwp
sp<=hl
hl=&0177756+de
hl=a^hl
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
hl=_cwp
hl+1+1
a=0401->*hl+a-^a->*(hl+1)
hl=&_nolook
a=*hl|*(hl+1)
jnz L3611
hl=&0177764+de
a=0401->*hl+a-^a->*(hl+1)
hl=&0177762+de
a-a->*hl->*(hl+1)
L5611: / 605
hl=&0177762+de
sp<=hl
hl=&_tbitset
bc<=sp
a=*bc-*hl=*(bc+1)-^*(hl+1)
jp L3611
hl=_cwp
hl+1+1+1+1
sp<=hl
hl=&0177762+de
hl=a^hl
hl+hl
hl<>*sp;bc<=sp
hl+bc
sp<=hl
hl=&0177762+de
hl=a^hl
hl+hl
bc=&_clset
hl+bc
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
hl=&0177762+de
a=*hl+0401->*hl=*(hl+1)+^0->*hl
jmp L5611
L3611: /607
hl=_cwp
a=l+024->l=h+^0->h
hl->_cwp
L3311: / 589
hl=&0177756+de
a=*hl+02->*hl=*(hl+1)+^0->*hl
jmp L7211
L5711: /618
hl=&_cldebug
a=*hl|*(hl+1)
jz L7711
hl=&_foutput
a=*hl|*(hl+1)
jz L7711
hl=_nolook=>sp
hl=&04+de
bc=^hl=>sp
hl=&L1021
sp<=hl
hl=_foutput=>sp
call _fprintf
af<=sp<=sp<=sp<=sp
hl=&_wsets
hl->c.r4
L3021: / 620
hl=&_cwp
a=c.r4-*hl=c.r4[01]-^*(hl+1)
jnc L7711
hl=c.r4+(bc=&02)
a=*hl|*(hl+1)
jz L3121
hl=&L5121
sp<=hl
hl=_foutput=>sp
call _fprintf
af<=sp<=sp
jmp L3121
L7711: /628
jmp c.rets
L3121: /622
hl=c.r4+(bc=&02)
a-a->*hl->*(hl+1)
hl=c.r4
bc=^hl=>sp
call _writem
af<=sp
sp<=bc
hl=&L7121
sp<=hl
hl=_foutput=>sp
call _fprintf
af<=sp<=sp<=sp
hl=c.r4+(bc=&04)
sp<=hl
call _prlook
af<=sp
hl=&L1221
sp<=hl
hl=_foutput=>sp
call _fprintf
af<=sp<=sp
hl=c.r4
a=l+024->l=h+^0->h
hl->c.r4
jmp L3021
L1421:
0164,0157,0157,040,0155,0141,0156,0171
040,0154,0157,0157,0153,0141,0150,0145
0141,0144,040,0163,0145,0164,0163,0
public _cldebug
public _closure
public _apack
public _gsdebug
public _putitem
public _pidebug
public _state
public _indebug
public _prlook
public _setunio
public _fprintf
public _zzmemsz
public _zzcwp
public _sprintf
public _chcopy
public _fclose
public _unlink
public __flushb
public __fillbu
public _aryfil
public _warray
public _error
public _fopen
public _others
public _callopt
public _summary
public _hidepro
public _go2out
public _output
public _stagen
public _cpfir
public _cempty
public _cpres
public _setup
public _main
public _pempty
public _pfirst
public _pres
public _nerrors
public _fatfl
public _clset
public _nlset
public _tbitset
public _writem
public _symnam
public _flset
public _cstash
public _zzsrcon
public _zzrrcon
public _zzclose
public _zzexcp
public _zzacent
public _zzgobes
public _zzgoent
public _lineno
public _temp1
public _indgo
public _memp
public _amem
public _mem
public _mem0
public _cwp
public _wsets
public _nolook
public _lkst
public _mstates
public _ntstate
public _tstates
public _defact
public _tystate
public _pstate
public _nstate
public _levprd
public _prdptr
public _nprod
public _nontrst
public _nnonter
public _toklev
public _tokset
public _ntokens
public _foutput
public _ftemp
public _ftable
public _fdefine
public _faction
public _finput
public _exit
public __exit
public __iob
_flset:
call c.ents
af=>sp=>sp
hl=_nlset=>sp
hl=&020=>sp
call c.imul
hl<=sp
bc=&_lkst
hl+bc
hl->c.r4
L3221: / 638
hl=c.r4=>sp
a=c.r4-020->c.r4=c.r4[01]-^0->c.r4[01]
hl<=sp
bc=&_lkst
a=c-l=b-^h
jnc L5221
hl=&04+de
hl=a^hl
hl->c.r2
hl=c.r4
hl->c.r3
hl=&0177766+de
sp<=hl
hl=_tbitset
hl+hl
sp<=hl
hl=c.r3
hl<>*sp;bc<=sp
hl+bc
bc<=sp
a=l->*bc=h->*(bc+1)
L7221: / 642
hl=&0177766+de
a=c.r3-*hl=c.r3[01]-^*(hl+1)
jnc L1321
hl=c.r2=>sp
hl=c.r2
hl+1+1
hl->c.r2
hl=c.r3=>sp
hl=c.r3
hl+1+1
hl->c.r3
hl<=sp
bc<=sp
a=*bc::*hl jnz L211=*(bc+1)::*(hl+1)
L211:
jz L7221
jmp L3221
L5221: /646
hl=_nlset=>sp
hl=_nlset
hl+1
hl->_nlset
hl=&020=>sp
call c.imul
hl<=sp
bc=&_lkst
hl+bc
hl->c.r4
a=_nlset-0302=_nlset[01]-^0401
jm L7321
hl=&L1421
sp<=hl
call _error
af<=sp
jmp L7321
L1321: /644
hl=c.r4
bc=hl
jmp c.rets
L7321: /650
hl=&0177770+de
a-a->*hl->*(hl+1)
L3421: / 650
hl=&0177770+de
sp<=hl
hl=&_tbitset
bc<=sp
a=*bc-*hl=*(bc+1)-^*(hl+1)
jp L5421
hl=&0177770+de
hl=a^hl
hl+hl
sp<=hl
hl=c.r4
hl<>*sp;bc<=sp
hl+bc
sp<=hl
hl=&0177770+de
hl=a^hl
hl+hl
sp<=hl
hl=&04+de
hl=a^hl
hl<>*sp;bc<=sp
hl+bc
bc<=sp
a=*hl->*bc=*(hl+1)->*(bc+1)
hl=&0177770+de
a=*hl+0401->*hl=*(hl+1)+^0->*hl
jmp L3421
L5421: /652
hl=c.r4
bc=hl
jmp c.rets
