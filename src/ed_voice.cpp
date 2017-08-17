#include "ed_voice.h"

#include <Windows.h>

static void* hDll = nullptr;

void SVCALL Start(void*) {

}
void SVCALL End(void*) {

}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	if (DLL_PROCESS_ATTACH == fdwReason) {
		::hDll = hinstDLL;
	}
	return TRUE;
}
