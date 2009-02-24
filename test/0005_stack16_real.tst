[init srand=9 mode=real]

eax=rand

[code start=0x100:0 bits=16]

	push eax
	pop ebx
