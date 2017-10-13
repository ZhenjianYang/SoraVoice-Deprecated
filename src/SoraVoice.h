#pragma once

#ifndef SVCALL
#define SVCALL __stdcall
#endif

namespace SoraVoice
{
	void Play(const char* v);
	void Stop();
	void Input();
	void Show(void* pD3DD);

	bool Init();
	bool End();
};


