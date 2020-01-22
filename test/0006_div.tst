[init mode=real]

idt.base=0x1000

[code start=00:0x2000]

	mov ax,1
	mov dx,1
	mov cx,0
	div cx
	push ax
	push dx

	mov ax,0x567
	mov dx,0x67
	mov cx,0x89
	div cx
	push ax
	push dx

	mov ax,0x567f
	mov dx,0x3267
	div word [data_1]
	push ax
	push dx

	mov ax,0x7812
	mov cl,0x96
	div cl
	push ax

	mov ax,0x2812
	div byte [data_1]
	push ax

	mov ax,1
	mov dx,1
	mov cx,0
	idiv cx
	push ax
	push dx

	mov ax,0x567
	mov dx,0x67
	mov cx,0x1892
	idiv cx
	push ax
	push dx

	mov ax,0x567f
	mov dx,0x1267
	idiv word [data_1]
	push ax
	push dx

	mov ax,0x3812
	mov cl,0x86
	idiv cl
	push ax

	mov ax,0x2812
	idiv byte [data_1]
	push ax

	hlt

interrupt_00:
	xor eax,eax
	xor edx,edx
	mov ecx,0x99999999
	iret

data_1	dd 0x12345678
