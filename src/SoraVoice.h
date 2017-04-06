#pragma once

struct InitParam;

class SoraVoice
{
public:
	virtual void Play(const char* v) = 0;
	virtual void Stop() = 0;
	virtual void Input() = 0;
	virtual void Show() = 0;

	virtual ~SoraVoice() = 0;

	static SoraVoice* CreateInstance(InitParam* initParam);
	static void DestoryInstance(SoraVoice* sv);
};


