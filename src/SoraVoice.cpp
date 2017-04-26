
#include "SoraVoice.h"
#include "SoraVoiceImpl.h"
#include "Log.h"

#include <dsound.h>
#ifdef ZA
#include <d3dx9.h>
#else
#include <d3d8/d3dx8.h>
#endif

#define GET_PTR(ptr) (ptr) ? *(ptr) : nullptr

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
constexpr char STR_D3DXCreateFontIndirect[] = "D3DXCreateFontIndirect";
#endif

constexpr char STR_dsound_dll[] = "dsound.dll";
constexpr char STR_DirectSoundCreate[] = "DirectSoundCreate";
using CallDSCreate = decltype(DirectSoundCreate)*;

SoraVoice * SoraVoice::CreateInstance(void * initParam)
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

	static void *t_ov_open_callbacks = GET_PTR(ip->addrs.p_ov_open_callbacks);
	static void *t_ov_info = GET_PTR(ip->addrs.p_ov_info);
	static void *t_ov_read = GET_PTR(ip->addrs.p_ov_read);
	static void *t_ov_clear = GET_PTR(ip->addrs.p_ov_clear);
	void *t_d3dd = GET_PTR(ip->addrs.p_d3dd);
	void *t_did = GET_PTR(ip->addrs.p_did);
	void *t_Hwnd = GET_PTR(ip->addrs.p_Hwnd);
	static void *t_pDS = GET_PTR(ip->addrs.p_pDS);

	LOG("ov_open_callbacks = 0x%08X", t_ov_open_callbacks);
	LOG("ov_info = 0x%08X", t_ov_info);
	LOG("ov_read = 0x%08X", t_ov_read);
	LOG("ov_clear = 0x%08X", t_ov_clear);
	LOG("d3dd = 0x%08X", t_d3dd);
	LOG("did = 0x%08X", t_did);
	LOG("Hwnd = 0x%08X", t_Hwnd);
	LOG("pDS = 0x%08X", t_pDS);

	if (!t_ov_open_callbacks || !t_ov_info || !t_ov_read || !t_ov_clear) {
		LOG("null ogg api exits, loading vorbisfile.dll ...");

		HMODULE ogg_dll = NULL;
		char buff[MAX_DLL_FULLPATH_LEN + 1];
		for (auto dir : DllDirs) for(auto filename : OvDllNames) {
			std::string path;
			path.append(dir).append(filename);
			GetFullPathName(path.c_str(), sizeof(buff), buff, NULL);

			ogg_dll = LoadLibrary(buff);
			if (ogg_dll) break;
		}
		if (ogg_dll) {
			t_ov_open_callbacks = GetProcAddress(ogg_dll, STR_ov_open_callbacks);
			t_ov_info = GetProcAddress(ogg_dll, STR_ov_info);
			t_ov_read = GetProcAddress(ogg_dll, STR_ov_read);
			t_ov_clear = GetProcAddress(ogg_dll, STR_ov_clear);
		}

		LOG("Loaded ov_open_callbacks = 0x%08X", t_ov_open_callbacks);
		LOG("Loaded ov_info = 0x%08X", t_ov_info);
		LOG("Loaded ov_read = 0x%08X", t_ov_read);
		LOG("Loaded ov_clear = 0x%08X", t_ov_clear);
	}

	if (!t_ov_open_callbacks || !t_ov_info || !t_ov_read || !t_ov_clear) {
		LOG("Load ogg apis failed.");
		return nullptr;
	}
#ifdef ZA
	if (!t_pDS) {
		LOG("pDS is nullpt, try to creat DirectSoundDevice");
		HMODULE ogg_dll = NULL;
		ogg_dll = LoadLibrary(STR_dsound_dll);
		if (ogg_dll) {
			auto pDirectSoundCreate = (CallDSCreate)GetProcAddress(ogg_dll, STR_DirectSoundCreate);
			if (pDirectSoundCreate) {
				pDirectSoundCreate(NULL, &t_pDS, NULL);
			}
		};
		return nullptr;
	}
#endif // ZA

	if (!t_pDS) {
		LOG("pDS is nullpt, failed to init SoraVoice");
		return nullptr;
	}

	Clock::InitClock(ip->rcd.now, ip->rcd.recent);
	return new SoraVoiceImpl((InitParam*)initParam);
}

void SoraVoice::DestoryInstance(SoraVoice * sv)
{
	delete sv;
}

