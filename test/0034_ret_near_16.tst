[init]

ss.limit=0xffff

[code start=0x100:0x0 bits=16]

	push 0xAAAA
	call func
	push 0xBBBB
	hlt

	func:
		ret 2
		nop
		nop
