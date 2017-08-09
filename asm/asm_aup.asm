%include "macro_common"

%define tmp _tmp(3)
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

%ifdef za
	cmp     byte [ebx + status_ended], 0
	jne     short auto_play

	cmp     dword [ebx + ptr_sv], 0
	jne     short auto_play

	push    eax
	push    ecx
	push    edx
	push    ebx + ptr_initparam
	call    dword [ebx + voice_init]
	pop     edx
	pop     ecx
	pop     eax
%endif

auto_play:
%ifndef sora
	call    dword [ebx + to(jcs_aup)]
%endif

	cmp     byte [ebx + order_aup], 0
	je      short aup_return
	mov     byte [ebx + order_aup], 0

%ifndef sora
	or      al, 0x20
%else
	mov     eax, [ebx + addr_keys]
	mov     byte [eax + dik_space], dik_press
%endif

aup_return:
%ifndef sora
	push    dword [ebx + next(jcs_aup)]
%else
	push    dword [ebx + to(jcs_aup)]
%endif
	mov     ebx, dword [ebx + tmp]
	ret

%undef tmp
%undef vs
