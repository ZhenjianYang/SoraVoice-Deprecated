
#include "InitParam.h"
#include "Log.h"
#include "Clock.h"

#include <string>
#include <dsound.h>
using DSD = IDirectSound;

#ifndef ZA
#include <d3d8/d3dx8.h>
#endif // !ZA


#define BIND(var, ptr) { if (ptr) (var) = decltype(var)(*(ptr)); \
						(ptr) = (decltype(ptr))&(var); }

constexpr char STR_vorbisfile_dll[] = "vorbisfile.dll";
constexpr char STR_libvorbisfile_dll[] = "libvorbisfile.dll";
constexpr char STR_ov_open_callbacks[] = "ov_open_callbacks";
constexpr char STR_ov_info[] = "ov_info";
constexpr char STR_ov_read[] = "ov_read";
constexpr char STR_ov_clear[] = "ov_clear";
constexpr const char* OvDllNames[] = { STR_vorbisfile_dll , STR_libvorbisfile_dll };
constexpr const char* DllDirs[] = { ".\\", ".\\dll\\", ".\\voice\\dll\\" };
constexpr int DllDirsNum = std::extent<decltype(DllDirs)>::value;
constexpr int OvDllNamesNum = std::extent<decltype(OvDllNames)>::value;
constexpr int MAX_DLL_FULLPATH_LEN = 1024;

#ifdef ZA
constexpr char STR_d3dx_dll[] = "d3dx9_42.dll";
constexpr char STR_D3DXCreateFontIndirectA[] = "D3DXCreateFontIndirectA";
#endif

constexpr char STR_dsound_dll[] = "dsound.dll";
constexpr char STR_DirectSoundCreate[] = "DirectSoundCreate";
using CallDSCreate = decltype(DirectSoundCreate)*;

static void* ov_open_callbacks;
static void* ov_info;
static void* ov_read;
static void* ov_clear;
static void* d3dd;
static void* did;
static HWND hWnd;
static DSD* pDS;
static CallDSCreate d3DXCreateFontIndirect;

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
	LOG("p->p_D3DXCreateFontIndirect = 0x%08X", ip->addrs.p_D3DXCreateFontIndirect);

	BIND(ov_open_callbacks, ip->addrs.p_ov_open_callbacks);
	BIND(ov_info, ip->addrs.p_ov_info);
	BIND(ov_read, ip->addrs.p_ov_read);
	BIND(ov_clear, ip->addrs.p_ov_clear);

	BIND(d3dd, ip->addrs.p_d3dd);
	BIND(did, ip->addrs.p_did);
	BIND(hWnd, ip->addrs.p_Hwnd);
	BIND(pDS, ip->addrs.p_pDS);

	BIND(d3DXCreateFontIndirect, ip->addrs.p_D3DXCreateFontIndirect);

	LOG("ov_open_callbacks = 0x%08X", ov_open_callbacks);
	LOG("ov_info = 0x%08X", ov_info);
	LOG("ov_read = 0x%08X", ov_read);
	LOG("ov_clear = 0x%08X", ov_clear);
	LOG("d3dd = 0x%08X", d3dd);
	LOG("did = 0x%08X", did);
	LOG("Hwnd = 0x%08X", hWnd);
	LOG("pDS = 0x%08X", pDS);
	LOG("D3DXCreateFontIndirect = 0x%08X", d3DXCreateFontIndirect);

	if (!ov_open_callbacks || !ov_info || !ov_read || !ov_clear) {
		LOG("null ogg api exits, now going to load vorbisfile.dll ...");

		HMODULE ogg_dll = NULL;
		char buff[MAX_DLL_FULLPATH_LEN + 1];
		for (auto dir : DllDirs) for (auto filename : OvDllNames) {
			std::string path;
			path.append(dir).append(filename);
			GetFullPathName(path.c_str(), sizeof(buff), buff, NULL);

			ogg_dll = LoadLibrary(buff);
			if (ogg_dll) break;
		}
		if (ogg_dll) {
			ov_open_callbacks = (void*)GetProcAddress(ogg_dll, STR_ov_open_callbacks);
			ov_info = (void*)GetProcAddress(ogg_dll, STR_ov_info);
			ov_read = (void*)GetProcAddress(ogg_dll, STR_ov_read);
			ov_clear = (void*)GetProcAddress(ogg_dll, STR_ov_clear);
		}

		LOG("Loaded ov_open_callbacks = 0x%08X", ov_open_callbacks);
		LOG("Loaded ov_info = 0x%08X", ov_info);
		LOG("Loaded ov_read = 0x%08X", ov_read);
		LOG("Loaded ov_clear = 0x%08X", ov_clear);
	}

	if (!ov_open_callbacks || !ov_info || !ov_read || !ov_clear) {
		LOG("Load ogg apis failed.");
		return false;
	}

#ifdef ZA
	if (!pDS) {
		LOG("pDS is nullptr, now going to creat DirectSoundDevice");
		if (!hWnd) {
			LOG("HWnd is nullptr, cound not DirectSoundDevice");
			return false;
		}
		HMODULE dsd_dll = NULL;
		dsd_dll = LoadLibrary(STR_dsound_dll);
		if (dsd_dll) {
			auto pDirectSoundCreate = (CallDSCreate)GetProcAddress(dsd_dll, STR_DirectSoundCreate);
			if (pDirectSoundCreate) {
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
#endif // ZA
	if (!pDS) {
		LOG("pDS is nullptr, failed to init SoraVoice");
		return false;
	}

	if (!d3DXCreateFontIndirect) {
		LOG("D3DXCreateFontIndirect is nullptr, now going to get D3DXCreateFontIndirect");

#ifdef ZA
		HMODULE d3dx_dll = NULL;
		d3dx_dll = LoadLibrary(STR_d3dx_dll);
		if (d3dx_dll) {
			d3DXCreateFontIndirect = (CallDSCreate)GetProcAddress(d3dx_dll, STR_D3DXCreateFontIndirectA);
		}//if (d3dx_dll) 
#else
		d3DXCreateFontIndirect = &D3DXCreateFontIndirect;
#endif
		LOG("new D3DXCreateFontIndirect = 0x%08X", d3DXCreateFontIndirect);
	}

	if (!initParam->addrs.p_mute) initParam->addrs.p_mute = &fake_mute;

	Clock::InitClock(ip->rcd.now, ip->rcd.recent);

	return true;
}

