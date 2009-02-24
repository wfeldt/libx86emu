[init srand=8 mode=real]

eax=rand

ss.limit=0xffffffff

[code start=0x100:0 bits=32]

	push eax
	pop ebx
