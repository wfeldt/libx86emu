[init srand=15]

0x20100-0x2010d: rand

eax=rand ebx=rand ecx=rand edx=rand esi=rand ebp=rand
edi=0x100
ds=0x2000 ss=0x3000

[code start=0x100:0x0]

	sub [di],cl
	pushf
	sub dl,ch
	pushf
	sub ch,[di+1]
	pushf
	db 0x2a, 0xf1		; sub dh,cl
	pushf

	sub [di+2],bx
	pushf
	sub bp,si
	pushf
	sub si,[di+4]
	pushf
	db 0x2b, 0xd3		; sub dx,bx
	pushf

	sub [di+6],ebx
	pushf
	sub ebp,esi
	pushf
	sub esi,[di+10]
	pushf
	db 0x66, 0x2b, 0xd3	; sub edx,ebx
	pushf

	sub al,0x97
	pushf
	sub ax,0x1257
	pushf
	sub eax,0x87493052
	pushf

