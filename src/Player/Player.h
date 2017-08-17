#pragma once

class Decoder;

namespace Player
{
	static constexpr int MaxVolume = 100;
	enum class Status {
		Playing,
		Stoped,
	};
	enum class StopType {
		PlayEnd,
		ForceStop,
		Error
	};
	using PlayID = unsigned;
	using StopCallBack = void (*) (PlayID playID, StopType stopType);

	static constexpr PlayID InvalidPlayID = 0;

	Status GetStatus();

	PlayID GetCurrentPlayID();
	const char* GetCurrentFile();

	bool Init(void* pDSD, StopCallBack stopCallBack = nullptr);
	bool End();

	PlayID Play(const char* fileName, int volume, Decoder* decoder = nullptr);
	void Stop();
	void SetVolume(int volume = MaxVolume);

	void SetStopCallBack(StopCallBack stopCallBack = nullptr);
	StopCallBack GetStopCallBack();
};
