#include "player.h"

#include "decoder.h"
#include "Decoder_Wav.h"
#include "Decoder_Ogg.h"

#include <Utils/Log.h>
#include <Utils/ApiPack.h>
#include <Utils/Clock.h>

#include <dsound.h>

#include <cmath>
#include <string>
#include <thread>
#include <mutex>
#include <queue>

using namespace Player;

static constexpr char STR_ov_open_callbacks[] = "ov_open_callbacks";
static constexpr char STR_ov_info[] = "ov_info";
static constexpr char STR_ov_read[] = "ov_read";
static constexpr char STR_ov_clear[] = "ov_clear";
static constexpr char STR_ov_pcm_total[] = "ov_pcm_total";

static constexpr int SAMPLES_BUF = 24000;
static constexpr int NUM_BUF = 2;
static constexpr int DELTA_TIME = 50;

static constexpr int INVALID_POS = 0x7FFFFFFF;

using LockGuard = std::lock_guard<std::mutex>;
using PlayInfo = struct {
	PlayID playID;
	Decoder* decoder;
	std::string fileNm;
};
using PlayQueue = std::queue<PlayInfo>;

namespace PL {
	static Status status = Status::Stoped;

	static PlayID playID = InvalidPlayID;
	static std::string fileName;
	static Decoder* decoder = nullptr;

	static bool stop = false;
	static bool ended = false;

	static IDirectSound* pDSD = nullptr;

	static StopCallBack stopCallBack = nullptr;
	static std::mutex mt_stopCallBack;

	static int volume = MaxVolume;

	static IDirectSoundBuffer *pDSBuff = nullptr;
	static std::mutex mt_DSBuff;
	static int buffSize = 0;
	static int buffIndex = 0;
	static int endPos = MaxVolume;
	static int curPos = 0;
	static int prePos = 0;

	static WAVEFORMATEX waveFormatEx {};
	static DSBUFFERDESC dSBufferDesc {};

	static PlayQueue playQueue;
	static std::mutex mt_playQueue;

	static HANDLE hEvent_Playing;
	static HANDLE hEvent_End;
	static std::thread th_playing;

	static struct {
		Ogg ogg;
		Wav wav;
	} _Decoders;
	static struct {
		Decoder* const decoder;
		std::string attr;
	} decoders[] = {
	{ &_Decoders.ogg, Ogg::Attr},
	{ &_Decoders.wav, Wav::Attr}
	};
	static Decoder *DFT_Decoder = &_Decoders.ogg;
}

using namespace PL;

static void thread_Playing();

static bool openSoundFile(const std::string& fileName);
static bool initDSBuff();
static bool startPlay();
static int playing();
static void finishPlay();

static inline int TO_DSVOLUME(int volume);
static inline Decoder* getDecoderByFileName(const std::string& fileName);
static inline PlayID generatePlayID();

Status Player::GetStatus() {
	return status;
}

PlayID Player::GetCurrentPlayID() {
	return playID;
}

const char* Player::GetCurrentFile() {
	return fileName.empty() ? nullptr : fileName.c_str();
}

PlayID Player::Play(const char* fileName, int volume, Decoder* decoder /*= nullptr*/) {
	PlayID playID = generatePlayID();
	if(!decoder) decoder = getDecoderByFileName(fileName);
	{
		LockGuard lock(mt_playQueue);
		playQueue.push({ playID, decoder, fileName });
	}
	PL::volume = volume;
	stop = false;
	SetEvent(hEvent_Playing);
	return playID;
}

void Player::Stop() {
	stop = true;
}

void Player::SetVolume(int volume /*= MaxVolume*/) {
	PL::volume = volume;
	{
		LockGuard lock(mt_DSBuff);
		if(pDSBuff)
			pDSBuff->SetVolume(TO_DSVOLUME(volume));
	}
};

void Player::SetStopCallBack(StopCallBack stopCallBack /*= nullptr*/) {
	LockGuard lock(mt_stopCallBack);
	PL::stopCallBack = stopCallBack;
}

