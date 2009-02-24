[init]

eax=0
ebx=0x807f
ecx=0xff01
edx=0xfe02


[code start=0x100:0x0]

	cmp bh,cl
	jo l_o
	mov al,1
l_o:
	inc ah

	cmp cl,dl
	jno l_no
	mov al,1
l_no:
	inc ah

	cmp cl,dl
	jb l_b
	mov al,1
l_b:
	inc ah

	cmp dl,cl
	jnb l_nb1
	mov al,1
l_nb1:
	inc ah

	cmp dl,dl
	jnb l_nb2
	mov al,1
l_nb2:
	inc ah

	cmp cl,cl
	jz l_z
	mov al,1
l_z:
	inc ah

	cmp cl,dl
	jnz l_nz
	mov al,1
l_nz:
	inc ah

	cmp cl,dl
	jna l_na1
	mov al,1
l_na1:
	inc ah

	cmp cl,cl
	jna l_na2
	mov al,1
l_na2:
	inc ah

	cmp dl,cl
	ja l_a
	mov al,1
l_a:
	inc ah

	cmp cl,dl
	js l_s
	mov al,1
l_s:
	inc ah

	cmp dl,cl
	jns l_ns
	mov al,1
l_ns:
	inc ah

	cmp cl,cl
	jp l_p
	mov al,1
l_p:
	inc ah

	cmp dl,cl
	jnp l_np
	mov al,1
l_np:
	inc ah

	cmp cl,dl
	jl l_l1
	mov al,1
l_l1:
	inc ah

	cmp ch,cl
	jl l_l2
	mov al,1
l_l2:
	inc ah

	cmp dl,cl
	jnl l_nl1
	mov al,1
l_nl1:
	inc ah

	cmp dl,dh
	jnl l_nl2
	mov al,1
l_nl2:
	inc ah

	cmp cl,dl
	jng l_ng1
	mov al,1
l_ng1:
	inc ah

	cmp ch,cl
	jng l_ng2
	mov al,1
l_ng2:
	inc ah

	cmp ch,ch
	jng l_ng3
	mov al,1
l_ng3:
	inc ah

	cmp dl,cl
	jg l_g1
	mov al,1
l_g1:
	inc ah

	cmp cl,ch
	jg l_g2
	mov al,1
l_g2:
	inc ah


