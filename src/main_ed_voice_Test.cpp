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
#include "ini.h"

#include <vorbis\vorbisfile.h>
#include <dsound.h>
#include <d3d8/d3dx8.h>
#include <dinput.h>

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

int main(int argc, char* argv[])
{
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

	void* vf_ov_open_callbacks = ov_open_callbacks;
	void* vf_ov_info = ov_info;
	void* vf_ov_read = ov_read;
	void* vf_ov_clear = ov_clear;

	LPDIRECTSOUND pDS = NULL;

	p.addrs.p_pDS = (void**)&pDS;
	p.addrs.p_ov_open_callbacks = &vf_ov_open_callbacks;
	p.addrs.p_ov_info = &vf_ov_info;
	p.addrs.p_ov_read = &vf_ov_read;
	p.addrs.p_ov_clear = &vf_ov_clear;

	DirectSoundCreate(NULL, &pDS, NULL);
	HWND hWnd = GetHwnd();
	pDS->lpVtbl->SetCooperativeLevel(pDS, hWnd, DSSCL_PRIORITY);

	p.addrs.p_Hwnd = (void**)&hWnd;
	void* d3d = nullptr;
	p.addrs.p_d3dd = &d3d;

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
