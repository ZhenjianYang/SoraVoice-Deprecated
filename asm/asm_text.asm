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
	jb      jcode

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

	push    ecx
	push    edx
	push    ptr_initparam
	push    edx
	call    dword [ptr_voice_play]
	pop     edx
	pop     ecx

return:
	pop     eax
	jmp     addr_jaeto_text

jcode:
	cmp     byte [edx], 5
	jnz     jcode2
	mov     byte [ptr_flag_code5], 1

jcode2:
	cmp     byte [edx], 2
	jne     short returnb
	cmp     dword [ptr_count_ch], 0
	je      short returnb
	mov     byte [ptr_flag_wait], 1

returnb:
	jmp     addr_jae_text + 6
	
	
	