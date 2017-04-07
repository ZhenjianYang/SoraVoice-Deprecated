%include "macro_common"

%ifdef sora_3rd
%include "macro_3rd"
%elifdef sora_sc
%include "macro_sc"
%else
%include "macro_fc"
%endif

[BITS 32]
section wait vstart=vs_wait

	cmp     byte [edx], 2
	jne     short return
	
	cmp     dword [ptr_count_ch], 0
	je      short return
	
	cmp     byte [ptr_flag_wait], 0
	jne      short return
	
	mov     byte [ptr_flag_wait], 1

return:
	jmp     addr_jmpto_wait
