#pragma once

#include <functional>

class SoundPlayer
{
public:
	static constexpr int MaxVolume = 100;
	enum class Status
	{
		Playing,
		Stoped,
	};
	enum class StopType
	{
		PlayEnd,
		ForceStop,
		Error
	};
	using PlayID = unsigned;
	using StopCallBack = std::function<void(PlayID playID, StopType stopType)>;

	static constexpr PlayID InvalidPlayID = 0;

	static SoundPlayer* CreatSoundPlayer(
		void* pDSD, 
		void* ov_open_callbacks, void* ov_info, void* ov_read, void* ov_clear,
		StopCallBack stopCallBack = nullptr);
	static void DestorySoundPlayer(SoundPlayer* player);

	virtual PlayID Play(const char* fileName, int volume) = 0;

	virtual void Stop() = 0;

	virtual void SetVolume(int volume = MaxVolume) = 0;

	virtual void SetStopCallBack(StopCallBack stopCallBack = nullptr) = 0;

	Status GetStatus() const { return status; }
protected:
	virtual ~SoundPlayer() {}
	Status status;
};
