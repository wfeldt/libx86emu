[init srand=26]

eax=rand ebx=rand
ss=0x3000

[code start=0x100:0x0]

	mov al,34h
	cbw
	push ax
	
	mov ah,bh
	mov al,86h
	cbw
	push ax

