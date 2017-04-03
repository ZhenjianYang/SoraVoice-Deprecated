%include "macro_common"

%ifdef sora_3rd
%include "macro_3rd"
%elifdef sora_sc
%include "macro_sc"
%else
%include "macro_fc"
%endif

[BITS 32]
section d3d vstart=vs_d3d
	pop eax
	push ptr_initparam
	push eax
	jmp [ptr_voice_d3d]

