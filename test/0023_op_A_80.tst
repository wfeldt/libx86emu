[init srand=23]

0x20000-0x20017: rand

ds=0x2000 ss=0x3000

[code start=0x100:0x0]

	mov al,0

l_10:

	mov cl,[si]
	lea si,[si+1]
m0:
	add cl,0x23
	pushf

	mov dh,[si]
	lea si,[si+1]
m1:
	add dh,-0x49
	pushf

m2:
	add byte[si],0x72
	lea si,[si+1]
	pushf

	add al,8
	add byte [cs:m0+1],8
	add byte [cs:m1+1],8
	add byte [cs:m2+1],8

	cmp al,0x40
	jb l_10

