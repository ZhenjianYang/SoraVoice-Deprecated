#include "SoundPlayer.h"

#include <vorbis\vorbisfile.h>
#include <dsound.h>

#include <string>
#include <thread>
#include <mutex>

static inline int TO_DSVOLUME(int volume, int Max_Volume) {
	return (volume) == 0 ?
		DSBVOLUME_MIN :
		(int)(2000 * log10(double(volume) / Max_Volume));
}

class SoundPlayerImpl : private SoundPlayer
{
	static constexpr int TIME_BUF = 1000;
	static constexpr int DELTA_TIME = 50;

	using LockGuard = std::lock_guard<std::mutex>;

	friend class SoundPlayer;

	static inline int TO_DSVOLUME(int volume) {
		return (volume) == 0 ?
			DSBVOLUME_MIN :
			(int)(2000 * log10(double(volume) / MaxVolume));
	}

private:
	virtual void Play(const char* fileName) override {
		Stop();
		{
			LockGuard lock(mt_fileName);
			this->fileName = fileName;
		}
		
		SetEvent(hEvent_Playing);
	}

	virtual void Stop() override {
		LockGuard lock(mt_DSBuff);
		status = Status::Stoped;
		pDSBuff->Stop();
	}

	virtual void SetVolume(int volume = MaxVolume) override {
		this->volume = volume;
		{
			LockGuard lock(mt_DSBuff);
			pDSBuff->SetVolume(TO_DSVOLUME(volume));
		}
	};

	virtual void SetStopCallBack(StopCallBack stopCallBack = nullptr) override {
		this->stopCallBack = stopCallBack;
	}

	virtual void Destory() override {
		Stop();
		status = Status::End;
		SetEvent(hEvent_Playing);
		WaitForSingleObject(hEvent_End, DELTA_TIME * 2);
	}

	SoundPlayerImpl(void * pDSD, void * ov_open_callbacks, void * ov_info, void * ov_read, void * ov_clear, StopCallBack* stopCallBack)
		:pDSD((decltype(this->pDSD))pDSD),
		ov_open_callbacks((decltype(this->ov_open_callbacks))ov_open_callbacks),
		ov_info((decltype(this->ov_info))ov_info),
		ov_read((decltype(this->ov_read))ov_read),
		ov_clear((decltype(this->ov_clear))ov_clear),
		stopCallBack(stopCallBack)
	{
		status = Status::Stoped;
		hEvent_Playing = CreateEvent(NULL, FALSE, FALSE, NULL);
		hEvent_Playing = CreateEvent(NULL, FALSE, FALSE, NULL);
		th_playing = std::thread(&SoundPlayerImpl::thread_Playing, this);
		th_playing.detach();
	}

	IDirectSound* const pDSD;
	decltype(::ov_open_callbacks)* const ov_open_callbacks;
	decltype(::ov_info)* const ov_info;
	decltype(::ov_read)* const ov_read;
	decltype(::ov_clear)* const ov_clear;
	StopCallBack* stopCallBack;

	int volume;
	IDirectSoundBuffer *pDSBuff;
	std::mutex mt_DSBuff;

	HANDLE hEvent_Playing;
	HANDLE hEvent_End;
	std::thread th_playing;

	std::string fileName;
	std::mutex mt_fileName;

	void thread_Playing();
};


SoundPlayer * SoundPlayer::CreatSoundPlayer(void * pDSD, void * ov_open_callbacks, void * ov_info, void * ov_read, void * ov_clear, StopCallBack* stopCallBack)
{
	SoundPlayerImpl* soundPlayer = new SoundPlayerImpl(pDSD, ov_open_callbacks, ov_info, ov_read, ov_clear, stopCallBack);
	return nullptr;
}

void SoundPlayerImpl::thread_Playing()
{
	while (status != Status::End)
	{
		DWORD rst = WaitForSingleObject(hEvent_Playing, INFINITE);
		if (rst == WAIT_OBJECT_0) {
			std::string fileName;
			{
				LockGuard lock(mt_fileName);
				fileName = this->fileName;
			}

			FILE* f = fopen(fileName.c_str(), "rb");
			if (f == NULL) {
				status = Status::Error;
				continue;
			}
		}
	}
	SetEvent(hEvent_End);
}
