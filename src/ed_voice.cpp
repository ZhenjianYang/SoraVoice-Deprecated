#include "ed_voice.h"

#include <Windows.h>

static void* hDll = nullptr;

int SVCALL Start() {
	return 0;
}
int SVCALL End() {
	return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	if (DLL_PROCESS_ATTACH == fdwReason) {
		::hDll = hinstDLL;
	}
	return TRUE;
}
