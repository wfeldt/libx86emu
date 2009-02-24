[init]

0x1700: 1 2

ecx=0x6789
edx=0x5555
esp=0x9000

[code start=0x100:0x0]


	mov es,cx
	mov fs,edx
	mov gs,[cs:0x700]
	push gs
	mov eax,0x1000
	mov [cs:eax],es
	mov [cs:eax+0x77],es
	mov [cs:eax-0x66],es
	mov [ds:bx],es
	mov [ss:0x1234],es
	mov [ss:bx+si+0x12],es
	mov [ss:eax+ebx],es
	mov [ss:eax+ebx+0x12],es
	mov [ss:eax+ebx+0x12345678],es
	mov [ss:eax+ebx*4],es
	mov [ss:eax+ebx*4+0x12],es
	mov [ss:eax+ebx*4+0x12345678],es
	mov [ss:ebx*8+0x12345678],es
	mov [ss:esi*2],es
	mov [ss:esi-1],es
	mov [ss:esi*2-1],es
	mov [ss:esi*2+1],es
	mov [ss:esi*8],es
	mov [ss:esi*8+1],es
	mov [ebp+1],es
	mov [esp+1],es
	db 0x36, 0x67, 0x8c, 0x04, 0x25, 0x01, 0x00, 0x00, 0x00
	db 0x36, 0x67, 0x8c, 0x04, 0x25, 0x01, 0x00, 0x00, 0xff
	mov ax,es
	mov ebx,fs
	mov [0x1704],gs
	pop gs

	jmp 0xc0:foo+0x400
	mov cx,0x1111
foo:
	mov bx,0x9999

	mov bl,0x71
	mov dh,0x34
