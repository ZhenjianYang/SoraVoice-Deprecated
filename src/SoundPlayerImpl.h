#undef CINTERFACE

#include "SoundPlayer.h"
#include "SoundFile.h"

#include "Log.h"
#include "Clock.h"
#include "ApiPack.h"

#include <dsound.h>

#include <cmath>
#include <string>
#include <thread>
#include <mutex>
#include <queue>

#include <vorbis\vorbisfile.h>

constexpr char STR_ov_open_callbacks[] = "ov_open_callbacks";
constexpr char STR_ov_info[] = "ov_info";
constexpr char STR_ov_read[] = "ov_read";
constexpr char STR_ov_clear[] = "ov_clear";

class Ogg : public SoundFile {
public:
	Ogg() = default;
	~Ogg() { Close(); }
	virtual bool Open(const char* fileName) override {
		FILE* f = fopen(fileName, "rb");
		if (f == NULL) {
			return false;
		}

		if (ov_open_callbacks(f, &ovFile, nullptr, 0, OV_CALLBACKS_DEFAULT) != 0) {
			fclose(f);
			return false;
		}

		vorbis_info* info = ov_info(&ovFile, -1);

		waveFormat.wFormatTag = 1;
		waveFormat.nChannels = info->channels;
		waveFormat.nSamplesPerSec = info->rate;
		waveFormat.wBitsPerSample = 16;
		waveFormat.nBlockAlign = info->channels * 16 / 8;
		waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;

		return true;
	}
	virtual int Read(void * buff, int size) override {
		if (!buff || !size) return 0;

		int bitstream = 0;
		int total = 0;
		constexpr int block = 4096;
		while (total < size)
		{
			int request = size - total < block ? size - total : block;
			int read = ov_read(&ovFile, (char*)buff + total, request, 0, 2, 1, &bitstream);
			if (read <= 0) break;
			else total += read;
		}
		for (char* p = (char*)buff + total; p < (char*)buff + size; p++) *p = 0;
		return total;
	}
	virtual void Close() override {
		ov_clear(&ovFile);
	}

private:
	OggVorbis_File ovFile{};
	Ogg(const Ogg&) = delete;
	Ogg& operator=(const Ogg&) = delete;

public:
	static void SetOggApis(void * ov_open_callbacks,
		void * ov_info, void * ov_read, void * ov_clear) {
		Ogg::ov_open_callbacks = (decltype(Ogg::ov_open_callbacks))ov_open_callbacks;
		Ogg::ov_info           = (decltype(Ogg::ov_info))          ov_info;
		Ogg::ov_read           = (decltype(Ogg::ov_read))          ov_read;
		Ogg::ov_clear          = (decltype(Ogg::ov_clear))         ov_clear;
	}
private:
	static decltype(::ov_open_callbacks)* ov_open_callbacks;
	static decltype(::ov_info)* ov_info;
	static decltype(::ov_read)* ov_read;
	static decltype(::ov_clear)* ov_clear;
};

class Wav : public SoundFile {
	static constexpr u32 tag_RIFF = 0x46464952;
	static constexpr u32 tag_WAVE = 0x45564157;
	static constexpr u32 tag_fmt  = 0x20746D66;
	static constexpr u32 tag_data = 0x61746164;

	struct WAVHead
	{
		u32 tag_RIFF;
		u32 size;
		u32 tag_WAVE;
		u32 tag_fmt;
		u32 size_WAVEFORMAT;
		WAVEFORMAT waveFormat;
		u32 tag_data;
		s32 size_data;
	} head{};
	int remain = 0;

public:
	Wav() = default;
	~Wav() { Close(); }
	virtual bool Open(const char* fileName) override {
		f = fopen(fileName, "rb");
		if (f == NULL) {
			return false;
		}
		if (fread(&head, 1, sizeof(head), f) != sizeof(head)) {
			fclose(f); f = NULL; return false;
		}

		if (head.tag_RIFF != tag_RIFF || head.tag_WAVE != tag_WAVE || head.tag_fmt != tag_fmt || head.tag_data != tag_data
			|| head.size_WAVEFORMAT != sizeof(WAVEFORMAT)) {
			fclose(f); f = NULL; return false;
		}
		waveFormat = head.waveFormat;
		if ((waveFormat.wBitsPerSample + 7) / 8 != waveFormat.nBlockAlign
			|| waveFormat.nAvgBytesPerSec != waveFormat.nBlockAlign * waveFormat.nSamplesPerSec) {
			fclose(f); f = NULL; return false;
		}

		remain = head.size_data;
		return remain >= 0;
	}
	virtual int Read(void * buff, int size) override {
		int request = size < remain ? size : remain;
		int read = fread(buff, 1, request, f);
		remain -= read;
		for (char* p = (char*)buff + read; p < (char*)buff + size; p++) *p = 0;
		return read;
	}
	virtual void Close() override {
		if (f) {
			fclose(f);
			f = NULL;
		}
	}

private:
	FILE* f = NULL;
	Wav(const Wav&) = delete;
	Wav& operator=(const Wav&) = delete;
};

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
		delete ogg;
		delete wav;
		{
			LockGuard lock(mt_playQueue);
			while (!playQueue.empty()) playQueue.pop();
		}
		SetEvent(hEvent_Playing);
		WaitForSingleObject(hEvent_End, DELTA_TIME * 3);
	}

	SoundPlayerImpl(void * pDSD, StopCallBack stopCallBack)
		:
		pDSD((decltype(this->pDSD))pDSD),
		stopCallBack(stopCallBack),
		hEvent_Playing(CreateEvent(NULL, FALSE, FALSE, NULL)),
		hEvent_End(CreateEvent(NULL, FALSE, FALSE, NULL)),
		th_playing(&SoundPlayerImpl::thread_Playing, this)
	{
		th_playing.detach();
		Ogg::SetOggApis(ApiPack::GetApi(STR_ov_open_callbacks),
						ApiPack::GetApi(STR_ov_info),
						ApiPack::GetApi(STR_ov_read),
						ApiPack::GetApi(STR_ov_clear));
	}
	PlayID playID = InvalidPlayID;
	std::string fileName;
	SoundFile* soundFile = nullptr;

	bool stop = false;
	bool ended = false;

	static constexpr char OggAttr[] = ".ogg";
	static constexpr char WavAttr[] = ".wav";
	SoundFile* const ogg = new Ogg;
	SoundFile* const wav = new Wav;

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
};
