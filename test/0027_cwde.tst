[init srand=27]

eax=rand ebx=rand
ss=0x3000

[code start=0x100:0x0]

	mov ax,3568h
	cwde
	push eax
	
	mov eax,ebx
	mov ax,8175h
	cwde
	push eax

