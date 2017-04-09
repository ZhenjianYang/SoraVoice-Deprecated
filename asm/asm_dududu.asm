%include "macro_common"

%ifdef sora_3rd
%include "macro_3rd"
%elifdef sora_sc
%include "macro_sc"
%else
%include "macro_fc"
%endif

[BITS 32]
section dududu vstart=vs_dududu
	cmp     byte [ptr_flag_code5], 0
	je      code5end
	push    ptr_initparam
	call    dword [ptr_voice_stop]
	
code5end:
	cmp     byte [ptr_flag_scode], SCODE_TEXT
	je      count
	cmp     byte [ptr_flag_scode], SCODE_SAY
	je      count
	cmp     byte [ptr_flag_scode], SCODE_TALK
	je      count
	jmp     disabledududu
	
count:
	cmp     dword [ptr_count_ch], 0
	je      first
	
	mov     eax, dword [ptr_count_ch]
	inc     eax
	mov     dword [ptr_count_ch], eax
	jmp     disabledududu
	
first:
	mov     eax, dword [ptr_now]
	mov     dword [ptr_time_textbeg], eax
	mov     dword [ptr_count_ch], 1

disabledududu:
	cmp     byte [ptr_flag_disable_du], 0
	je      short return
	
	cmp     byte [ptr_mute], 0
	jne     short return

	mov     byte [ptr_mute], 1
	call    addr_dududu
	mov     byte [ptr_mute], 0

	jmp     addr_call_dududu+5

return:
	call    addr_dududu
	jmp     addr_call_dududu+5
