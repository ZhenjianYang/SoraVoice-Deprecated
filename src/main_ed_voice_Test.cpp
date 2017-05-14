#define CINTERFACE 1

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <memory>

#include "ed_voice.h"
#include "InitParam.h"
#include "Hook.h"
#include "INI.h"

#include <vorbis\vorbisfile.h>
#include <dsound.h>
#include <d3d8/d3dx8.h>

#include "Hooked_dinput8.h"

#include "SoraVoice.h"

using namespace std;

static HWND GetHwnd(void)
{
	char pszWindowTitle[1024];
	GetConsoleTitle(pszWindowTitle, 1024);
	return FindWindow(NULL, pszWindowTitle);
}

HRESULT WINAPI Fake_IDirect3DDevice8_Present(IDirect3DDevice8 * D3DD, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion) {
	return 0;
}

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	long SVCALL DirectInput8Create(void* hinst, unsigned dwVersion, void* riidltf, void **ppvOut, void *punkOuter);
	//long SVCALL DirectInput8Create(void* hinst, unsigned dwVersion, void* riidltf, void **ppvOut, void *punkOuter);

#ifdef __cplusplus
}
#endif // __cplusplus

int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);

	INI ini("SoraData.ini");
	ofstream ofs("NewSoraData.ini");
	for (int i = 0; i < ini.Num(); i++) {
		auto& group = ini.GetGroup(i);

		if(i != 0) ofs << "[" << group.Name() << "]" << endl;
		for (int j = 0; j < group.Num(); j++) {
			ofs << group.GetKey(j) << " = " << group.GetValue(j) << endl;
		}
	}
	ofs.close();

	InitParam p;
	memset(&p, 0, sizeof(p));

	bool ao = true;

	constexpr char STR_DLL_ZA[] = "za_voice.dll";
	constexpr char STR_DLL[] = "ed_voice.dll";
	constexpr char STR_Init[] = "Init";
	constexpr char STR_End[] = "End";
	constexpr char STR_Play[] = "Play";
	constexpr char STR_Stop[] = "Stop";


	HMODULE dll3 = LoadLibraryA("libvorbisfile.dll");
	decltype(ov_open_callbacks) *vf_ov_open_callbacks;
	decltype(ov_info) *vf_ov_info;
	decltype(ov_read) *vf_ov_read;
	decltype(ov_clear) *vf_ov_clear;
	if (dll3) {
		vf_ov_open_callbacks = (decltype(vf_ov_open_callbacks))GetProcAddress(dll3, "ov_open_callbacks");
		vf_ov_info = (decltype(vf_ov_info))GetProcAddress(dll3, "ov_info");;
		vf_ov_read = (decltype(vf_ov_read))GetProcAddress(dll3, "ov_read");;
		vf_ov_clear = (decltype(vf_ov_clear))GetProcAddress(dll3, "ov_clear");;
	}

	HMODULE dll = LoadLibrary(ao ? STR_DLL_ZA : STR_DLL);
	decltype(::Init)* Init = nullptr;
	decltype(::Stop)* Stop = nullptr;
	decltype(::End)* End = nullptr;
	decltype(::Play)* Play = nullptr;
	if (dll) {
		Init = (decltype(Init))GetProcAddress(dll, STR_Init);
		Stop = (decltype(Stop))GetProcAddress(dll, STR_Stop);
		End = (decltype(End))GetProcAddress(dll, STR_End);
		Play = (decltype(Play))GetProcAddress(dll, STR_Play);
	}

	if (!Init) return 0;
	if (!Stop) return 0;
	if (!End) return 0;
	if (!Play) return 0;

	//void* vf_ov_open_callbacks = ov_open_callbacks;
	//void* vf_ov_info = ov_info;
	//void* vf_ov_read = ov_read;
	//void* vf_ov_clear = ov_clear;

	GUID guidf = { 0xBF798030,0x483A,0x4DA2,0xAA,0x99,0x5D,0x64,0xED,0x36,0x97,0x00 };
	//void* pDI;
	//DirectInput8Create(GetModuleHandle(0), 0x800, &guidf, &pDI, 0);

	HWND hWnd = GetHwnd();
	p.addrs.p_Hwnd = (void**)&hWnd;

	LPDIRECTSOUND pDS = NULL;
	if (!ao) {
		DirectSoundCreate(NULL, &pDS, NULL);
		pDS->lpVtbl->SetCooperativeLevel(pDS, hWnd, DSSCL_PRIORITY);
		p.addrs.p_pDS = (void**)&pDS;
	}

	//p.addrs.p_pDS = (void**)&pDS;
	//p.addrs.p_ov_open_callbacks = &vf_ov_open_callbacks;
	//p.addrs.p_ov_info = &vf_ov_info;
	//p.addrs.p_ov_read = &vf_ov_read;
	//p.addrs.p_ov_clear = &vf_ov_clear;

	//DirectSoundCreate(NULL, &pDS, NULL);
	
	//pDS->lpVtbl->SetCooperativeLevel(pDS, hWnd, DSSCL_PRIORITY);
	

	IDirect3DDevice8 d3dd;
	IDirect3DDevice8Vtbl vtbl;
	d3dd.lpVtbl = &vtbl;
	memset(&vtbl, 0, sizeof(vtbl));
	vtbl.Present = Fake_IDirect3DDevice8_Present;

	Init(&p);
	InitHook_SetInitParam(&p);
	Hook_D3D_Present(&d3dd);

	for (int i = 0; i < 100; i++) {
		vtbl.Present(&d3dd, 0, 0, 0, 0);
	}

	vtbl.Present(&d3dd, 0, 0, 0, 0);
	string cmd;
	while ((cin >> cmd) && cmd != "exit")
	{
		Play((void *)cmd.c_str(), &p);
	}
	End(&p);
	return 0;
}
