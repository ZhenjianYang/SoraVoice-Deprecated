;Reference: https://github.com/Ouroboros/JuusanKoubou/tree/master/Source/Falcom/ED6FC%20Steam

%include "macro_common"

%define tmp _tmp(6)
%define vs vs_ldscnB

[BITS 32]
section ldscnB vstart=vs

%ifdef sora

	push    eax
	push    ebx
	call    ldscnB_start
ldscnB_start:
	pop     ebx
	pop     eax
	sub     ebx, 7 + vs
	mov     dword [ebx + tmp], eax
	pop     eax

	mov     edx, dword [esp + 8]
	mov     eax, edx
	shr     eax, 16

	cmp     eax, 1
	je      ldscnB_call
	cmp     eax, 0x21
	jne     ldscnB_call_ori
ldscnB_call:
	mov     ecx, dword [esp + 4]
	push    ecx
	mov     dword [ecx], 0
	and     edx, 0xFFFF
	mov     ecx, dword [ebx + addr_ppscn]
	mov     ecx, dword [eax * 4 + ecx]
	lea     eax, [edx * 8 + edx]
	lea     eax, [ecx + eax * 4]
	push    eax
	call    dword [ebx + voice_ldscn]
	test    eax, eax
	jz      ldscnB_call_ori

ldscnB_return:
	mov     ebx, dword [ebx + tmp]
	ret

ldscnB_call_ori:
	push    dword [ebx + to(jcs_ldscnB)]
	mov     ebx, dword [ebx + tmp]
	ret

%endif

%undef tmp
%undef vs
