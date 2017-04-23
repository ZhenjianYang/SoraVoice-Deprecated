%include "macro_common"

%define tmp tmp3
%define vs vs_aup 

[BITS 32]
section input vstart=vs_aup
	push    eax
	push    ebx
	call    aup_start
aup_start:
	pop     ebx
	pop     eax
	mov     dword [ebx + tmp], eax
	sub     ebx, 7 + vs
	pop     eax

	cmp byte [ebx + status_ended], 0
	jne short aup_return

	cmp dword [ebx + ptr_sv], 0
	jne short auto_play

	push ecx
	push edx
	push    dword [ebx + ptr_initparam]
	call    dword [ebx + voice_init]
	pop edx
	pop ecx

auto_play:
	cmp byte [ebx + order_aup], 0
	je short aup_return
	mov byte [ebx + order_aup], 0

%ifdef ao
	call dword [ebx + to(jcs_aup)]
	mov eax, 1

aup_return:
	push    dword [ebx + next(jcs_aup)]
	mov     ebx, dword [ebx + tmp]
	ret
%else
	mov eax, [ebx + addr_keys]
	mov byte [eax + dik_space], dik_press

aup_return:
	push    dword [ebx + to(jcs_aup)]
	mov     ebx, dword [ebx + tmp]
	ret
%endif

times 0x100-($-$$) db 0

%undef tmp
%undef vs
