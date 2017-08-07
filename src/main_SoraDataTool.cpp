#define CINTERFACE

#include <Windows.h>

#include <d3d9/d3d9.h>
#include <dsound.h>
#include <dinput.h>

#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <unordered_map>

using namespace std;
#define ERROR_EXIT(condition) if(condition) { printf(#condition "\n"); return false; }

static HWND GetHwnd(void)
{
	char pszWindowTitle[1024];
	GetConsoleTitle(pszWindowTitle, 1024);
	return FindWindow(NULL, pszWindowTitle);
}

bool GetDSound();
bool GetDInput();
bool GetDX9();

int main(int argc, char* argv[])
{
	GetDSound();
	GetDInput();
	GetDX9();

	/////////////////////////////////////////////////////////////////////////





	return 0;
}

bool GetDSound() {
	HMODULE md_dsound = LoadLibraryA("dsound.dll");
	auto pDirectSoundCreate = (decltype(DirectSoundCreate)*)GetProcAddress(md_dsound, "DirectSoundCreate");

	ERROR_EXIT(!pDirectSoundCreate);
	IDirectSound* pDS;
	ERROR_EXIT(DS_OK != pDirectSoundCreate(NULL, &pDS, NULL));
	ERROR_EXIT(DS_OK != pDS->lpVtbl->SetCooperativeLevel(pDS, GetHwnd(), DSSCL_PRIORITY));

	WAVEFORMATEX waveFormatEx{};
	DSBUFFERDESC dSBufferDesc{};
	waveFormatEx.wFormatTag = 1;
	waveFormatEx.nChannels = 1;
	waveFormatEx.nSamplesPerSec = 48000;
	waveFormatEx.wBitsPerSample = 16;
	waveFormatEx.nBlockAlign = 2;
	waveFormatEx.nAvgBytesPerSec = waveFormatEx.nSamplesPerSec * waveFormatEx.wBitsPerSample / 8;
	waveFormatEx.cbSize = 0;
	dSBufferDesc.dwSize = sizeof(dSBufferDesc);
	dSBufferDesc.dwFlags = DSBCAPS_CTRLVOLUME;
	dSBufferDesc.dwBufferBytes = waveFormatEx.nAvgBytesPerSec;
	dSBufferDesc.dwReserved = 0;
	dSBufferDesc.lpwfxFormat = &waveFormatEx;
	dSBufferDesc.guid3DAlgorithm = {};
	IDirectSoundBuffer *pDSBuff;
	ERROR_EXIT(DS_OK != pDS->lpVtbl->CreateSoundBuffer(pDS, &dSBufferDesc, &pDSBuff, NULL));

	printf("DirectSoundCreate : dsound.dll+%08X\n", (unsigned)pDirectSoundCreate - (unsigned)md_dsound);
	printf("IDirectSoundBuffer::Play : dsound.dll+%08X\n", (unsigned)pDSBuff->lpVtbl->Play - (unsigned)md_dsound);
	printf("\n");

	pDSBuff->lpVtbl->Release(pDSBuff);
	pDS->lpVtbl->Release(pDS);

	return true;
}


bool GetDInput() {
	HMODULE md_dinput = LoadLibraryA("dinput8.dll");
	auto pDirectInput8Create = (decltype(DirectInput8Create)*)GetProcAddress(md_dinput, "DirectInput8Create");
	ERROR_EXIT(!pDirectInput8Create);

	const GUID guid_IID_IDirectInput8 = { 0xBF798030, 0x483A, 0x4DA2, 0xAA, 0x99, 0x5D, 0x64, 0xED, 0x36, 0x97, 0x00 };
	const GUID guid_SysKeyboard = { 0x6F1D2B61, 0xD5A0, 0x11CF, 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 };

	IDirectInput8* pDI;
	auto p = GetModuleHandle(NULL);
	ERROR_EXIT(DI_OK != pDirectInput8Create(p, DIRECTINPUT_VERSION, guid_IID_IDirectInput8, (void**)&pDI, NULL));

	IDirectInputDevice8* pDID;
	ERROR_EXIT(DI_OK != pDI->lpVtbl->CreateDevice(pDI, guid_SysKeyboard, &pDID, NULL));

	printf("IDirectInput8::CreateDevice : dinput.dll+%08X\n", (unsigned)pDI->lpVtbl->CreateDevice - (unsigned)md_dinput);
	printf("IDirectInputDevice8::GetDeviceState : dinput.dll+%08X\n", (unsigned)pDID->lpVtbl->GetDeviceState - (unsigned)md_dinput);
	printf("\n");

	pDID->lpVtbl->Release(pDID);
	pDI->lpVtbl->Release(pDI);

	return true;
}

bool GetDX9() {
	return true;
}
