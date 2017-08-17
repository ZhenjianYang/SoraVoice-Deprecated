#include <ed_voice.h>
#include <Windows.h>

#ifdef DINPUT8
#define DLL_NAME dinput8
#define API_NAME DirectInput8Create
#define HOOKED_API Hooked_DirectInput8Create
#define CALL_PARAM_DCL (void * hinst, unsigned dwVersion, void * riidltf, void ** ppvOut, void * punkOuter)
#define CALL_PARAM (hinst, dwVersion, riidltf, ppvOut, punkOuter)
#define ERR_CODE 0x80070057L
#elif defined(DSOUND)
#define DLL_NAME dsound
#define API_NAME DirectSoundCreate
#define HOOKED_API Hooked_DirectSoundCreate
#define CALL_PARAM_DCL (void* pcGuidDevice, void **ppDS, void* pUnkOuter)
#define CALL_PARAM (pcGuidDevice, ppDS, pUnkOuter)
#define ERR_CODE 0x88780032
#endif

#define _S(V) #V
#define S(V) _S(V)

#ifndef SVCALL
#define SVCALL __stdcall
#endif // !SVCALL

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
	long SVCALL HOOKED_API CALL_PARAM_DCL;

#ifdef __cplusplus
}
#endif // __cplusplus

#define OLD_NAME_DLL S(DLL_NAME) "_old.dll"
#define SYS_PATH_DLL "%systemroot%\\System32\\" S(DLL_NAME) ".dll"
#define STR_HOOKAPI_NAME S(API_NAME)

#define STR_ED_VOICE_DLL "voice/ed_voice.dll"
#define STR_START "Start"
#define STR_END "End"

#define MAX_PATH_LEN 512

static HMODULE dll = NULL;
static HMODULE dll_ed_voice = NULL;
static bool init = false;
static struct {
	decltype(::Start)* Start;
	decltype(::End)* End;
} ed_voice_apis;

using Call_Create = decltype(HOOKED_API)*;
static Call_Create ori_api = nullptr;

long SVCALL HOOKED_API CALL_PARAM_DCL
{
#if _DEBUG
	MessageBox(0, "Stop", "Stop", 0);
#endif // _DEBUG

	if (!dll) {
		dll = LoadLibraryA(OLD_NAME_DLL);
		if (!dll) {
			char buff[MAX_PATH_LEN + 1];
			ExpandEnvironmentStringsA(SYS_PATH_DLL, buff, sizeof(buff));
			dll = LoadLibraryA(buff);
		}

		if (dll) {
			ori_api = (Call_Create)GetProcAddress(dll, STR_HOOKAPI_NAME);
		}
	}
	
	auto rst = ori_api ? ori_api CALL_PARAM : ERR_CODE;

	if (!init) {
		init = true;
		dll_ed_voice = LoadLibraryA(STR_ED_VOICE_DLL);
		if (dll_ed_voice) {
			ed_voice_apis.Start = (decltype(ed_voice_apis.Start))GetProcAddress(dll_ed_voice, STR_START);
			ed_voice_apis.End = (decltype(ed_voice_apis.End))GetProcAddress(dll_ed_voice, STR_END);

			if (ed_voice_apis.Start) {
				if (!ed_voice_apis.Start()) {
					FreeLibrary(dll_ed_voice);
					ed_voice_apis.Start = ed_voice_apis.End = nullptr;
				}
			}
		}
	}

	return rst;
}

