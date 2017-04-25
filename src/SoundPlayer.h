#pragma once

class SoundPlayer
{
public:
	static constexpr int MaxVolume = 100;
	enum class Status
	{
		Playing,
		Stoped,
		Error,
		End
	};
	using StopCallBack = void (Status);

	static SoundPlayer* CreatSoundPlayer(
		void* pDSD, 
		void* ov_open_callbacks, void* ov_info, void* ov_read, void* ov_clear,
		StopCallBack* stopCallBack = nullptr);

	virtual void Play(const char* fileName, int volume) = 0;

	virtual void Stop() = 0;

	virtual void SetVolume(int volume = MaxVolume) = 0;

	virtual void SetStopCallBack(StopCallBack* stopCallBack = nullptr) = 0;

	virtual void Destory() = 0;

	Status GetStatus() const { return status; }
protected:
	virtual ~SoundPlayer() {}
	Status status;
};
