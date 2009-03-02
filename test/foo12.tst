[init mode=real srand=31]

idt.base=0x9000

edx=0x3da

[code start=0x100:0x50]

	in al,dx
	mov ah,al
	in al,dx
	shl eax,16
	in al,dx
	mov ah,al
	out dx,al
	in al,dx

