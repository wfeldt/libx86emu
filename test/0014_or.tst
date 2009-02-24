[init srand=11]

0x20100-0x2010d: rand

eax=rand ebx=rand ecx=rand edx=rand esi=rand ebp=rand
edi=0x100
ds=0x2000 ss=0x3000

[code start=0x100:0x0]

	or [di],cl
	pushf
	or dl,ch
	pushf
	or ch,[di+1]
	pushf
	db 0x0a, 0xf1		; or dh,cl
	pushf

	or [di+2],bx
	pushf
	or bp,si
	pushf
	or si,[di+4]
	pushf
	db 0x0b, 0xd3		; or dx,bx
	pushf

	or [di+6],ebx
	pushf
	or ebp,esi
	pushf
	or esi,[di+10]
	pushf
	db 0x66, 0x0b, 0xd3	; or edx,ebx
	pushf

	or al,0x97
	pushf
	or ax,0x1257
	pushf
	or eax,0x87493052
	pushf

