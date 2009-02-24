[init]

eax=0
ebx=0x807f
ecx=0xff01
edx=0xfe02


[code start=0x100:0x0]

	cmp bh,cl
	jo near l_o
	mov al,1
l_o:
	inc ah

	cmp cl,dl
	jno near l_no
	mov al,1
l_no:
	inc ah

	cmp cl,dl
	jb near l_b
	mov al,1
l_b:
	inc ah

	cmp dl,cl
	jnb near l_nb1
	mov al,1
l_nb1:
	inc ah

	cmp dl,dl
	jnb near l_nb2
	mov al,1
l_nb2:
	inc ah

	cmp cl,cl
	jz near l_z
	mov al,1
l_z:
	inc ah

	cmp cl,dl
	jnz near l_nz
	mov al,1
l_nz:
	inc ah

	cmp cl,dl
	jna near l_na1
	mov al,1
l_na1:
	inc ah

	cmp cl,cl
	jna near l_na2
	mov al,1
l_na2:
	inc ah

	cmp dl,cl
	ja near l_a
	mov al,1
l_a:
	inc ah

	cmp cl,dl
	js near l_s
	mov al,1
l_s:
	inc ah

	cmp dl,cl
	jns near l_ns
	mov al,1
l_ns:
	inc ah

	cmp cl,cl
	jp near l_p
	mov al,1
l_p:
	inc ah

	cmp dl,cl
	jnp near l_np
	mov al,1
l_np:
	inc ah

	cmp cl,dl
	jl near l_l1
	mov al,1
l_l1:
	inc ah

	cmp ch,cl
	jl near l_l2
	mov al,1
l_l2:
	inc ah

	cmp dl,cl
	jnl near l_nl1
	mov al,1
l_nl1:
	inc ah

	cmp dl,dh
	jnl near l_nl2
	mov al,1
l_nl2:
	inc ah

	cmp cl,dl
	jng near l_ng1
	mov al,1
l_ng1:
	inc ah

	cmp ch,cl
	jng near l_ng2
	mov al,1
l_ng2:
	inc ah

	cmp ch,ch
	jng near l_ng3
	mov al,1
l_ng3:
	inc ah

	cmp dl,cl
	jg near l_g1
	mov al,1
l_g1:
	inc ah

	cmp cl,ch
	jg near l_g2
	mov al,1
l_g2:
	inc ah


