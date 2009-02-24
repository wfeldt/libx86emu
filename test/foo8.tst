[init mode=pm]

gdt.base=0x2000
gdt[0]=0
gdt[8]=descr(base=0x1000,limit=0x8fff,acc=0x9b)
gdt[16]=descr(base=0x3000,limit=0xa0000fff,acc=0xc93)

[code start=8:0 bits=16]

	mov bx,12
	mov fs,bx
	nop

