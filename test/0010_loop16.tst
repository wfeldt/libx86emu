[init mode=protected]

gdt.base=0x2000
gdt[0x00]=0
gdt[0x08]=descr(base=0x1000,limit=0xffff,acc=0x093)
gdt[0x10]=descr(base=0,limit=0xffffffff,acc=0xc93)

[code start=8:0 bits=16]

	mov ecx,0x70010
l1:
	inc ebx
	loop l1

