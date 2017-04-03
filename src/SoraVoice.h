#pragma once

#include "InitParam.h"

class SoraVoice
{
private:
	const byte& isAo;
	byte& ended;

	InitParam::Status* const status;
	InitParam::Order* const order;

	struct Config* const config;
	struct Ogg* const ogg;
	struct DSD* const dsd;
	struct Thread* const th;
	struct InputData* const ipt;

private:
	void init();
	void destory();
	int readSoundData(char* buff, int size);
	void threadReadData();
	void playSoundFile();
	void stopPlaying();

public:
	SoraVoice(InitParam* initParam);
	~SoraVoice() { this->destory(); }

	void Play(const char* v);
	void Stop();
	void Input();
	void Show(void *D3DD);
};


