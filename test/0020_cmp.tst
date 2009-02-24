[init srand=17]

0x20100-0x2010d: rand

eax=rand ebx=rand ecx=rand edx=rand esi=rand ebp=rand
edi=0x100
ds=0x2000 ss=0x3000

[code start=0x100:0x0]

	cmp [di],cl
	pushf
	cmp dl,ch
	pushf
	cmp ch,[di+1]
	pushf
	db 0x3a, 0xf1		; cmp dh,cl
	pushf

	cmp [di+2],bx
	pushf
	cmp bp,si
	pushf
	cmp si,[di+4]
	pushf
	db 0x3b, 0xd3		; cmp dx,bx
	pushf

	cmp [di+6],ebx
	pushf
	cmp ebp,esi
	pushf
	cmp esi,[di+10]
	pushf
	db 0x66, 0x3b, 0xd3	; cmp edx,ebx
	pushf

	cmp al,0x97
	pushf
	cmp ax,0x1257
	pushf
	cmp eax,0x87493052
	pushf

