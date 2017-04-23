#pragma once

class SoraVoice
{
public:
	virtual void Play(const char* v) = 0;
	virtual void Stop() = 0;
	virtual void Input() = 0;
	virtual void Show() = 0;

	virtual ~SoraVoice() {};

	static SoraVoice* CreateInstance(void* initParam);
	static void DestoryInstance(SoraVoice* sv);
};


