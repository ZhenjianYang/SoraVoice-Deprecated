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

static Status PL_status = Status::Stoped;

static PlayID PL_playID = InvalidPlayID;
static std::string PL_fileName;
static Decoder* PL_decoder = nullptr;

static bool PL_stop = false;
static bool PL_ended = false;

static IDirectSound* PL_pDSD = nullptr;

static StopCallBack PL_stopCallBack = nullptr;
static std::mutex PL_mt_stopCallBack;

static int PL_volume = MaxVolume;

static IDirectSoundBuffer *PL_pDSBuff = nullptr;
static std::mutex PL_mt_DSBuff;
static int PL_buffSize = 0;
static int PL_buffIndex = 0;
static int PL_endPos = MaxVolume;
static int PL_curPos = 0;
static int PL_prePos = 0;

static WAVEFORMATEX PL_waveFormatEx{};
static DSBUFFERDESC PL_dSBufferDesc{};

static PlayQueue PL_playQueue;
static std::mutex PL_mt_playQueue;

static HANDLE PL_hEvent_Playing;
static HANDLE PL_hEvent_End;
static std::thread PL_th_playing;

static struct {
	Decoder* const decoder;
	std::string attr;
} Decoders[] = {
	{ Ogg::ogg, Ogg::Attr },
	{ Wav::wav, Wav::Attr }
};
static Decoder* const DFT_Decoder = Wav::wav;

static void thread_Playing();

static bool openSoundFile(const std::string& fileName);
static bool initDSBuff();
static bool startPlay();
static int playing();
static void finishPlay();

static inline int toDSVolume(int volume);
static inline Decoder* getDecoderByFileName(const std::string& fileName);
static inline PlayID generatePlayID();

Status Player::GetStatus() {
	return PL_status;
}

PlayID Player::GetCurrentPlayID() {
	return PL_playID;
}

const char* Player::GetCurrentFile() {
	return PL_fileName.empty() ? nullptr : PL_fileName.c_str();
}

PlayID Player::Play(const char* fileName, int volume, Decoder* decoder /*= nullptr*/) {
	PlayID playID = generatePlayID();
	if(!decoder) decoder = getDecoderByFileName(fileName);
	{
		LockGuard lock(PL_mt_playQueue);
		PL_playQueue.push({ playID, decoder, fileName });
	}
	PL_volume = volume;
	PL_stop = false;
	SetEvent(PL_hEvent_Playing);
	return playID;
}

void Player::Stop() {
	PL_stop = true;
}

void Player::SetVolume(int volume /*= MaxVolume*/) {
	PL_volume = volume;
	{
		LockGuard lock(PL_mt_DSBuff);
		if(PL_pDSBuff)
			PL_pDSBuff->SetVolume(toDSVolume(volume));
	}
};

void Player::SetStopCallBack(StopCallBack stopCallBack /*= nullptr*/) {
	LockGuard lock(PL_mt_stopCallBack);
	PL_stopCallBack = stopCallBack;
}

StopCallBack Player::GetStopCallBack() {
	LockGuard lock(PL_mt_stopCallBack);
	return PL_stopCallBack;
}

bool Player::Init(void * pDSD, StopCallBack stopCallBack)
{
	PL_pDSD = (decltype(PL_pDSD))pDSD;
	PL_stopCallBack = stopCallBack;
	PL_hEvent_Playing = CreateEvent(NULL, FALSE, FALSE, NULL);
	PL_hEvent_End = CreateEvent(NULL, FALSE, FALSE, NULL);
	PL_th_playing = std::thread(thread_Playing);
	PL_th_playing.detach();
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
	PL_ended = true;
	Stop();
	{
		LockGuard lock(PL_mt_playQueue);
		while (!PL_playQueue.empty()) PL_playQueue.pop();
	}
	SetEvent(PL_hEvent_Playing);
	WaitForSingleObject(PL_hEvent_End, DELTA_TIME * 3);
	return true;
}

