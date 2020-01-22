; test 32 bit (signed) idiv
;
; note: the number of successful tests is returned on the stack
;

[init mode=real]

idt.base=0x1000

[code start=0x0:0x2000]

; regular div
%macro	div_u 2
	dq %1
	dd %2
	dd %1 / %2
	dd %1 % %2
	dd 0
%endmacro

; regular idiv
%macro	div_s 2
	dq %1
	dd %2
	dd %1 // %2
	dd %1 %% %2
	dd 0
%endmacro

; (i)div with exception
%macro	div_x 2
	dq %1
	dd %2
	dd 0x44444444
	dd 0
	dd 0
%endmacro

	mov si,table_start
	xor edi,edi
div_10:
	mov eax,[si]
	mov edx,[si+4]
	mov ecx,[si+8]
	idiv ecx
	cmp eax,[si+12]
	jnz stop
	cmp edx,[si+16]
	jnz stop
	pushfd
	pop dword [si+20]
	inc edi
	add si,24
	cmp si,table_end
	jb div_10
stop:
	xor eax,eax
	push eax
	push edi
	hlt

interrupt_00:
	xor edx,edx
	mov eax,0x44444444
	mov ecx,1
	iret

	align 0x10, db 0
table_start:
	div_s 57, 11
	div_s 3000, 2000
	div_s 1812363, 8627
	div_s 0xef542987, 23
	div_s 23, 0xef542987
        div_s 0x2587fe287d346534, 0x7f542987
	div_s 0x7fffffff, 1
	div_s 0x7fffffff000000, 0x1000000

	div_s -3000, 2000
	div_s 1812363, -8627
	div_s 0xef542987, -23
	div_s -23, 0xef542987
        div_s -0x2587fe287d346534, 0x7f542987
        div_s 0x3587fe287d346534, -0x7f542987
	div_s 0x80000000, -1
	div_s 0x7fffffff, -1

	div_s -7, -5
	div_s -300, -200
	div_s -1812363, -8627
	div_s -0xef542987, -23
	div_s -5463, 0x506152cf
        div_s -0x2587fe287d346534, -0x7f542987
        div_s -0x317506152ccf9f28, -0x68716f59
	div_s -0x7fffffff, -1

	div_x 0x80000000, 1
	div_x -0x80000000, -1
	div_x 0, 0
	div_x 100, 0
	div_x -100, 0
        div_x 0x4587fe287d346534, -10
        div_x 0x4587fe287d346534, 10
        div_x 0x4587fe287d346534, 0x7f542987
table_end:
