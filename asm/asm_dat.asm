%include "macro_common"

%define tmp _tmp(5)
%define vs vs_dat

[BITS 32]
section dat vstart=vs
	push    eax
	push    ebx
	call    dat_start
dat_start:
	pop     ebx
	pop     eax
	sub     ebx, 7 + vs
	mov     dword [ebx + tmp], eax
	pop     eax

	cmp     eax, 1
	je      dat_call
	cmp     eax, 21
	jne     dat_call_ori
dat_call:
	mov     dword [ebp], edi
	push    ebp
	push    ecx
	call    dword [ebx + voice_dat]
	test    eax, eax
	jnz     dat_return

dat_call_ori:
	call    dword [ebx + to(jcs_dat)]

dat_return:
	push    dword [ebx + next(jcs_dat)]
	mov     ebx, dword [ebx + tmp]
	ret

%undef tmp
%undef vs
