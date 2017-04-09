%include "macro_common"

%ifdef sora_3rd
%include "macro_3rd"
%elifdef sora_sc
%include "macro_sc"
%else
%include "macro_fc"
%endif

[BITS 32]
section scode vstart=vs_scode
	cmp     al, SCODE_TEXT
	je      record_code
	cmp     al, SCODE_SAY
	je      record_code
	cmp     al, SCODE_TALK
	je      record_code
	cmp     al, SCODE_MENU
	je      record_code
	jmp     return
	
record_code:
	mov     byte [ptr_flag_scode], al
	
return:
	push    edx
	mov     ecx, edi
	call    esi
	jmp     addr_clesi_scode + 5
