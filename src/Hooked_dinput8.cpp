
#define CINTERFACE 1
#include <Windows.h>
#include <dinput.h>

#include "Hooked_dinput8.h"
#include "Hooked_dinput8_InitVoice.h"

#define DINPUT8_DLL "dinput8.dll"
#define OLD_DINPUT8_DLL ".\\dinput8_old.dll"
#define SYS_DINPUT8_DLL "%systemroot%\\System32\\dinput8.dll"

#define STR_DirectInput8Create "DirectInput8Create"

#define MAX_PATH_LEN 1024

HRESULT SVCALL Hooked_CreateDevice(IDirectInput8A* This, REFGUID rguid, LPDIRECTINPUTDEVICE8A * lplpDirectInputDevice, LPUNKNOWN pUnkOuter);

using Call_Create = decltype(Hooked_DirectInput8Create);
static Call_Create* ori_DirectInput8Create = nullptr;

using Call_CreateDevice = decltype(Hooked_CreateDevice);
static Call_CreateDevice* ori_CreateDevice = nullptr;

const GUID guid_keyboard = { 0x6F1D2B61, 0xD5A0, 0x11CF, 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 };

static HMODULE dll = 0;
static HINSTANCE hinstDLL;

HRESULT SVCALL Hooked_CreateDevice(IDirectInput8A* This, REFGUID rguid, LPDIRECTINPUTDEVICE8A * lplpDirectInputDevice, LPUNKNOWN pUnkOuter) {
	auto rst = ori_CreateDevice(This, rguid, lplpDirectInputDevice, pUnkOuter);

	if (rst == DI_OK && ip && !ip->addrs.p_did) {
		ip->addrs.p_did = (void**)lplpDirectInputDevice;
		Go();
	}

	return rst;
}

long SVCALL Hooked_DirectInput8Create(void * hinst, unsigned dwVersion, void * riidltf, void ** ppvOut, void * punkOuter)
{
	//MessageBox(NULL, NULL, "Stop", NULL);
	if (!dll) {
		dll = LoadLibraryA(OLD_DINPUT8_DLL);
		if (!dll) {
			char buff[MAX_PATH_LEN + 1];
			ExpandEnvironmentStringsA(SYS_DINPUT8_DLL, buff, sizeof(buff));
			dll = LoadLibraryA(buff);
		}
	}

	if (dll && !ori_DirectInput8Create) {
		ori_DirectInput8Create = (Call_Create*)GetProcAddress(dll, STR_DirectInput8Create);
	}

	if (ori_DirectInput8Create) {
		auto rst = ori_DirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);

		if (!ip) {
			Init(hinstDLL);
			if (ip) {
				if (ip->addrs.p_did) {
					Go();
				}
				else {
					if (rst == DI_OK && !ori_CreateDevice) {
						IDirectInput8 *pDirectInput = (decltype(pDirectInput))(*ppvOut);
						ori_CreateDevice = pDirectInput->lpVtbl->CreateDevice;

						DWORD tmp;
						if (VirtualProtect(&pDirectInput->lpVtbl->CreateDevice, 4, PAGE_EXECUTE_READWRITE, &tmp)) {
							ori_CreateDevice = pDirectInput->lpVtbl->CreateDevice;
							pDirectInput->lpVtbl->CreateDevice = Hooked_CreateDevice;
						}
					}
				}
			}
		}
		
		return rst;
	}
	else {
		return 0x80070057L;
	}
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	::hinstDLL = hinstDLL;
	return TRUE;
}

