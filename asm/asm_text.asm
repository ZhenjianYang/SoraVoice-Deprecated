%include "macro_common"

%ifdef sora_3rd
%include "macro_3rd"
%elifdef sora_sc
%include "macro_sc"
%else
%include "macro_fc"
%endif

[BITS 32]
section text vstart=vs_text
	push    eax
	
	mov     eax, edx 
	cmp     byte [eax], '#'
	jnz     short return
	
loop:
	inc     eax
	cmp     byte [eax], '0'
	jb      short loopend
	cmp     byte [eax], '9'
	jbe     short loop
	
loopend:
	cmp     byte [eax], 'V'
	jnz     short return
	
	push    ebx
	push    ecx
	push    edx
	push    ptr_initparam
	push    edx
	call    dword [ptr_voice_play]
	pop     edx
	pop     ecx
	pop     ebx

return:
	pop     eax
	jmp     addr_jmpto_text
