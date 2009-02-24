[init srand=13]

0x20100-0x2010d: rand

eax=rand ebx=rand ecx=rand edx=rand esi=rand ebp=rand
edi=0x100
ds=0x2000 ss=0x3000

[code start=0x100:0x0]

	sbb [di],cl
	pushf
	sbb dl,ch
	pushf
	sbb ch,[di+1]
	pushf
	db 0x1a, 0xf1		; sbb dh,cl
	pushf

	sbb [di+2],bx
	pushf
	sbb bp,si
	pushf
	sbb si,[di+4]
	pushf
	db 0x1b, 0xd3		; sbb dx,bx
	pushf

	sbb [di+6],ebx
	pushf
	sbb ebp,esi
	pushf
	sbb esi,[di+10]
	pushf
	db 0x66, 0x1b, 0xd3	; sbb edx,ebx
	pushf

	sbb al,0x97
	pushf
	sbb ax,0x1257
	pushf
	sbb eax,0x87493052
	pushf

