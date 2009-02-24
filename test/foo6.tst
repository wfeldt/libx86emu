[init srand=25]

0x20050-0x20053: rand

eax=rand
ebx=rand

ds=0x2000

[code start=0x100:0]

	mov ah,8
	rdtsc
	mov cr1,ebx
	mov dr6,ebx
	mov cr2,eax
	mov ecx,cr2
	mov edx,dr6



