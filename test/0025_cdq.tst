[init srand=25]

eax=rand edx=rand ebx=rand
ss=0x3000

[code start=0x100:0x0]

	mov eax,12345678h
	cdq
	push edx
	
	mov edx,ebx
	mov eax,98765432h
	cdq
	push edx

