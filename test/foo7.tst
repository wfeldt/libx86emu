[init mode=pm]

eax=0xffff

gdt.base=0x2000
gdt[0]=0
gdt[8]=descr(base=0x1000,limit=0x8fff,acc=0x9b)
gdt[16]=descr(base=0x3000,limit=0xa0000fff,acc=0xc93)

[code start=8:0 bits=16]

	inc ax

	mov bx,16
	mov es,bx

	mov dword [es:0x50],0x12345678

	mov ecx,0x10
l1:
	inc ebx
	loop l1

	mov bx,12
	mov fs,bx

	nop

