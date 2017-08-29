#include "SoraVoice.h"

#include <SVData.h>
#include <Message.h>
#include <Utils/Log.h>
#include <Utils/ApiPack.h>

#include <dsound.h>

SVData SV;

static HMODULE dsd_dll;
static HMODULE d3dx_dll;

using DSD = IDirectSound;
using CallDSCreate = decltype(DirectSoundCreate)*;
constexpr char STR_dsound_dll[] = "dsound.dll";
constexpr char STR_DirectSoundCreate[] = "DirectSoundCreate";

#define BIND(var, ptr) { if (ptr) (var) = decltype(var)(*(ptr)); \
						(ptr) = (decltype(ptr))&(var); }

constexpr char STR_vorbisfile_dll[] = "vorbisfile.dll";
constexpr char STR_libvorbisfile_dll[] = "libvorbisfile.dll";
constexpr char STR_ov_open_callbacks[] = "ov_open_callbacks";
constexpr char STR_ov_info[] = "ov_info";
constexpr char STR_ov_read[] = "ov_read";
constexpr char STR_ov_clear[] = "ov_clear";
constexpr char STR_ov_pcm_total[] = "ov_pcm_total";
constexpr const char* OvDllNames[] = { STR_vorbisfile_dll , STR_libvorbisfile_dll };
constexpr const char* DllDirs[] = { ".\\dll\\", ".\\voice\\dll\\" };

static void* d3dd;
static void* did;
static HWND hWnd;
static DSD* pDS;

constexpr char STR_d3dx_dll_za[] = "d3dx9_42.dll";
constexpr char STR_d3dx_dll_tits[] = "d3dx9_43.dll";
constexpr const char* STR_D3DX9_APIS[][2] = {
	{ "D3DXCreateFontIndirect", "D3DXCreateFontIndirectW" },
	{ "D3DXCreateSprite", "D3DXCreateSprite" }
};

