#include "ed_voice.h"
#include "SoraVoice.h"
#include "Log.h"
#include "HookD3d.h"

SVDECL void SVCALL Init(void *p)
{
	InitParam* ip = (InitParam*)p;
	if (!ip || ip->sv) return;

	SoraVoice *sv = new SoraVoice(ip);
	sv->Init();
	ip->sv = sv;
}

SVDECL void SVCALL End(void *p)
{
	InitParam* ip = (InitParam*)p;
	if (!ip || !ip->sv) return;

	((SoraVoice *)ip->sv)->End();
	delete ip->sv;
	ip->sv = nullptr;
}

SVDECL void SVCALL Play(void *v, void *p)
{
	InitParam* ip = (InitParam*)p;
	if (!v || !p || !ip->sv) return;

	((SoraVoice*)ip->sv)->Play((const char*)v);
}

SVDECL void SVCALL Stop(void *p)
{
	InitParam* ip = (InitParam*)p;
	if (!ip || !ip->sv) return;

	((SoraVoice *)ip->sv)->Stop();
}

SVDECL void SVCALL Input(void *p)
{
	InitParam* ip = (InitParam*)p;
	if (!ip || !ip->sv) return;

	((SoraVoice *)ip->sv)->Input();
}

SVDECL void* D3DCALL D3DCreate(void *p, unsigned SDKVersion)
{
	InitParam* ip = (InitParam*)p;
	if (ip) {
		InitHook(ip);
	}
	return HK_Direct3D_Create8(SDKVersion);
}
