[init srand=28]

0x20000-0x2000f: rand

ecx=rand esi=rand edi=rand
ds=0x2000 es=0x4000

[code start=0x100:0x0]

	mov si,0
	mov di,0x50
	mov cx,0x0f
	movsb
	rep movsb

