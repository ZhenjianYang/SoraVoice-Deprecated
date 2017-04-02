#include <iostream>
#include <string>

#include "ed_voice.h"
#include "ed_voice_type.h"

#include <vorbis\vorbisfile.h>
#include <dsound.h>

#include <dinput.h>

using namespace std;

static HWND GetHwnd(void)
{
	char pszWindowTitle[1024];
	GetConsoleTitle(pszWindowTitle, 1024);
	return FindWindow(NULL, pszWindowTitle);
}

int main(int argc, char* argv[])
{
	InitParam p;

	VF_ov_open_callbacks* vf_ov_open_callbacks = ov_open_callbacks;
	VF_ov_info* vf_ov_info = ov_info;
	VF_ov_read* vf_ov_read = ov_read;
	VF_ov_clear* vf_ov_clear = ov_clear;

	LPDIRECTSOUND pDS = NULL;

	p.p_pDS = &pDS;
	p.p_ov_open_callbacks = &vf_ov_open_callbacks;
	p.p_ov_info = &vf_ov_info;
	p.p_ov_read = &vf_ov_read;
	p.p_ov_clear = &vf_ov_clear;

	DirectSoundCreate(NULL, &pDS, NULL);
	HWND hWnd = GetHwnd();
	pDS->SetCooperativeLevel(hWnd, DSSCL_PRIORITY);

	Init(&p);

	string cmd;
	while ((cin >> cmd) && cmd != "exit")
	{
		Play((void *)cmd.c_str(), &p);
	}
	End(&p);
	return 0;
}
