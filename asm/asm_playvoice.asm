%ifdef sora_3rd
%include "macro_3rd"
%elifdef sora_sc
%include "macro_sc"
%else
%include "macro_fc"
%endif

[BITS 32]
section playvoice vstart=vs_playvoice
	push    eax
	push    ebx
	push    ecx
	push    edx
	mov     eax, edx
	mov     cl, byte [eax]
	cmp     cl, '#'
	jnz     short return
loop:
	inc     eax
	mov     cl, byte [eax]
	cmp     cl, '0'
	jb      short loopend
	cmp     cl, '9'
	jbe     short loop
loopend:
	cmp     cl, 'V'
	jnz     short return
	push    ptr_initparam
	push    edx
	call    dword [ptr_voice_play]
	nop
	nop
	nop
	nop
	nop
return:
	pop     edx
	pop     ecx
	pop     ebx
	pop     eax
	jmp     addr_jmpto_showtext
