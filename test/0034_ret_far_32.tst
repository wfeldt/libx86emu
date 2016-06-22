[init]

ss.limit=0xffffffff

[code start=0x100:0x0 bits=32]

	push 0xAAAAAAAA
	push cs
	call func
	push 0xBBBBBBBB
	hlt

	func:
		retf 4
		nop
		nop
