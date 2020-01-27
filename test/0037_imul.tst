; test 8, 16, 32 bit (signed) imul
;
; note: the number of successful tests is returned on the stack
;

[init mode=real]

idt.base=0x1000

[code start=0x0:0x2000]

%macro	imul_32 2
	dd %1
	dd %2 
	dq (%1) * (%2)
	dd 0
%endmacro

%macro	imul_16 2
	dd %1
	dd %2 
	dq ((%1) %% 0x10000) * ((%2) %% 0x10000)
	dd 0
%endmacro

%macro	imul_8 2
	dd %1
	dd %2 
	dq ((%1) %% 0x100) * ((%2) %% 0x100)
	dd 0
%endmacro

	xor edi,edi

	; 32 bit
	mov si,table_start_32
mul_10:
        xor eax,eax
	mov eax,[si]
	mov ecx,[si+4]
	imul ecx
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
	imul cx
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
	imul cl
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
	imul_32 0, 11
	imul_32 57, -11
	imul_32 0x3246, 73
	imul_32 0xa12373, 15
	imul_32 -0x80000000, 227
	imul_32 -0x80000000, -227
        imul_32 0x249dae82, 0x07431dbc
	imul_32 -0x5ebf2ff1, -0x2d208954
table_end_32:

table_start_16:
	imul_16 0, 12
	imul_16 79, 12
	imul_16 0x22a9, -21
	imul_16 0x7ae3, 0x2511
	imul_16 -0x8000, 876
	imul_16 -0x8000, -3327
	imul_16 0x3193, 0x49da
	imul_16 -0x4e07,-0x71fd
table_end_16:

table_start_8:
	imul_8 0, 17
	imul_8 57, 3
	imul_8 0x40, 2
	imul_8 -0x40, 2
	imul_8 -0x80, 76
	imul_8 -0x80, -37
	imul_16 0x33, 0x4a
	imul_16 -0x47,-0x71
table_end_8:
