#include <iostream>
#include <string>

#include "ed_voice.h"
#include "InitParam.h"

#include <vorbis\vorbisfile.h>
#include <dsound.h>

#include <dinput.h>

#include "SoraVoice.h"

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
	memset(&p, 0, sizeof(p));

	void* vf_ov_open_callbacks = ov_open_callbacks;
	void* vf_ov_info = ov_info;
	void* vf_ov_read = ov_read;
	void* vf_ov_clear = ov_clear;

	LPDIRECTSOUND pDS = NULL;

	p.p_pDS = (void**)&pDS;
	p.p_ov_open_callbacks = &vf_ov_open_callbacks;
	p.p_ov_info = &vf_ov_info;
	p.p_ov_read = &vf_ov_read;
	p.p_ov_clear = &vf_ov_clear;

	DirectSoundCreate(NULL, &pDS, NULL);
	HWND hWnd = GetHwnd();
	pDS->SetCooperativeLevel(hWnd, DSSCL_PRIORITY);

	p.p_Hwnd = &hWnd;
	p.p_d3dd = &p.p_Hwnd;

	Init(&p);

	string cmd;
	while ((cin >> cmd) && cmd != "exit")
	{
		Play((void *)cmd.c_str(), &p);
	}
	End(&p);
	return 0;
}
