
#define CINTERFACE 1
#include <Windows.h>

#include "Hooked_dinput8.h"
#include "Hooked_InitVoice.h"

#define DINPUT8_DLL "dinput8.dll"
#define OLD_DINPUT8_DLL ".\\dinput8_old.dll"
#define SYS_DINPUT8_DLL "%systemroot%\\System32\\dinput8.dll"

#define STR_DirectInput8Create "DirectInput8Create"

#define MAX_PATH_LEN 1024

static HMODULE dll = NULL;
static HINSTANCE hinstDLL;
static bool init = false;

using Call_Create = decltype(Hooked_DirectInput8Create)*;
static Call_Create ori_DirectInput8Create = nullptr;

long SVCALL Hooked_DirectInput8Create(void * hinst, unsigned dwVersion, void * riidltf, void ** ppvOut, void * punkOuter)
{
#if _DEBUG
	MessageBox(0, "Stop", "Stop", 0);
#endif // _DEBUG

	if (!dll) {
		dll = LoadLibraryA(OLD_DINPUT8_DLL);
		if (!dll) {
			char buff[MAX_PATH_LEN + 1];
			ExpandEnvironmentStringsA(SYS_DINPUT8_DLL, buff, sizeof(buff));
			dll = LoadLibraryA(buff);
		}

		if (dll) {
			ori_DirectInput8Create = (Call_Create)GetProcAddress(dll, STR_DirectInput8Create);
		}
	}
	
	if (!init) {
		init = InitEDVoice(hinstDLL);
	}

	if (ori_DirectInput8Create) {
		return ori_DirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);
	}
	else {
		return 0x80070057L;
	}
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		::hinstDLL = hinstDLL;
		break;
	default:
		break;
	}
	return TRUE;
}

