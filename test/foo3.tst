[init]

ecx=0x1234

[code start=0x100:0x0]

	mov es,ecx

	call l1
	mov [0x2000],es

l1:
	mov [0x3000],es

	call l2
	mov [0x2004],es
l2:
	mov [0x3004],es

	call 0xc0:l3+0x400
	mov [0x2008],es

	hlt
l3:
	mov [0x3008],es

	retf
