[init]

ss.limit=0xffff

[code start=0x100:0x0 bits=16]

	push 0xAAAA
	push cs
	call func
	push 0xBBBB
	hlt

	func:
		retf 2
		nop
		nop
