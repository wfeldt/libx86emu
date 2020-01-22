; test 32 bit (unsigned) div
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
	div ecx
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
	div_u 57, 11
	div_u 3000, 2000
	div_u 1812363, 8627
	div_u 0xef542987, 23
	div_u 23, 0xef542987
        div_u 0x2587fe287d346534, 0x7f542987
        div_u 0x7587fe287d346534, 0x9239b842
	div_u 0x7fffffff, 1

	div_u 0x7fffffff000000, 0x1000000
        div_u 0x4587fe277d346534, 0x4587fe28
	div_u 0x80000000, 1
	div_u 0xffffffff, 1
	div_x 0, 0
	div_x 100, 0
        div_x 0x4587fe287d346534, 10
        div_x 0x4587fe287d346534, 0x4587fe28
table_end:
