[init mode=real]

idt.base=0x9000

edx=0x1
eax=0x2045

[code start=0x100:0x50]

	div cx
	hlt

interrupt_00:
	pop ebx
	push ebx
	mov cx,0x100
	iret

