; test 8, 16, 32 bit (unsigned) mul
;
; note: the number of successful tests is returned on the stack
;

[init mode=real]

idt.base=0x1000

[code start=0x0:0x2000]

%macro	mul_32 2
	dd %1
	dd %2 
	dq (%1) * (%2)
	dd 0
%endmacro

%macro	mul_16 2
	dd %1
	dd %2 
	dq ((%1) % 0x10000) * ((%2) % 0x10000)
	dd 0
%endmacro

%macro	mul_8 2
	dd %1
	dd %2 
	dq ((%1) % 0x100) * ((%2) % 0x100)
	dd 0
%endmacro

	xor edi,edi

	; 32 bit
	mov si,table_start_32
mul_10:
        xor eax,eax
	mov eax,[si]
	mov ecx,[si+4]
	mul ecx
	cmp eax,[si+8]
	jnz stop
	cmp edx,[si+12]
	jnz stop
	pushfd
	pop dword [si+16]
	inc edi
	add si,20
	cmp si,table_end_32
	jb mul_10

	; 16 bit
	mov si,table_start_16
mul_20:
        xor eax,eax
	mov eax,[si]
	mov ecx,[si+4]
	mul cx
	cmp ax,[si+8]
	jnz stop
	cmp dx,[si+10]
	jnz stop
	pushfd
	pop dword [si+16]
	inc edi
	add si,20
	cmp si,table_end_16
	jb mul_20

	; 8 bit
	mov si,table_start_8
mul_30:
        xor eax,eax
	mov eax,[si]
	mov ecx,[si+4]
	mul cl
	cmp al,[si+8]
	jnz stop
	cmp ah,[si+9]
	jnz stop
	pushfd
	pop dword [si+16]
	inc edi
	add si,20
	cmp si,table_end_8
	jb mul_30


stop:
	push edi
	hlt

	align 0x10, db 0

table_start_32:
	mul_32 0, 11
	mul_32 57, 11
	mul_32 0x9246, 73
	mul_32 0xa12373, 15
	mul_32 0x80000000, 227
	mul_32 0xffffffff, 227
        mul_32 0xe49dae82, 0xf7431dbc
	mul_32 0x2ebf2ff1, 0xed208954
table_end_32:

table_start_16:
	mul_16 0, 12
	mul_16 79, 12
	mul_16 0x22a9, 21
	mul_16 0xbae3, 0x2511
	mul_16 0x8000, 876
	mul_16 0xffff, 3327
	mul_16 0xf193, 0xa9da
	mul_16 0x9e07, 0x71fd
table_end_16:

table_start_8:
	mul_8 0, 17
	mul_8 57, 3
	mul_8 0x40, 2
	mul_8 0x80, 0x80
	mul_8 0x80, 76
	mul_8 0xff, 37
	mul_16 0x93, 0x4a
	mul_16 0xf7, 0xe1
table_end_8:
