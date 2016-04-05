[init srand=16]

0x20000-0x203ff: rand
eax=0 ecx=0 edx=0 ebp=0
ds=0x2000 es=0x3000 ss=0xf000

[code start=0x100:0x0]

x1:
	add bp,0x8000	; alternately set OF flag
	mov dx,[si]
	rol dx,cl
	pushf
	mov [es:di],cx
	pop word [es:di + 2]
	mov dword [es:di + 4],edx
	add si,2
	add di,8
	inc cx
	cmp cx,200h
	jb x1


; with count = 1, OF flag is set
	mov cx,1
	xor bx,bx
	xor bp,bp
	xor si,si
x2:
	add bp,0x8000	; alternately set OF flag
	mov dx,[si]
	rol dx,cl
	pushf
	mov [es:di],cx
	pop word [es:di + 2]
	mov dword [es:di + 4],edx
	add si,2
	add di,8
	inc bx
	cmp bx,100h
	jb x2

