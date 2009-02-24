[init srand=19]

0x20100-0x20105: rand

eax=rand ebx=rand ecx=rand edx=rand esi=rand ebp=rand
edi=0x100
ds=0x2000 ss=0x3000

[code start=0x100:0x0]

	imul bx,cx,0x529a
	pushf

	imul dx,si,-0x4287
	pushf

	imul ax,[di],0x903
	pushf

	imul ebp,esi,0x79b20a61
	pushf

	imul esi,ebx,-0x2905d2e4
	pushf

	imul ecx,[di+2],-0x6b50305a
	pushf