int toDSVolume(int volume) {
	return (volume) == 0 ?
		DSBVOLUME_MIN :
		(int)(2000 * std::log10(double(volume) / MaxVolume));
}

Decoder* getDecoderByFileName(const std::string& fileName) {
	std::string attr = fileName.substr(fileName.rfind('.') + 1);
	for(auto da : Decoders) {
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
	while (!PL_ended)
	{
		DWORD waitResult = WaitForSingleObject(PL_hEvent_Playing, INFINITE);
		if (waitResult != WAIT_OBJECT_0) continue;

		PL_status = Status::Playing;
		PL_playID = InvalidPlayID;
		PL_fileName.clear();

		std::queue<PlayID> forceStop;
		{
			LockGuard lock(PL_mt_playQueue);
			while (PL_playQueue.size() > 1)
			{
				forceStop.push(PL_playQueue.front().playID);
				PL_playQueue.pop();
			}

			if (!PL_playQueue.empty())
			{
				PL_playID = PL_playQueue.front().playID;
				PL_fileName = std::move(PL_playQueue.front().fileNm);
				PL_decoder = PL_playQueue.front().decoder;
				PL_playQueue.pop();
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

		if (PL_playID != InvalidPlayID) {
			bool rst = openSoundFile(PL_fileName)
				&& initDSBuff()
				&& startPlay();

			if (rst) {
				int remain = 0;
				while (!PL_stop && PL_playQueue.empty() && (remain = playing()) > 0) {
					int delta = remain * Clock::TimeUnitsPerSecond / PL_waveFormatEx.nAvgBytesPerSec + 1;
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
			else if (PL_stop || !PL_playQueue.empty()) {
				stopType = StopType::ForceStop;
			}
			else {
				stopType = StopType::PlayEnd;
			}

			StopCallBack tmp_stopCallBack = GetStopCallBack();
			if (tmp_stopCallBack) {
				tmp_stopCallBack(PL_playID, stopType);
			}
		} //if (play.playID != InvalidPlayID)

		PL_fileName.clear();
		PL_playID = InvalidPlayID;
		PL_status = Status::Stoped;
	} //while (!ended)
	SetEvent(PL_hEvent_End);
}

bool openSoundFile(const std::string& fileName){
	LOG("Open file %s ...", fileName.c_str());
	LOG("%s File.", PL_decoder == Ogg::ogg ? "ogg" : "wav");
	if (!PL_decoder->Open(fileName.c_str())) {
		LOG("Open file as sound file failed!");
		return false;
	}
	LOG("File opened, information:\n"
		"    Channels      : %d\n"
		"    SamplesPerSec : %d\n"
		"    Total Time  : %d ms",
		PL_decoder->WaveFormat.nChannels,
		PL_decoder->WaveFormat.nSamplesPerSec,
		PL_decoder->SamplesTotal() * 1000 / PL_decoder->WaveFormat.nSamplesPerSec
	);

	PL_waveFormatEx.wFormatTag = PL_decoder->WaveFormat.wFormatTag;
	PL_waveFormatEx.nChannels = PL_decoder->WaveFormat.nChannels;
	PL_waveFormatEx.nSamplesPerSec = PL_decoder->WaveFormat.nSamplesPerSec;
	PL_waveFormatEx.wBitsPerSample = PL_decoder->WaveFormat.wBitsPerSample;
	PL_waveFormatEx.nBlockAlign = PL_decoder->WaveFormat.nBlockAlign;
	PL_waveFormatEx.nAvgBytesPerSec = PL_decoder->WaveFormat.nAvgBytesPerSec;
	PL_waveFormatEx.cbSize = 0;

	return true;
}

bool initDSBuff(){
	memset(&PL_dSBufferDesc, 0, sizeof(PL_dSBufferDesc));
	PL_dSBufferDesc.dwSize = sizeof(PL_dSBufferDesc);
	PL_dSBufferDesc.dwFlags = DSBCAPS_CTRLVOLUME;
	PL_dSBufferDesc.dwBufferBytes = PL_waveFormatEx.nBlockAlign * SAMPLES_BUF * NUM_BUF;
	PL_dSBufferDesc.dwReserved = 0;
	PL_dSBufferDesc.lpwfxFormat = &PL_waveFormatEx;
	PL_dSBufferDesc.guid3DAlgorithm = { 0 };

	PL_buffSize = PL_dSBufferDesc.dwBufferBytes / NUM_BUF;

	PL_buffIndex = 0;
	PL_endPos = INVALID_POS;
	PL_curPos = 0;
	PL_prePos = 0;

	if (DS_OK != PL_pDSD->CreateSoundBuffer(&PL_dSBufferDesc, &PL_pDSBuff, NULL)) {
		LOG("Create sound buff failed!");
		return false;
	}
	LOG("Sound buff opened");
	return true;
}

bool startPlay(){
	void *AP1, *AP2;
	DWORD AB1, AB2;
	if (DS_OK == PL_pDSBuff->Lock(0, PL_buffSize * (NUM_BUF - 1), &AP1, &AB1, &AP2, &AB2, 0)) {
		PL_decoder->Read(AP1, AB1);
		if (AP2) PL_decoder->Read(AP2, AB2);
		PL_pDSBuff->Unlock(AP1, AB1, AP2, AB2);
	}
	else {
		PL_pDSBuff->Release();
		LOG("Write first data failed!");
		return 0;
	}
	LOG("First data wroten");

	PL_pDSBuff->SetVolume(toDSVolume(PL_volume));
	PL_pDSBuff->Play(0, 0, DSBPLAY_LOOPING);

	LOG("Playing...");
	return 1;
}

int playing() {
	PL_pDSBuff->GetCurrentPosition((LPDWORD)&PL_curPos, NULL);
	if (PL_curPos < PL_prePos) {
		PL_buffIndex = 0;
		if (PL_endPos != INVALID_POS) PL_endPos -= PL_dSBufferDesc.dwBufferBytes;
	}
	PL_prePos = PL_curPos;
	if (PL_curPos > PL_endPos) {
		return 0;
	}
	else if (PL_curPos > PL_buffIndex * PL_buffSize) {
		const int buffReadIndex = (PL_buffIndex + NUM_BUF - 1) % NUM_BUF;

		const int start = buffReadIndex * PL_buffSize;
		const int size = buffReadIndex == NUM_BUF - 1 ?
			PL_dSBufferDesc.dwBufferBytes - (NUM_BUF - 1) * PL_buffSize
			: PL_buffSize;

		void *AP1, *AP2;
		DWORD AB1, AB2;
		int read = 0;

		if (DS_OK == PL_pDSBuff->Lock(start, size, &AP1, &AB1, &AP2, &AB2, 0)) {
			read = PL_decoder->Read(AP1, AB1);
			if (AP2) read += PL_decoder->Read(AP2, AB2);
			PL_pDSBuff->Unlock(AP1, AB1, AP2, AB2);
		}

		if (PL_endPos == INVALID_POS && read < size) {
			PL_endPos = start + read;
			if (PL_endPos < PL_curPos) PL_endPos += PL_dSBufferDesc.dwBufferBytes;
		}
		PL_buffIndex += 1;
	}

	return PL_endPos == INVALID_POS ?
		PL_dSBufferDesc.dwBufferBytes - PL_curPos % PL_buffSize
		: PL_endPos - PL_curPos;
}

void finishPlay() {
	PL_decoder->Close();
	PL_decoder = nullptr;

	{
		LockGuard lock(PL_mt_DSBuff);
		if (PL_pDSBuff) {
			PL_pDSBuff->Stop();
			PL_pDSBuff->Release();
			PL_pDSBuff = NULL;
		}
	}
}