StopCallBack Player::GetStopCallBack() {
	LockGuard lock(mt_stopCallBack);
	return stopCallBack;
}

bool Player::Init(void * pDSD, StopCallBack stopCallBack)
{
	pDSD = (decltype(pDSD))pDSD;
	PL::stopCallBack = stopCallBack;
	hEvent_Playing = CreateEvent(NULL, FALSE, FALSE, NULL);
	hEvent_End = CreateEvent(NULL, FALSE, FALSE, NULL);
	th_playing = std::thread(thread_Playing);
	th_playing.detach();
	Ogg::SetOggApis(ApiPack::GetApi(STR_ov_open_callbacks),
					ApiPack::GetApi(STR_ov_info),
					ApiPack::GetApi(STR_ov_read),
					ApiPack::GetApi(STR_ov_clear),
					ApiPack::GetApi(STR_ov_pcm_total)
					);
	return true;
}

bool Player::End()
{
	ended = true;
	Stop();
	{
		LockGuard lock(mt_playQueue);
		while (!playQueue.empty()) playQueue.pop();
	}
	SetEvent(hEvent_Playing);
	WaitForSingleObject(hEvent_End, DELTA_TIME * 3);
	return true;
}

int TO_DSVOLUME(int volume) {
	return (volume) == 0 ?
		DSBVOLUME_MIN :
		(int)(2000 * std::log10(double(volume) / MaxVolume));
}

Decoder* getDecoderByFileName(const std::string& fileName) {
	std::string attr = fileName.substr(fileName.rfind('.') + 1);
	for(auto da : decoders) {
		if(attr == da.attr) {
			return da.decoder;
		}
	}
	return DFT_Decoder;
}

PlayID generatePlayID() {
	static PlayID last = InvalidPlayID;
	return ++last == InvalidPlayID ? ++last : last;
}

void thread_Playing()
{
	while (!ended)
	{
		DWORD waitResult = WaitForSingleObject(hEvent_Playing, INFINITE);
		if (waitResult != WAIT_OBJECT_0) continue;

		status = Status::Playing;
		playID = InvalidPlayID;
		fileName.clear();

		std::queue<PlayID> forceStop;
		{
			LockGuard lock(mt_playQueue);
			while (playQueue.size() > 1)
			{
				forceStop.push(playQueue.front().playID);
				playQueue.pop();
			}

			if (!playQueue.empty())
			{
				playID = playQueue.front().playID;
				fileName = std::move(playQueue.front().fileNm);
				decoder = playQueue.front().decoder;
				playQueue.pop();
			}
		}

		if(!forceStop.empty()) {
			StopCallBack tmp_stopCallBack = GetStopCallBack();
			if(tmp_stopCallBack) {
				while(!forceStop.empty()) {
					tmp_stopCallBack(forceStop.front(), StopType::ForceStop);
					forceStop.pop();
				}
			}
		}

		if (playID != InvalidPlayID) {
			bool rst = openSoundFile(fileName)
				&& initDSBuff()
				&& startPlay();

			if (rst) {
				int remain = 0;
				while (!stop && playQueue.empty() && (remain = playing())) {
					int delta = remain * Clock::TimeUnitsPerSecond / waveFormatEx.nAvgBytesPerSec + 1;
					if (delta > DELTA_TIME) delta = DELTA_TIME;
					Clock::Sleep(delta);
				}
			}

			LOG("Play end.");
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

			StopCallBack tmp_stopCallBack = GetStopCallBack();
			if (tmp_stopCallBack) {
				tmp_stopCallBack(playID, stopType);
			}
		} //if (play.playID != InvalidPlayID)

		fileName.clear();
		playID = InvalidPlayID;
		status = Status::Stoped;
	} //while (!ended)
	SetEvent(hEvent_End);
}

