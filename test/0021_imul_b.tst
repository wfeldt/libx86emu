[init srand=18]

0x20100-0x20105: rand

eax=rand ebx=rand ecx=rand edx=rand esi=rand ebp=rand
edi=0x100
ds=0x2000 ss=0x3000

[code start=0x100:0x0]

	imul bx,cx,7
	pushf

	imul dx,si,-53
	pushf

	imul ax,[di],102
	pushf

	imul ebp,esi,38
	pushf

	imul esi,ebx,-49
	pushf

	imul ecx,[di+2],-71
	pushf

