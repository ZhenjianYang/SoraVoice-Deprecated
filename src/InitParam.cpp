
#include "InitParam.h"
#include "Log.h"
#include "Clock.h"
#include "ApiPack.h"

#include <string>
#include <dsound.h>

using DSD = IDirectSound;

#define BIND(var, ptr) { if (ptr) (var) = decltype(var)(*(ptr)); \
						(ptr) = (decltype(ptr))&(var); }

constexpr char STR_vorbisfile_dll[] = "vorbisfile.dll";
constexpr char STR_libvorbisfile_dll[] = "libvorbisfile.dll";
constexpr char STR_ov_open_callbacks[] = "ov_open_callbacks";
constexpr char STR_ov_info[] = "ov_info";
constexpr char STR_ov_read[] = "ov_read";
constexpr char STR_ov_clear[] = "ov_clear";
constexpr char STR_ov_time_total[] = "ov_time_total";
constexpr const char* OvDllNames[] = { STR_vorbisfile_dll , STR_libvorbisfile_dll };
constexpr const char* DllDirs[] = { ".\\dll\\", ".\\voice\\dll\\" };

#ifdef ZA
constexpr char STR_d3dx_dll[] = "d3dx9_42.dll";
constexpr const char* STR_D3DX9_APIS[][2] = {
	{"D3DXCreateFontIndirect", "D3DXCreateFontIndirectW"},
	{"D3DXCreateSprite", "D3DXCreateSprite" }
};
#else
#include <d3d8\d3dx8.h>
constexpr char STR_D3DXCreateFontIndirect[] = "D3DXCreateFontIndirect";
#endif

constexpr char STR_dsound_dll[] = "dsound.dll";
constexpr char STR_DirectSoundCreate[] = "DirectSoundCreate";
using CallDSCreate = decltype(DirectSoundCreate)*;

static void* ov_open_callbacks;
static void* ov_info;
static void* ov_read;
static void* ov_clear;
static void* ov_time_total;
static void* d3dd;
static void* did;
static HWND hWnd;
static DSD* pDS;

static unsigned fake_mute = 1;

