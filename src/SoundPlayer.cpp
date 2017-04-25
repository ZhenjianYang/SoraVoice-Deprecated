#undef CINTERFACE

#include "SoundPlayer.h"

#include "Log.h"
#include "Clock.h"

#include <vorbis\vorbisfile.h>
#include <dsound.h>

#include <string>
#include <thread>
#include <mutex>
#include <queue>

class SoundPlayerImpl : private SoundPlayer
{
	static constexpr int TIME_BUF = 500;
	static constexpr int NUM_BUF = 2;
	static constexpr int DELTA_TIME = 50;

	static constexpr DWORD INVALID_POS = 0xFFFFFFFF;

	using LockGuard = std::lock_guard<std::mutex>;
	using PlayInfo = struct { PlayID playID; std::string fileNm;};
	using PlayQueue = std::queue<PlayInfo>;

	friend class SoundPlayer;

	static inline int TO_DSVOLUME(int volume) {
		return (volume) == 0 ?
			DSBVOLUME_MIN :
			(int)(2000 * log10(double(volume) / MaxVolume));
	}

private:
	virtual PlayID Play(const char* fileName, int volume) override {
		PlayID playID = generatePlayID();
		{
			LockGuard lock(mt_playQueue);
			this->playQueue.push({ playID, fileName });
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

	virtual ~SoundPlayerImpl() override {
		ended = true;
		Stop();
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
		ov_open_callbacks((decltype(this->ov_open_callbacks))ov_open_callbacks),
		ov_info((decltype(this->ov_info))ov_info),
		ov_read((decltype(this->ov_read))ov_read),
		ov_clear((decltype(this->ov_clear))ov_clear),
		stopCallBack(stopCallBack),
		hEvent_Playing(CreateEvent(NULL, FALSE, FALSE, NULL)),
		hEvent_End(CreateEvent(NULL, FALSE, FALSE, NULL)),
		th_playing(&SoundPlayerImpl::thread_Playing, this)
	{
		status = Status::Stoped;
		th_playing.detach();
	}

	bool stop = false;
	bool ended = false;

	IDirectSound* const pDSD;
	decltype(::ov_open_callbacks)* const ov_open_callbacks;
	decltype(::ov_info)* const ov_info;
	decltype(::ov_read)* const ov_read;
	decltype(::ov_clear)* const ov_clear;

	StopCallBack stopCallBack;
	std::mutex mt_stopCallBack;

	int volume = MaxVolume;

	IDirectSoundBuffer *pDSBuff = nullptr;
	std::mutex mt_DSBuff;
	unsigned buffSize = 0;
	unsigned buffIndex = 0;
	DWORD endPos = MaxVolume;
	DWORD curPos = 0;
	DWORD prePos = 0;

	OggVorbis_File ovFile {0};
	WAVEFORMATEX waveFormatEx {0};
	DSBUFFERDESC dSBufferDesc {0};

	PlayQueue playQueue;
	std::mutex mt_playQueue;

	const HANDLE hEvent_Playing;
	const HANDLE hEvent_End;
	std::thread th_playing;

	unsigned readOggData(void * buff, unsigned size)
	{
		if (!buff || !size) return 0;

		memset(buff, 0, size);

		int bitstream = 0;
		unsigned total = 0;

		constexpr int block = 4096;

		while (total < size)
		{
			int request = size - total < block ? size - total : block;
			int read = ov_read(&ovFile, (char*)buff + total, request, 0, 2, 1, &bitstream);
			if (read <= 0) return total;

			total += read;
		}
		return total;
	}

	void thread_Playing();

	bool SoundPlayerImpl::openSoundFile(const std::string& fileName);
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

SoundPlayer * SoundPlayer::CreatSoundPlayer(void * pDSD,
		void * ov_open_callbacks, void * ov_info, void * ov_read, void * ov_clear,
		StopCallBack stopCallBack)
{
	SoundPlayer* soundPlayer = new SoundPlayerImpl(pDSD,
			ov_open_callbacks, ov_info, ov_read, ov_clear,
			stopCallBack);
	return soundPlayer;
}

void SoundPlayer::DestorySoundPlayer(SoundPlayer * player)
{
	delete player;
}

void SoundPlayerImpl::thread_Playing()
{
	while (!ended)
	{
		DWORD waitResult = WaitForSingleObject(hEvent_Playing, INFINITE);
		if (waitResult != WAIT_OBJECT_0) continue;

		status = Status::Playing;

		PlayInfo play{ InvalidPlayID };
		{
			LockGuard lock(mt_playQueue);
			while (playQueue.size() > 1)
			{
				play = std::move(playQueue.front());
				playQueue.pop();
				LockGuard lock2(mt_stopCallBack);
				if (stopCallBack) {
					stopCallBack(play.playID, StopType::ForceStop);
				}
			}

			if (!playQueue.empty())
			{
				play = std::move(playQueue.front());
				playQueue.pop();
			}
		}

		if (play.playID != InvalidPlayID) {
			bool rst = openSoundFile(play.fileNm)
				&& initDSBuff()
				&& startPlay();

			if (rst) {
				int remain = 0;
				while (!stop && playQueue.empty() && (remain = playing())) {
					int delta = remain * waveFormatEx.nAvgBytesPerSec / Clock::TimeUnitsPerSecond;
					if (delta > DELTA_TIME) delta = DELTA_TIME;
					Clock::Sleep(delta);
				}
			}

			finishPlay();

			StopType stopType;
			if (!rst) {
				stopType = StopType::Error;
			}
			else if (stop || !playQueue.empty()) {
				stopType = StopType::ForceStop;
			}
			else {
				stopType = StopType::PlayEnd;
			}

			{
				LockGuard lock(mt_stopCallBack);
				if (stopCallBack) {
					stopCallBack(play.playID, stopType);
				}
			}
		} //if (play.playID != InvalidPlayID)

		status = Status::Stoped;
	} //while (!ended)
	SetEvent(hEvent_End);
}

bool SoundPlayerImpl::openSoundFile(const std::string& fileName){
	LOG("Open file %s ...", fileName.c_str());
	FILE* f = fopen(fileName.c_str(), "rb");
	if (f == NULL) {
		return 0;
	}

	if (ov_open_callbacks(f, &ovFile, nullptr, 0, OV_CALLBACKS_DEFAULT) != 0) {
		fclose(f);
		LOG("Open file as ogg failed!");
		return 0;
	}
	LOG("Ogg file opened");
	return 1;
}

bool SoundPlayerImpl::initDSBuff(){
	vorbis_info* info = ov_info(&ovFile, -1);

	memset(&waveFormatEx, 0, sizeof(waveFormatEx));
	waveFormatEx.wFormatTag = WAVE_FORMAT_PCM;
	waveFormatEx.nChannels = info->channels;
	waveFormatEx.nSamplesPerSec = info->rate;
	waveFormatEx.wBitsPerSample = 16;
	waveFormatEx.nBlockAlign = info->channels * 16 / 8;
	waveFormatEx.nAvgBytesPerSec = waveFormatEx.nSamplesPerSec * waveFormatEx.nBlockAlign;
	waveFormatEx.cbSize = 0;

	memset(&dSBufferDesc, 0, sizeof(dSBufferDesc));
	dSBufferDesc.dwSize = sizeof(dSBufferDesc);
	dSBufferDesc.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_CTRLVOLUME;
	dSBufferDesc.dwBufferBytes = waveFormatEx.nAvgBytesPerSec * TIME_BUF * NUM_BUF / Clock::TimeUnitsPerSecond;
	dSBufferDesc.dwReserved = 0;
	dSBufferDesc.lpwfxFormat = &waveFormatEx;
	dSBufferDesc.guid3DAlgorithm = GUID_NULL;

	buffSize = dSBufferDesc.dwBufferBytes / NUM_BUF;

	buffIndex = 0;
	endPos = INVALID_POS;
	curPos = 0;
	prePos = 0;

	if (DS_OK != pDSD->CreateSoundBuffer(&dSBufferDesc, &pDSBuff, NULL)) {
		LOG("Create sound buff failed!");
		return 0;
	}
	LOG("Sound buff opened");
	return 1;
}

bool SoundPlayerImpl::startPlay(){
	void *AP1, *AP2;
	DWORD AB1, AB2;
	if (DS_OK == pDSBuff->Lock(0, buffSize * (NUM_BUF - 1), &AP1, &AB1, &AP2, &AB2, 0)) {
		readOggData(AP1, AB1);
		if (AP2) readOggData(AP2, AB2);
		pDSBuff->Unlock(AP1, AB1, AP2, AB2);
	}
	else {
		pDSBuff->Release();
		LOG("Write first data failed!");
		return 0;
	}
	LOG("First data wroten");

	pDSBuff->SetVolume(TO_DSVOLUME(this->volume));
	pDSBuff->Play(0, 0, DSBPLAY_LOOPING);

	LOG("Playing...");
	return 1;
}

int SoundPlayerImpl::playing() {
	pDSBuff->GetCurrentPosition(&curPos, NULL);
	if(curPos < prePos) {
		buffIndex = 0;
		if(endPos != INVALID_POS) endPos -= dSBufferDesc.dwBufferBytes;
	}
	prePos = curPos;
	if(curPos > endPos) {
		return 0;
	}
	else if(curPos > buffIndex * buffSize) {
		const int buffReadIndex = (buffIndex + NUM_BUF - 1) % NUM_BUF;

		const int start = buffReadIndex * buffSize;
		const int size = buffReadIndex == NUM_BUF - 1 ?
			dSBufferDesc.dwBufferBytes - (NUM_BUF - 1) * buffSize
			: buffSize;

		void *AP1, *AP2;
		DWORD AB1, AB2;
		int read = 0;

		if (DS_OK == pDSBuff->Lock(start, size, &AP1, &AB1, &AP2, &AB2, 0)) {
			read = readOggData(AP1, AB1);
			if (AP2) read += readOggData(AP2, AB2);
			pDSBuff->Unlock(AP1, AB1, AP2, AB2);
		}

		if (endPos == INVALID_POS && read < size) {
			endPos = start + size;
			if (endPos < curPos) endPos += dSBufferDesc.dwBufferBytes;
		}
		buffIndex += 1;
	}

	return endPos == INVALID_POS ?
			dSBufferDesc.dwBufferBytes - curPos % buffSize
			: endPos - curPos;
}

void SoundPlayerImpl::finishPlay() {
	ov_clear(&ovFile);

	{
		LockGuard lock(mt_DSBuff);
		if (pDSBuff) {
			pDSBuff->Stop();
			pDSBuff->Release();
			pDSBuff = NULL;
		}
	}
}
