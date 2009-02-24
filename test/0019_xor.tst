[init srand=16]

0x20100-0x2010d: rand

eax=rand ebx=rand ecx=rand edx=rand esi=rand ebp=rand
edi=0x100
ds=0x2000 ss=0x3000

[code start=0x100:0x0]

	xor [di],cl
	pushf
	xor dl,ch
	pushf
	xor ch,[di+1]
	pushf
	db 0x32, 0xf1		; xor dh,cl
	pushf

	xor [di+2],bx
	pushf
	xor bp,si
	pushf
	xor si,[di+4]
	pushf
	db 0x33, 0xd3		; xor dx,bx
	pushf

	xor [di+6],ebx
	pushf
	xor ebp,esi
	pushf
	xor esi,[di+10]
	pushf
	db 0x66, 0x33, 0xd3	; xor edx,ebx
	pushf

	xor al,0x97
	pushf
	xor ax,0x1257
	pushf
	xor eax,0x87493052
	pushf

