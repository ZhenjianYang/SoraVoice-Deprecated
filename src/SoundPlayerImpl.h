#pragma once

#include "SoundPlayer.h"
#include "SoundFile.h"

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
		SoundFile* soundFile;
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

	virtual PlayID Play(const char* fileName, int volume, SoundFile* soundFile = nullptr) override {
		PlayID playID = generatePlayID();
		if(!soundFile) soundFile = getSoundFileByFileName(fileName);
		{
			LockGuard lock(mt_playQueue);
			this->playQueue.push({ playID, soundFile, fileName });
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
		{
			LockGuard lock(mt_playQueue);
			while (!playQueue.empty()) playQueue.pop();
		}
		SetEvent(hEvent_Playing);
		auto rst = WaitForSingleObject(hEvent_End, DELTA_TIME * 3);
		if (rst == WAIT_OBJECT_0) {
			delete ogg;
			delete wav;
		}
	}

	SoundPlayerImpl(void * pDSD, StopCallBack stopCallBack);

	PlayID playID = InvalidPlayID;
	std::string fileName;
	SoundFile* soundFile = nullptr;

	bool stop = false;
	bool ended = false;

	SoundFile* const ogg;
	SoundFile* const wav;

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

	WAVEFORMATEX waveFormatEx {};
	DSBUFFERDESC dSBufferDesc {};

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

	SoundFile* getSoundFileByFileName(const char* fileName) {
		constexpr int len_attr = sizeof(OggAttr) - 1;
		auto p = fileName;
		int len = 0;
		while(*p) { p++; len++; }
		if(len < len_attr) return wav;
		for(int i = 0; i < len_attr; i++) {
			if(*(p + i - len_attr) != OggAttr[i]) return wav;
		}
		return ogg;
	}

	static PlayID generatePlayID() {
		static PlayID last = InvalidPlayID;
		return ++last == InvalidPlayID ? ++last : last;
	}

	static constexpr char OggAttr[] = ".ogg";
	static constexpr char WavAttr[] = ".wav";
};
