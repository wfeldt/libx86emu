[init mode=real]

idt.base=0x9000

edx=0x1
eax=0x2045

[code start=0x100:0x50]

	sti

	int 8
	mov dl,0x77
	hlt

interrupt_08:
	pop ebx
	push ebx
	mov cx,0x100
	iret

