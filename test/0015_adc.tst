[init srand=12]

0x20100-0x2010d: rand

eax=rand ebx=rand ecx=rand edx=rand esi=rand ebp=rand
edi=0x100
ds=0x2000 ss=0x3000

[code start=0x100:0x0]

	adc [di],cl
	pushf
	adc dl,ch
	pushf
	adc ch,[di+1]
	pushf
	db 0x12, 0xf1		; adc dh,cl
	pushf

	adc [di+2],bx
	pushf
	adc bp,si
	pushf
	adc si,[di+4]
	pushf
	db 0x13, 0xd3		; adc dx,bx
	pushf

	adc [di+6],ebx
	pushf
	adc ebp,esi
	pushf
	adc esi,[di+10]
	pushf
	db 0x66, 0x13, 0xd3	; adc edx,ebx
	pushf

	adc al,0x97
	pushf
	adc ax,0x1257
	pushf
	adc eax,0x87493052
	pushf

