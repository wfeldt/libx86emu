[init srand=14]

0x20100-0x2010d: rand

eax=rand ebx=rand ecx=rand edx=rand esi=rand ebp=rand
edi=0x100
ds=0x2000 ss=0x3000

[code start=0x100:0x0]

	and [di],cl
	pushf
	and dl,ch
	pushf
	and ch,[di+1]
	pushf
	db 0x22, 0xf1		; and dh,cl
	pushf

	and [di+2],bx
	pushf
	and bp,si
	pushf
	and si,[di+4]
	pushf
	db 0x23, 0xd3		; and dx,bx
	pushf

	and [di+6],ebx
	pushf
	and ebp,esi
	pushf
	and esi,[di+10]
	pushf
	db 0x66, 0x23, 0xd3	; and edx,ebx
	pushf

	and al,0x97
	pushf
	and ax,0x1257
	pushf
	and eax,0x87493052
	pushf

