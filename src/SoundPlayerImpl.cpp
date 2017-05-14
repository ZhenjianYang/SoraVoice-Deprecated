#include "SoundPlayerImpl.h"
#include "SoundFileImpl.h"
#include "ApiPack.h"

#include "Log.h"
#include <chrono>

static constexpr char STR_ov_open_callbacks[] = "ov_open_callbacks";
static constexpr char STR_ov_info[] = "ov_info";
static constexpr char STR_ov_read[] = "ov_read";
static constexpr char STR_ov_clear[] = "ov_clear";
static constexpr char STR_ov_time_total[] = "ov_time_total";

constexpr char SoundPlayerImpl::OggAttr[];
constexpr char SoundPlayerImpl::WavAttr[];

SoundPlayer * SoundPlayer::CreatSoundPlayer(void * pDSD, StopCallBack stopCallBack)
{
	return new SoundPlayerImpl(pDSD, stopCallBack);
}

SoundPlayerImpl::SoundPlayerImpl(void * pDSD, StopCallBack stopCallBack)
	:ogg(new Ogg), wav(new Wav),
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
					ApiPack::GetApi(STR_ov_clear),
					ApiPack::GetApi(STR_ov_time_total)
	);
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
				soundFile = playQueue.front().soundFile;
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
					int delta = remain * SoundFile::TimeUnitsPerSecond / waveFormatEx.nAvgBytesPerSec + 1;
					if (delta > DELTA_TIME) delta = DELTA_TIME;
					using TimeUnit = std::chrono::duration<int, std::ratio<1, SoundFile::TimeUnitsPerSecond>>;
					std::this_thread::sleep_for(TimeUnit(delta));
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

bool SoundPlayerImpl::openSoundFile(const std::string& fileName){
	LOG("Open file %s ...", fileName.c_str());
	LOG("%s File.", soundFile == ogg ? "ogg" : "wav");
	if (!soundFile->Open(fileName.c_str())) {
		LOG("Open file as sound file failed!");
		return false;
	}
	LOG("File opened, information:\n"
		"    Channels      : %d\n"
		"    SamplesPerSec : %d\n"
		"    Total Length  : %d ms",
		soundFile->WaveFormat.nChannels, 
		soundFile->WaveFormat.nSamplesPerSec,
		soundFile->Length()
	);

	waveFormatEx.wFormatTag = soundFile->WaveFormat.wFormatTag;
	waveFormatEx.nChannels = soundFile->WaveFormat.nChannels;
	waveFormatEx.nSamplesPerSec = soundFile->WaveFormat.nSamplesPerSec;
	waveFormatEx.wBitsPerSample = soundFile->WaveFormat.wBitsPerSample;
	waveFormatEx.nBlockAlign = soundFile->WaveFormat.nBlockAlign;
	waveFormatEx.nAvgBytesPerSec = soundFile->WaveFormat.nAvgBytesPerSec;
	waveFormatEx.cbSize = 0;

	return true;
}

bool SoundPlayerImpl::initDSBuff(){
	memset(&dSBufferDesc, 0, sizeof(dSBufferDesc));
	dSBufferDesc.dwSize = sizeof(dSBufferDesc);
	dSBufferDesc.dwFlags = DSBCAPS_CTRLVOLUME;
	dSBufferDesc.dwBufferBytes = waveFormatEx.nAvgBytesPerSec * TIME_BUF * NUM_BUF / SoundFile::TimeUnitsPerSecond;
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

bool SoundPlayerImpl::startPlay(){
	void *AP1, *AP2;
	DWORD AB1, AB2;
	if (DS_OK == pDSBuff->Lock(0, buffSize * (NUM_BUF - 1), &AP1, &AB1, &AP2, &AB2, 0)) {
		soundFile->Read(AP1, AB1);
		if (AP2) soundFile->Read(AP2, AB2);
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
			read = soundFile->Read(AP1, AB1);
			if (AP2) read += soundFile->Read(AP2, AB2);
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

void SoundPlayerImpl::finishPlay() {
	soundFile->Close();
	soundFile = nullptr;

	{
		LockGuard lock(mt_DSBuff);
		if (pDSBuff) {
			pDSBuff->Stop();
			pDSBuff->Release();
			pDSBuff = NULL;
		}
	}
}
