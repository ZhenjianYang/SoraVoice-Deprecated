#pragma once

#ifndef SVCALL
#define SVCALL __stdcall
#endif

namespace SoraVoice
{
	void Play(const char* v);
	void Stop();
	void Input();
	void Show();

	bool Init();
	bool End();
};


