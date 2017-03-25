#include <iostream>
#include <string>

#include "ed_voice.h"
#include "ed_voice_type.h"

#include <vorbis\vorbisfile.h>
#include <dsound.h>

using namespace std;

static VF _vf;
static LPDIRECTSOUND _pDS = NULL;

InitParam p;

static HWND GetHwnd(void)
{
	char pszWindowTitle[1024];
	GetConsoleTitle(pszWindowTitle, 1024);
	return FindWindow(NULL, pszWindowTitle);
}

int main(int argc, char* argv[])
{
	p.p_pDS = &_pDS;
	p.p_ov_open_callbacks = &_vf.ov_open_callbacks;
	p.p_ov_info = &_vf.ov_info;
	p.p_ov_read = &_vf.ov_read;
	p.p_ov_clear = &_vf.ov_clear;

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
