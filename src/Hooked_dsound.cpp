
#define CINTERFACE 1
#include <Windows.h>

#include "Hooked_dsound.h"
#include "Hooked_InitVoice.h"

#define DSOUND_DLL "dsound.dll"
#define OLD_DSOUND_DLL ".\\dsound_old.dll"
#define SYS_DSOUND_DLL "%systemroot%\\System32\\dsound.dll"

#define STR_DirectSoundCreate "DirectSoundCreate"

#define MAX_PATH_LEN 1024

static HMODULE dll = NULL;
static HINSTANCE hinstDLL;
static bool init = false;

using Call_Create = decltype(Hooked_DirectSoundCreate)*;
static Call_Create ori_DirectSoundCreate = nullptr;

long SVCALL Hooked_DirectSoundCreate(void* pcGuidDevice, void **ppDS, void* pUnkOuter)
{
#if _DEBUG
	MessageBox(0, "Stop", "Stop", 0);
#endif // _DEBUG

	if (!dll) {
		dll = LoadLibraryA(OLD_DSOUND_DLL);
		if (!dll) {
			char buff[MAX_PATH_LEN + 1];
			ExpandEnvironmentStringsA(SYS_DSOUND_DLL, buff, sizeof(buff));
			dll = LoadLibraryA(buff);
		}

		if (dll) {
			ori_DirectSoundCreate = (Call_Create)GetProcAddress(dll, STR_DirectSoundCreate);
		}
	}
	
	auto rst = ori_DirectSoundCreate ? ori_DirectSoundCreate(pcGuidDevice, ppDS, pUnkOuter) : 0x88780032;

	if (!init) {
		init = InitEDVoice(hinstDLL);
	}

	return rst;
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

