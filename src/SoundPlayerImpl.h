#undef CINTERFACE

#include "SoundPlayer.h"
#include "SoundFile.h"

#include "Log.h"
#include "Clock.h"

#include <dsound.h>

#include <cmath>
#include <string>
#include <thread>
#include <mutex>
#include <queue>


class SoundPlayerImpl : private SoundPlayer
{
	static constexpr int TIME_BUF = 500;
	static constexpr int NUM_BUF = 2;
	static constexpr int DELTA_TIME = 50;

	using SDWORD = long;
	static constexpr SDWORD INVALID_POS = 0x7FFFFFFF;

	using LockGuard = std::lock_guard<std::mutex>;
	using PlayInfo = struct { 
		PlayID playID; 
		FileType type;
		std::string fileNm;
	};
	using PlayQueue = std::queue<PlayInfo>;

	friend class SoundPlayer;

	static inline int TO_DSVOLUME(int volume) {
		return (volume) == 0 ?
			DSBVOLUME_MIN :
			(int)(2000 * std::log10(double(volume) / MaxVolume));
	}

private:
	virtual PlayID GetCurrentPlayID() const override {
		return playID;
	}

	virtual const char* GetCurrentFile() const override {
		return fileName.empty() ? nullptr : fileName.c_str();
	}

	virtual PlayID Play(const char* fileName, FileType type, int volume) override {
		PlayID playID = generatePlayID();
		{
			LockGuard lock(mt_playQueue);
			this->playQueue.push({ playID, type, fileName });
		}
		this->volume = volume;
		stop = false;
		SetEvent(hEvent_Playing);
		return playID;
	}

	virtual void Stop() override {
		stop = true;
	}

	virtual void SetVolume(int volume = MaxVolume) override {
		this->volume = volume;
		{
			LockGuard lock(mt_DSBuff);
			if(pDSBuff)
				pDSBuff->SetVolume(TO_DSVOLUME(volume));
		}
	};

	virtual void SetStopCallBack(StopCallBack stopCallBack = nullptr) override {
		LockGuard lock(mt_stopCallBack);
		this->stopCallBack = stopCallBack;
	}

	virtual StopCallBack GetStopCallBack() const override {
		LockGuard lock(mt_stopCallBack);
		return stopCallBack;
	}

	virtual ~SoundPlayerImpl() override {
		ended = true;
		Stop();
		for (auto sound : soundFiles) delete sound;
		{
			LockGuard lock(mt_playQueue);
			while (!playQueue.empty()) playQueue.pop();
		}
		SetEvent(hEvent_Playing);
		WaitForSingleObject(hEvent_End, DELTA_TIME * 3);
	}

	SoundPlayerImpl(void * pDSD,
			void * ov_open_callbacks, void * ov_info, void * ov_read, void * ov_clear,
			StopCallBack stopCallBack)
		:
		pDSD((decltype(this->pDSD))pDSD),
		stopCallBack(stopCallBack),
		hEvent_Playing(CreateEvent(NULL, FALSE, FALSE, NULL)),
		hEvent_End(CreateEvent(NULL, FALSE, FALSE, NULL)),
		th_playing(&SoundPlayerImpl::thread_Playing, this)
	{
		th_playing.detach();
		Ogg::SetOggApis(ov_open_callbacks, ov_info, ov_read, ov_clear);
	}
	PlayID playID = InvalidPlayID;
	std::string fileName;
	SoundFile* soundFile = nullptr;

	bool stop = false;
	bool ended = false;

	SoundFile* const soundFiles[2] =  { new Ogg, new Wav };

	IDirectSound* const pDSD;

	StopCallBack stopCallBack;
	mutable std::mutex mt_stopCallBack;

	int volume = MaxVolume;

	IDirectSoundBuffer *pDSBuff = nullptr;
	mutable std::mutex mt_DSBuff;
	int buffSize = 0;
	int buffIndex = 0;
	SDWORD endPos = MaxVolume;
	SDWORD curPos = 0;
	SDWORD prePos = 0;

	WAVEFORMATEX waveFormatEx {0};
	DSBUFFERDESC dSBufferDesc {0};

	PlayQueue playQueue;
	mutable std::mutex mt_playQueue;

	const HANDLE hEvent_Playing;
	const HANDLE hEvent_End;
	std::thread th_playing;

	void thread_Playing();

	bool openSoundFile(const std::string& fileName);
	bool initDSBuff();
	bool startPlay();

	/*
	 * return value:
	 *     0 finished
	 *     >0 remain bytes in buff (at least)
	 */
	int playing();

	void finishPlay();

	static PlayID generatePlayID() {
		static PlayID last = InvalidPlayID;
		return ++last == InvalidPlayID ? ++last : last;
	}
};
