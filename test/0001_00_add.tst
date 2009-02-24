[init]

esi=0x0000 edi=0xffff ebp=0xa000 ebx=0x5000
edx=0x4433 ecx=0x6655
ds=0x2000 ss=0x4000 es=0x6000

[code start=0x100:0x0]

	add [bx+si],cl
	add [bx+di],cl
	add [bp+si],cl
	add [bp+di],cl
	add [si],cl
	add [di],cl
	add [0x9000],cl
	add [bx],cl

	add [bx+si+0x40],ch
	add [bx+di+0x40],ch
	add [bp+si+0x40],ch
	add [bp+di+0x40],ch
	add [si+0x40],ch
	add [di+0x40],ch
	add [bp+0x40],ch
	add [bx+0x40],ch

	add [es:bx+si+0x800],dl
	add [es:bx+di+0x800],dl
	add [es:bp+si+0x800],dl
	add [es:bp+di+0x800],dl
	add [es:si+0x800],dl
	add [es:di+0x800],dl
	add [es:bp+0x800],dl
	add [es:bx+0x800],dl

	add al,dh
	add cl,dh
	add dl,dh
	add bl,dh
	add ah,dh
	add ch,dh
	add dh,dh
	add bh,dh

	add [ecx],al

