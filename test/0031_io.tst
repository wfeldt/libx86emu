[init mode=real srand=31]

idt.base=0x9000

eax=rand edx=rand

[code start=0x100:0x50]

	out dx,al
	out dx,ax
	out dx,eax

	in al,dx
	in ax,dx
	in eax,dx

	in al,0x57
	out 0x67,al

