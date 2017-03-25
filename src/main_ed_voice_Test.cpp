#include <iostream>
#include <string>

#include "ed_voice.h"
#include <vorbis\vorbisfile.h>
#include <dsound.h>

using namespace std;

using VF_ov_open_callbacks = decltype(ov_open_callbacks);
using VF_ov_info = decltype(ov_info);
using VF_ov_read = decltype(ov_read);
using VF_ov_clear = decltype(ov_clear);

static struct {
	VF_ov_open_callbacks* ov_open_callbacks;
	VF_ov_info* ov_info;
	VF_ov_read* ov_read;
	VF_ov_clear* ov_clear;
} _vf;
static LPDIRECTSOUND _pDS = NULL;

struct InitParam
{
	bool _isAo = false;
	int reversed0 = 0;
	int reversed1 = 0;
	int reversed2 = 0;

	LPDIRECTSOUND* p_pDS = &_pDS;

	VF_ov_open_callbacks** p_ov_open_callbacks = &_vf.ov_open_callbacks;
	VF_ov_info** p_ov_info = &_vf.ov_info;
	VF_ov_read** p_ov_read = &_vf.ov_read;
	VF_ov_clear** p_ov_clear = &_vf.ov_clear;
} p;

static HWND GetHwnd(void)
{
	char pszWindowTitle[1024];
	GetConsoleTitle(pszWindowTitle, 1024);
	return FindWindow(NULL, pszWindowTitle);
}

int main(int argc, char* argv[])
{
	_vf.ov_open_callbacks = ov_open_callbacks;
	_vf.ov_info = ov_info;
	_vf.ov_read = ov_read;
	_vf.ov_clear = ov_clear;

	DirectSoundCreate(NULL, &_pDS, NULL);
	_pDS->SetCooperativeLevel(GetHwnd(), DSSCL_PRIORITY);

	Init(&p);

	string cmd;
	while ((cin >> cmd) && cmd != "exit")
	{
		Play((void *)cmd.c_str(), &p);
	}

	return 0;
}