bool openSoundFile(const std::string& fileName){
	LOG("Open file %s ...", fileName.c_str());
	LOG("%s File.", decoder == &_Decoders.ogg ? "ogg" : "wav");
	if (!decoder->Open(fileName.c_str())) {
		LOG("Open file as sound file failed!");
		return false;
	}
	LOG("File opened, information:\n"
		"    Channels      : %d\n"
		"    SamplesPerSec : %d\n"
		"    Total Time  : %d ms",
		decoder->WaveFormat.nChannels,
		decoder->WaveFormat.nSamplesPerSec,
		decoder->SamplesTotal() * 1000 / decoder->WaveFormat.nSamplesPerSec
	);

	waveFormatEx.wFormatTag = decoder->WaveFormat.wFormatTag;
	waveFormatEx.nChannels = decoder->WaveFormat.nChannels;
	waveFormatEx.nSamplesPerSec = decoder->WaveFormat.nSamplesPerSec;
	waveFormatEx.wBitsPerSample = decoder->WaveFormat.wBitsPerSample;
	waveFormatEx.nBlockAlign = decoder->WaveFormat.nBlockAlign;
	waveFormatEx.nAvgBytesPerSec = decoder->WaveFormat.nAvgBytesPerSec;
	waveFormatEx.cbSize = 0;

	return true;
}

bool initDSBuff(){
	memset(&dSBufferDesc, 0, sizeof(dSBufferDesc));
	dSBufferDesc.dwSize = sizeof(dSBufferDesc);
	dSBufferDesc.dwFlags = DSBCAPS_CTRLVOLUME;
	dSBufferDesc.dwBufferBytes = waveFormatEx.nBlockAlign * SAMPLES_BUF * NUM_BUF;
	dSBufferDesc.dwReserved = 0;
	dSBufferDesc.lpwfxFormat = &waveFormatEx;
	dSBufferDesc.guid3DAlgorithm = { 0 };

	buffSize = dSBufferDesc.dwBufferBytes / NUM_BUF;

	buffIndex = 0;
	endPos = INVALID_POS;
	curPos = 0;
	prePos = 0;

	if (DS_OK != pDSD->CreateSoundBuffer(&dSBufferDesc, &pDSBuff, NULL)) {
		LOG("Create sound buff failed!");
		return false;
	}
	LOG("Sound buff opened");
	return true;
}

bool startPlay(){
	void *AP1, *AP2;
	DWORD AB1, AB2;
	if (DS_OK == pDSBuff->Lock(0, buffSize * (NUM_BUF - 1), &AP1, &AB1, &AP2, &AB2, 0)) {
		decoder->Read(AP1, AB1);
		if (AP2) decoder->Read(AP2, AB2);
		pDSBuff->Unlock(AP1, AB1, AP2, AB2);
	}
	else {
		pDSBuff->Release();
		LOG("Write first data failed!");
		return 0;
	}
	LOG("First data wroten");

	pDSBuff->SetVolume(TO_DSVOLUME(volume));
	pDSBuff->Play(0, 0, DSBPLAY_LOOPING);

	LOG("Playing...");
	return 1;
}

int playing() {
	pDSBuff->GetCurrentPosition((LPDWORD)&curPos, NULL);
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
			read = decoder->Read(AP1, AB1);
			if (AP2) read += decoder->Read(AP2, AB2);
			pDSBuff->Unlock(AP1, AB1, AP2, AB2);
		}

		if (endPos == INVALID_POS && read < size) {
			endPos = start + read;
			if (endPos < curPos) endPos += dSBufferDesc.dwBufferBytes;
		}
		buffIndex += 1;
	}

	return endPos == INVALID_POS ?
			dSBufferDesc.dwBufferBytes - curPos % buffSize
			: endPos - curPos;
}

void finishPlay() {
	decoder->Close();
	decoder = nullptr;

	{
		LockGuard lock(mt_DSBuff);
		if (pDSBuff) {
			pDSBuff->Stop();
			pDSBuff->Release();
			pDSBuff = NULL;
		}
	}
}

