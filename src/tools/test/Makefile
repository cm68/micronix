test:	../disas ../dump
	../disas -a 0x100 -e 0x18b DDT.COM >DDTSTART.ASM
	../disas -s 0x100 -a 0 -f DDT.SYM DDT.COM >DDTEND.ASM
	../dump DDT.COM
