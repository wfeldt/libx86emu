[init mode=protected]

gdt.base=0x2000
gdt[0x00]=0
gdt[0x08]=descr(base=0x1000,limit=0xffff,acc=0x493)
gdt[0x10]=descr(base=0,limit=0xffffffff,acc=0xc93)

ss=0x10

[code start=8:0 bits=32]

	push 0x12345678
	pop eax
	push ss
	pop ds
	push ds
	pop eax
