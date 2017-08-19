#include "ed_voice.h"

#include <Init.h>
#include <Windows.h>

static void* hDll = nullptr;
static int last_rst = 0;

int SVCALL Start() {
	if (!last_rst) last_rst = StartSoraVoice(hDll);
	return last_rst;
}
int SVCALL End() {
	return EndSoraVoice();
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID)
{
	if (DLL_PROCESS_ATTACH == fdwReason) {
		::hDll = hinstDLL;
	}
	return TRUE;
}
