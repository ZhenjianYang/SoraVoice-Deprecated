#include "asm.h"
#include "inline_asm.h"
#include "Calls.h"
#include <SVData.h>

static unsigned tmp_path;
static unsigned tmp_ret;

NACKED void ASM::scnp() {
	INLINE_ASM {
		pop     eax;
		mov     dword ptr[tmp_ret], eax;
		mov     eax, dword ptr[esp];
		mov     dword ptr[tmp_path], eax;

		call    dword ptr[SV.jcs.scnp.to];

		push    eax;
		push    dword ptr[tmp_path];
		call    ASM_RdScnPath;
		pop     eax;

		jmp     dword ptr[tmp_ret]
	}
}
