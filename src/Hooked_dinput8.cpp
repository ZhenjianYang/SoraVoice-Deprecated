
#define CINTERFACE 1
#include <Windows.h>
#include <dinput.h>

#include "Hooked_dinput8.h"


HRESULT SVCALL Hooked_CreateDevice(IDirectInput8A* This, REFGUID rguid, LPDIRECTINPUTDEVICE8A * lplpDirectInputDevice, LPUNKNOWN pUnkOuter);
HRESULT SVCALL Hooked_GetDeviceState(IDirectInputDevice8A* This, DWORD cbData, LPVOID lpvData);


HMODULE dll = 0;

using Call_Create = decltype(Hooked_DirectInput8Create);
Call_Create* ori_DirectInput8Create = nullptr;

using Call_CreateDevice = decltype(Hooked_CreateDevice);
Call_CreateDevice* pCreateDevice = nullptr;

using Call_State = decltype(Hooked_GetDeviceState);
Call_State* pGetDeviceState = nullptr;

const GUID guid_keyboard = { 0x6F1D2B61, 0xD5A0, 0x11CF, 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 };

HRESULT SVCALL Hooked_CreateDevice(IDirectInput8A* This, REFGUID rguid, LPDIRECTINPUTDEVICE8A * lplpDirectInputDevice, LPUNKNOWN pUnkOuter) {
	auto rst = pCreateDevice(This, rguid, lplpDirectInputDevice, pUnkOuter);

	if (rst == DI_OK && !pGetDeviceState && rguid == guid_keyboard) {
		IDirectInputDevice8A* pDIDevice = *lplpDirectInputDevice;
		pGetDeviceState = pDIDevice->lpVtbl->GetDeviceState;

		DWORD tmp;
		if (VirtualProtect(&pDIDevice->lpVtbl->GetDeviceState, 4, PAGE_EXECUTE_READWRITE, &tmp))
			pDIDevice->lpVtbl->GetDeviceState = Hooked_GetDeviceState;
	}

	return rst;
}

HRESULT SVCALL Hooked_GetDeviceState(IDirectInputDevice8A * This, DWORD cbData, LPVOID lpvData)
{
	auto rst = pGetDeviceState(This, cbData, lpvData);

	if (rst == DI_OK && cbData == 0x100) {
		unsigned char* buff = (unsigned char*)lpvData;

		if (buff[DIK_Z]) buff[DIK_K] = 0x80;
	}

	return rst;
}

long SVCALL Hooked_DirectInput8Create(void * hinst, unsigned dwVersion, void * riidltf, void ** ppvOut, void * punkOuter)
{
	
	if (!dll) {
		dll = LoadLibrary("C:\\windows\\system32\\dinput8.dll");
	}

	if (dll && !ori_DirectInput8Create) {
		ori_DirectInput8Create = (Call_Create*)GetProcAddress(dll, "DirectInput8Create");
	}

	if (ori_DirectInput8Create) {
		MessageBox(NULL, "³É¹¦", NULL, 0);
		auto rst = ori_DirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);

		if (rst == DI_OK && !pCreateDevice) {
			IDirectInput8 *pDirectInput = (decltype(pDirectInput))(*ppvOut);
			pCreateDevice = pDirectInput->lpVtbl->CreateDevice;

			DWORD tmp;
			if(VirtualProtect(&pDirectInput->lpVtbl->CreateDevice, 4, PAGE_EXECUTE_READWRITE, &tmp))
				pDirectInput->lpVtbl->CreateDevice = Hooked_CreateDevice;
		}
		return rst;
	}
	else {
		MessageBox(NULL, "Ê§°Ü", NULL, 0);
		return 0x80070057L;
	}
}
