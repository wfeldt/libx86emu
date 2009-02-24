[init srand=29]

0x90020000-0x9002000f: rand

ecx=rand esi=rand edi=rand
ds=0x2000 es=0x4000
ds.limit=0xefffffff
es.limit=0xefffffff

[code start=0x100:0x0 bits=32]

	mov esi,0x90000000
	mov edi,0x90000050
	mov ecx,0x0f
	movsb
	rep movsb

