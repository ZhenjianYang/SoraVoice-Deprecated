#include "ed_voice.h"

#include <Init.h>
#include <Windows.h>

static void* hDll = nullptr;

int SVCALL Start() {
	return StartSoraVoice(hDll);
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
