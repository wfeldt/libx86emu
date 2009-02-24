[init srand=24]

eax=rand edx=rand ebx=rand
ss=0x3000

[code start=0x100:0x0]

	mov ax,1234h
	cwd
	push dx
	
	mov edx,ebx
	mov ax,9876h
	cwd
	push dx

