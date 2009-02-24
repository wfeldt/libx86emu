[init srand=10]

0x20100-0x2010d: rand

eax=rand ebx=rand ecx=rand edx=rand esi=rand ebp=rand
edi=0x100
ds=0x2000 ss=0x3000

[code start=0x100:0x0]

	add [di],cl
	pushf
	add dl,ch
	pushf
	add ch,[di+1]
	pushf
	db 0x02, 0xf1		; add dh,cl
	pushf

	add [di+2],bx
	pushf
	add bp,si
	pushf
	add si,[di+4]
	pushf
	db 0x03, 0xd3		; add dx,bx
	pushf

	add [di+6],ebx
	pushf
	add ebp,esi
	pushf
	add esi,[di+10]
	pushf
	db 0x66, 0x03, 0xd3	; add edx,ebx
	pushf

	add al,0x97
	pushf
	add ax,0x1257
	pushf
	add eax,0x87493052
	pushf

