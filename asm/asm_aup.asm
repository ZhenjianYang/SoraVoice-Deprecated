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
	sub     ebx, 7 + vs
	mov     dword [ebx + tmp], eax
	pop     eax

	cmp byte [ebx + status_ended], 0
	jne short aup_return

	cmp dword [ebx + ptr_sv], 0
	jne short auto_play

	push    eax
	push    ecx
	push    edx
	push    ebx + ptr_initparam
	call    dword [ebx + voice_init]
	pop     edx
	pop     ecx
	pop     eax

auto_play:
	cmp byte [ebx + order_aup], 0
	je short aup_return
	mov byte [ebx + order_aup], 0

%ifdef za
	call    dword [ebx + to(jcs_aup)]
	mov     eax, 1
	push    dword [ebx + next(jcs_aup)]
	mov     ebx, dword [ebx + tmp]
	ret

aup_return:
	call    dword [ebx + to(jcs_aup)]
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

%undef tmp
%undef vs