bool InitAddrs(InitParam* initParam)
{
	InitParam* ip = (InitParam*)initParam;

	LOG("p = 0x%08X", ip);
	LOG("p->p_ov_open_callbacks = 0x%08X", ip->addrs.p_ov_open_callbacks);
	LOG("p->p_ov_info = 0x%08X", ip->addrs.p_ov_info);
	LOG("p->p_ov_read = 0x%08X", ip->addrs.p_ov_read);
	LOG("p->p_ov_clear = 0x%08X", ip->addrs.p_ov_clear);
	LOG("p->p_d3dd = 0x%08X", ip->addrs.p_d3dd);
	LOG("p->p_did = 0x%08X", ip->addrs.p_did);
	LOG("p->p_Hwnd = 0x%08X", ip->addrs.p_Hwnd);
	LOG("p->p_pDS = 0x%08X", ip->addrs.p_pDS);
	LOG("p->p_mute = 0x%08X", ip->addrs.p_mute);
	LOG("p->p_keys = 0x%08X", ip->addrs.p_keys);
	LOG("p->p_global = 0x%08X", ip->addrs.p_global);

#ifdef ZA
	if (ip->addrs.p_global) {
		if (ip->addrs.p_d3dd) ip->addrs.p_d3dd = (void**)((char*)*ip->addrs.p_global + (int)ip->addrs.p_d3dd);
		if (ip->addrs.p_did) ip->addrs.p_did = (void**)((char*)*ip->addrs.p_global + (int)ip->addrs.p_did);
		if (ip->addrs.p_Hwnd) ip->addrs.p_Hwnd = (void**)((char*)*ip->addrs.p_global + (int)ip->addrs.p_Hwnd);

		LOG("Adjuested p->p_d3dd = 0x%08X", ip->addrs.p_d3dd);
		LOG("Adjuested p->p_did = 0x%08X", ip->addrs.p_did);
		LOG("Adjuested p->p_Hwnd = 0x%08X", ip->addrs.p_Hwnd);
	}
#endif // ZA

	BIND(ov_open_callbacks, ip->addrs.p_ov_open_callbacks);
	BIND(ov_info, ip->addrs.p_ov_info);
	BIND(ov_read, ip->addrs.p_ov_read);
	BIND(ov_clear, ip->addrs.p_ov_clear);

	BIND(d3dd, ip->addrs.p_d3dd);
	BIND(did, ip->addrs.p_did);
	BIND(hWnd, ip->addrs.p_Hwnd);
	BIND(pDS, ip->addrs.p_pDS);

	LOG("ov_open_callbacks = 0x%08X", ov_open_callbacks);
	LOG("ov_info = 0x%08X", ov_info);
	LOG("ov_read = 0x%08X", ov_read);
	LOG("ov_clear = 0x%08X", ov_clear);
	LOG("d3dd = 0x%08X", d3dd);
	LOG("did = 0x%08X", did);
	LOG("Hwnd = 0x%08X", hWnd);
	LOG("pDS = 0x%08X", pDS);

	//if (!ov_open_callbacks || !ov_info || !ov_read || !ov_clear) 
	{
		//LOG("null ogg api exists, now going to load vorbisfile.dll ...");
		LOG("Now going to load vorbisfile.dll ...");

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
			ov_time_total = (void*)GetProcAddress(ogg_dll, STR_ov_time_total);
		}

		LOG("Loaded ov_open_callbacks = 0x%08X", ov_open_callbacks);
		LOG("Loaded ov_info = 0x%08X", ov_info);
		LOG("Loaded ov_read = 0x%08X", ov_read);
		LOG("Loaded ov_clear = 0x%08X", ov_clear);
		LOG("Loaded ov_time_total = 0x%08X", ov_time_total);
	}

	if (!ov_open_callbacks || !ov_info || !ov_read || !ov_clear) {
		LOG("Load ogg apis failed.");
		return false;
	}
	else {
		ApiPack::AddApi(STR_ov_open_callbacks, ov_open_callbacks);
		ApiPack::AddApi(STR_ov_info, ov_info);
		ApiPack::AddApi(STR_ov_read, ov_read);
		ApiPack::AddApi(STR_ov_clear, ov_clear);
		ApiPack::AddApi(STR_ov_time_total, ov_time_total);
	}

	if (!pDS) {
		LOG("pDS is nullptr, now going to creat DirectSoundDevice");
		if (!hWnd) {
			LOG("HWnd is nullptr, cound not DirectSoundDevice");
			return false;
		}
		HMODULE dsd_dll = NULL;
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
						return false;
					}
				}//if (pDS) 
			}//if (pDirectSoundCreate)
		}//if (dsd_dll) 

		LOG("new pDS = 0x%08X", pDS);
	}

	if (!pDS) {
		LOG("pDS is nullptr, failed to init SoraVoice");
		return false;
	}

	LOG("Now going to get d3dx Apis");
#ifdef ZA
		HMODULE d3dx_dll = NULL;
		d3dx_dll = LoadLibrary(STR_d3dx_dll);
		if (d3dx_dll) {
			for (auto api : STR_D3DX9_APIS) {
				void* ptrApi = (void*)GetProcAddress(d3dx_dll, api[1]);
				if (ptrApi) {
					ApiPack::AddApi(api[0], ptrApi);
					LOG("Api %s (Original:%s) loaded, address = 0x%08X", api[0], api[1], ptrApi);
				}
			}
		}//if (d3dx_dll) 
		else {
			LOG("Load %s failed.", STR_d3dx_dll);
		}
#else
	ApiPack::AddApi(STR_D3DXCreateFontIndirect, &D3DXCreateFontIndirect);
	LOG("Api %s loaded, address = 0x%08X", STR_D3DXCreateFontIndirect, &D3DXCreateFontIndirect);
#endif

	if (!initParam->addrs.p_mute) initParam->addrs.p_mute = &fake_mute;

	Clock::InitClock(ip->rcd.now, ip->rcd.recent);

	return true;
}