int InitSVData()
{
	SV.status.ended = 1;

	LOG("p = 0x%08X", (unsigned)&SV);
	LOG("p->p_d3dd = 0x%08X", (unsigned)SV.addrs.p_d3dd);
	LOG("p->p_did = 0x%08X", (unsigned)SV.addrs.p_did);
	LOG("p->p_Hwnd = 0x%08X", (unsigned)SV.addrs.p_Hwnd);
	LOG("p->p_pDS = 0x%08X", (unsigned)SV.addrs.p_pDS);
	LOG("p->p_mute = 0x%08X", (unsigned)SV.addrs.p_mute);
	LOG("p->p_keys = 0x%08X", (unsigned)SV.addrs.p_keys);
	LOG("p->p_global = 0x%08X", (unsigned)SV.addrs.p_global);
	LOG("p->p_rnd_vlst = 0x%08X", (unsigned)SV.p_rnd_vlst);

	if (SV.series == SERIES_ZEROAO && SV.addrs.p_global) {
		if (SV.addrs.p_d3dd) SV.addrs.p_d3dd = (void**)((char*)*SV.addrs.p_global + (int)SV.addrs.p_d3dd);
		if (SV.addrs.p_did) SV.addrs.p_did = (void**)((char*)*SV.addrs.p_global + (int)SV.addrs.p_did);
		if (SV.addrs.p_Hwnd) SV.addrs.p_Hwnd = (void**)((char*)*SV.addrs.p_global + (int)SV.addrs.p_Hwnd);

		LOG("Adjuested p->p_d3dd = 0x%08X", (unsigned)SV.addrs.p_d3dd);
		LOG("Adjuested p->p_did = 0x%08X", (unsigned)SV.addrs.p_did);
		LOG("Adjuested p->p_Hwnd = 0x%08X", (unsigned)SV.addrs.p_Hwnd);
	}

	BIND(d3dd, SV.addrs.p_d3dd);
	BIND(did, SV.addrs.p_did);
	BIND(hWnd, SV.addrs.p_Hwnd);
	BIND(pDS, SV.addrs.p_pDS);

	LOG("d3dd = 0x%08X", (unsigned)d3dd);
	LOG("did = 0x%08X", (unsigned)did);
	LOG("Hwnd = 0x%08X", (unsigned)hWnd);
	LOG("pDS = 0x%08X", (unsigned)pDS);

	if (!pDS && SV.series == SERIES_ZEROAO) {
		LOG("pDS is nullptr, now going to creat DirectSoundDevice");
		if (!hWnd) {
			LOG("HWnd is nullptr, cound not DirectSoundDevice");
			return false;
		}
		dsd_dll = NULL;
		dsd_dll = LoadLibraryA(STR_dsound_dll);
		if (dsd_dll) {
			auto pDirectSoundCreate = (CallDSCreate)GetProcAddress(dsd_dll, STR_DirectSoundCreate);
			if (pDirectSoundCreate) {
				ApiPack::AddApi(STR_DirectSoundCreate, (void*)pDirectSoundCreate);
				pDirectSoundCreate(NULL, &pDS, NULL);
				if (pDS) {
					if (DS_OK != pDS->SetCooperativeLevel(hWnd, DSSCL_PRIORITY)) {
						pDS->Release();
						pDS = nullptr;
						LOG("Init DSD failed.");
					}
				}//if (pDS) 
			}//if (pDirectSoundCreate)
		}//if (dsd_dll) 

		LOG("new pDS = 0x%08X", (unsigned)pDS);
	}

	if (!pDS) {
		LOG("pDS is nullptr, failed to init SoraVoice");
		if (dsd_dll) {
			FreeLibrary(dsd_dll);
			dsd_dll = nullptr;
		}
		return false;
	}

	LOG("Now going to get d3dx Apis");

	if (SV.dxver == DX9) {
		d3dx_dll = NULL;
		d3dx_dll = LoadLibrary(SV.series == SERIES_ZEROAO ? STR_d3dx_dll_za : STR_d3dx_dll_tits);
		if (d3dx_dll) {
			for (auto api : STR_D3DX9_APIS) {
				void* ptrApi = (void*)GetProcAddress(d3dx_dll, api[1]);
				if (ptrApi) {
					ApiPack::AddApi(api[0], ptrApi);
					LOG("Api %s (Original:%s) loaded, address = 0x%08X", api[0], api[1], (unsigned)ptrApi);
				}
			}
		}//if (d3dx_dll) 
		else {
			LOG("Load %s failed.", SV.series == SERIES_ZEROAO ? STR_d3dx_dll_za : STR_d3dx_dll_tits);
		}
	}

	{
		LOG("Now going to load vorbisfile.dll ...");

		void* ov_open_callbacks = nullptr;
		void* ov_info = nullptr;
		void* ov_read = nullptr;
		void* ov_clear = nullptr;
		void* ov_pcm_total = nullptr;

		HMODULE ogg_dll = NULL;
		for (auto dir : DllDirs) {
			SetDllDirectoryA(dir);
			for (auto filename : OvDllNames) {
				ogg_dll = LoadLibraryA(filename);
				if (ogg_dll) break;
			}
		}
		SetDllDirectoryA(NULL);
		if (ogg_dll) {
			ov_open_callbacks = (void*)GetProcAddress(ogg_dll, STR_ov_open_callbacks);
			ov_info = (void*)GetProcAddress(ogg_dll, STR_ov_info);
			ov_read = (void*)GetProcAddress(ogg_dll, STR_ov_read);
			ov_clear = (void*)GetProcAddress(ogg_dll, STR_ov_clear);
			ov_pcm_total = (void*)GetProcAddress(ogg_dll, STR_ov_pcm_total);
		}

		LOG("Loaded ov_open_callbacks = 0x%08X", (unsigned)ov_open_callbacks);
		LOG("Loaded ov_info = 0x%08X", (unsigned)ov_info);
		LOG("Loaded ov_read = 0x%08X", (unsigned)ov_read);
		LOG("Loaded ov_clear = 0x%08X", (unsigned)ov_clear);
		LOG("Loaded ov_pcm_total = 0x%08X", (unsigned)ov_pcm_total);

		if (!ov_open_callbacks || !ov_info || !ov_read || !ov_clear || !ov_pcm_total) {
			LOG("Load ogg apis failed.");
			if (ogg_dll && SV.series == SERIES_ZEROAO) {
				FreeLibrary(ogg_dll);
				ogg_dll = nullptr;
			}
			if (dsd_dll) {
				FreeLibrary(dsd_dll);
				dsd_dll = nullptr;
			}
			return false;
		}
		else {
			ApiPack::AddApi(STR_ov_open_callbacks, ov_open_callbacks);
			ApiPack::AddApi(STR_ov_info, ov_info);
			ApiPack::AddApi(STR_ov_read, ov_read);
			ApiPack::AddApi(STR_ov_clear, ov_clear);
			ApiPack::AddApi(STR_ov_pcm_total, ov_pcm_total);
		}
	}

	Message.LoadMessage();

	SV.status.ended = 0;

	return 1;
}
