#include "ed_voice.h"
#include "config.h"
#include "Log.h"
#include "ed_voice_type.h"

#include "mapping.h"

#include <thread>
#include <mutex>

#include <dsound.h>

#include <test.h>

#define CONFIG_FILE "ed_voice.ini"

#define VOICEFILE_PREFIX "voice\\ch"
#define VOICEFILE_PREFIX_AO "voice\\\\v"
#define VOICEFILE_ATTR	".ogg"
#define VOLUME_STEP 5

static const int NUM_AUDIO_BUF = 2;
static const int NUM_NOTIFY_PER_BUFF = 8;
static const int NUM_NOTIFY = NUM_AUDIO_BUF * NUM_NOTIFY_PER_BUFF;
static const int TIME_BUF = 1;

static_assert(NUM_NOTIFY <= MAXIMUM_WAIT_OBJECTS, "Notifies exceeds the maxmin number");

static int _ReadSoundData(SVData *sv, char* buff, int size);
static void _ThreadReadData(SVData *sv);
static void _PlaySoundFile(SVData *sv);
static void _StopPlaying(SVData *sv);

struct SVData
{
	const InitParam* const initParam;

	const byte& isAo;
	byte& ended;

	InitParam::Status* const status;
	InitParam::Order* const order;

private:
	Config _config;

	struct _OggType {
		VF_ov_open_callbacks* const ov_open_callbacks;
		VF_ov_info* const ov_info;
		VF_ov_read* const ov_read;
		VF_ov_clear* const ov_clear;

		OggVorbis_File ovf;
		char oggFn[sizeof(VOICEFILE_PREFIX) + MAX_VOICEID_LEN + sizeof(VOICEFILE_ATTR)];

	private:
		friend SVData;
		_OggType(InitParam* ip)
			:ov_open_callbacks(*ip->p_ov_open_callbacks),
			ov_info(*ip->p_ov_info),
			ov_read(*ip->p_ov_read),
			ov_clear(*ip->p_ov_clear) {
			memset(&ovf, 0, sizeof(ovf));
			memset(oggFn, 0, sizeof(oggFn));
		}
	} _ogg;

	struct _DsdType {
		const LPDIRECTSOUND pDS;
		LPDIRECTSOUNDBUFFER pDSBuff;
		WAVEFORMATEX waveFormatEx;
		DSBUFFERDESC dSBufferDesc;
		DSBPOSITIONNOTIFY dSNotifies[NUM_NOTIFY];

		int buffSize;
		int notifySize;
		int halfNotifySize;

	private:
		friend SVData;
		_DsdType(InitParam* ip)
			: pDS(*ip->p_pDS), pDSBuff(nullptr), 
			buffSize(0), notifySize(0), halfNotifySize(0) {
			memset(&waveFormatEx, 0, sizeof(waveFormatEx));
			memset(&dSBufferDesc, 0, sizeof(dSBufferDesc));
			memset(dSNotifies, 0, sizeof(dSNotifies));
		}
	} _dsd;

	struct _ThType{
		HANDLE hEvents[NUM_NOTIFY];
		HANDLE hEventEnd;
		std::thread th_read;
		std::mutex mt;
		int playEnd;

	private:
		friend SVData;
		_ThType(InitParam* ip) : playEnd(-1), hEventEnd(NULL){
			memset(hEvents, 0, sizeof(hEvents));
		}
	} _th;

	struct InpType {
		char* const keys;

	private:
		friend SVData;
		InpType(InitParam* ip) : keys(ip->p_Keys) {
		}
	} _inp;

public:
	decltype(_ogg)* const ogg = &_ogg;
	decltype(_dsd)* const dsd = &_dsd;
	decltype(_th)* const th = &_th;
	decltype(_inp)* const inp = &_inp;
	decltype(_config)* const config = &_config;

	SVData(InitParam* initParam)
		:initParam(initParam), isAo(initParam->isAo), ended(initParam->status.ended), status(&initParam->status), order(&initParam->order),
		_ogg(initParam), _dsd(initParam), _th(initParam), _inp(initParam)
	{
	}
};

static inline int TO_DSVOLUME(int volume) {
	return (volume) == 0 ? 
		DSBVOLUME_MIN : 
		(int)(2000 * log10(double(volume) / MAX_Volume));
}

SVDECL void SVCALL Init(void *p)
{
	InitParam* ip = (InitParam*)p;
	if (!ip || ip->sv) return;

	LOG_OPEN;
	
	SVData *sv = new SVData(ip);

	LOG("p = 0x%08X", sv->initParam);
	LOG("p->isAo = 0x%08X", sv->initParam->isAo);
	LOG("p->p_pDS = 0x%08X", sv->initParam->p_pDS);
	LOG("p->p_ov_open_callbacks = 0x%08X", sv->initParam->p_ov_open_callbacks);
	LOG("p->p_ov_info = 0x%08X", sv->initParam->p_ov_info);
	LOG("p->p_ov_read = 0x%08X", sv->initParam->p_ov_read);
	LOG("p->p_ov_clear = 0x%08X", sv->initParam->p_ov_clear);

	LOG("pDS = 0x%08X", sv->dsd->pDS);
	LOG("ov_open_callbacks = 0x%08X", sv->ogg->ov_open_callbacks);
	LOG("ov_info = 0x%08X", sv->ogg->ov_info);
	LOG("ov_read = 0x%08X", sv->ogg->ov_read);
	LOG("ov_clear = 0x%08X", sv->ogg->ov_clear);

	if (!sv->dsd->pDS || !sv->ogg->ov_open_callbacks || !sv->ogg->ov_info || !sv->ogg->ov_read || !sv->ogg->ov_clear) {
		LOG("nullptr exists, return!");
		delete sv;
		return;
	}

	sv->config->LoadConfig(CONFIG_FILE);
	LOG("Config loaded");
	LOG("config.Volume = %d", sv->config->Volume);
	LOG("config.DisableDududu = %d", sv->config->DisableDududu);
	LOG("config.DisableDialogSE = %d", sv->config->DisableDialogSE);
	LOG("config.SkipVoice = %d", sv->config->SkipVoice);
	LOG("config.AutoPlay = %d", sv->config->AutoPlay);

	for (int i = 0; i < NUM_NOTIFY; i++) {
		sv->th->hEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
		sv->dsd->dSNotifies[i].hEventNotify = sv->th->hEvents[i];
	}
	sv->th->hEventEnd = CreateEvent(NULL, FALSE, FALSE, NULL);
	LOG("Event created!");

	const char* pre_fix = sv->isAo ? VOICEFILE_PREFIX_AO : VOICEFILE_PREFIX;
	for (int i = 0; i < sizeof(VOICEFILE_PREFIX); i++) {
		sv->ogg->oggFn[i] = pre_fix[i];
	}

	ip->sv = sv;
	ip->sv->ended = 0;

	sv->th->th_read = std::thread(_ThreadReadData, sv);
	sv->th->th_read.detach();
	LOG("Thread created!");
}

SVDECL void SVCALL End(void *p)
{
	InitParam* ip = (InitParam*)p;
	if (!ip || !ip->sv) return;

	SVData* sv = ip->sv;
	Stop(ip);
	
	sv->ended = 1;
	SetEvent(sv->th->hEvents[0]);

	WaitForSingleObject(sv->th->hEventEnd, INFINITE);

	delete sv;
	ip->sv = nullptr;
	memset(&ip->order, 0, sizeof(ip->order));
}

SVDECL void SVCALL Play(void *v, void *p)
{
	InitParam* ip = (InitParam*)p;
	if (!v || !p || !ip->sv) return;
	
	SVData *sv = ip->sv;

	const char* t = (const char*)v;
	if (*t != '#') return;

	int idx = sizeof(VOICEFILE_PREFIX) - 1;
	t++;

	int len_vid = 0;
	long long vid = 0;
	for (int i = 0; i < MAX_VOICEID_LEN; i++) {
		if (*t < '0' || *t > '9') break;
		vid *= 10; vid += *t - '0';
		sv->ogg->oggFn[idx + len_vid] = *t;

		len_vid++; t++;
	}
	sv->ogg->oggFn[idx + len_vid] = 0;
	if (*t != 'V' || len_vid == 0) return;

	LOG("Input Voice ID is \"%s\"", sv->ogg->oggFn + idx);
	LOG("The max length of voice id need mapping is %d", MAX_VOICEID_LEN_NEED_MAPPING);

	if (len_vid <= MAX_VOICEID_LEN_NEED_MAPPING) {
		vid += VoiceIdAdjustAdder[len_vid];
		LOG("Adjusted Voice ID is %d", vid);
		LOG("Number of mapping is %d", NUM_MAPPING);

		if (vid >= NUM_MAPPING) {
			LOG("Adjusted Voice ID is out of the range of Mapping", NUM_MAPPING);
			return;
		}

		if (VoiceIdMapping[vid][0] == '\0') {
			LOG("Mapping Voice ID is empty");
			return;
		}

		for (len_vid = 0; VoiceIdMapping[vid][len_vid]; len_vid++) {
			sv->ogg->oggFn[idx + len_vid] = VoiceIdMapping[vid][len_vid];
		}
		sv->ogg->oggFn[idx + len_vid] = 0;
	}
	LOG("True Voice ID is \"%s\"", sv->ogg->oggFn + idx);
	idx += len_vid;
	for (int i = 0; i < sizeof(VOICEFILE_ATTR); i++) {
		sv->ogg->oggFn[idx++] = VOICEFILE_ATTR[i];
	}

	LOG("Ogg filename: %s", sv->ogg->oggFn);
	_PlaySoundFile(sv);
}

SVDECL void SVCALL Stop(void *p)
{
	InitParam* ip = (InitParam*)p;
	if (!ip || !ip->sv) return;

	LOG("Force stopping is called.");

	SVData *sv = ip->sv;

	sv->th->mt.lock();
	_StopPlaying(sv);
	sv->th->mt.unlock();
}

SVDECL void SVCALL Input(void *p)
{
	InitParam* ip = (InitParam*)p;
	if (!ip || !ip->sv) return;
}

int _ReadSoundData(SVData *sv, char* buff, int size) {
	if (buff == nullptr || size <= 0) return 0;

	for (int i = 0; i < size; i++) buff[i] = 0;
	int total = 0;

	static const int block = 4096;
	int bitstream = 0;

	while (total < size)
	{
		int request = size - total < block ? size - total : block;
		int read = sv->ogg->ov_read(&sv->ogg->ovf, buff + total, request, 0, 2, 1, &bitstream);
		if (read <= 0) return total;

		total += read;
	}

	return total;
}

void _ThreadReadData(SVData* sv)
{
	auto &playEnd = sv->th->playEnd;
	auto &ended = sv->ended;
	while (!ended)
	{
		DWORD rst = WaitForMultipleObjects(NUM_NOTIFY, sv->th->hEvents, FALSE, INFINITE);
		if (rst >= WAIT_OBJECT_0 && rst < WAIT_OBJECT_0 + NUM_NOTIFY) {
			sv->th->mt.lock();
			
			if (sv->status->playing) {
				const int id = rst - WAIT_OBJECT_0;
				if (id == playEnd) {
					LOG("Voice end, stop!");
					_StopPlaying(sv);
					if (sv->config->AutoPlay) sv->order->autoPlay = 1;
				}
				else {
					const int buff_no = id / NUM_NOTIFY_PER_BUFF;
					const int notify_no_inbuff = id % NUM_NOTIFY_PER_BUFF;

					if (notify_no_inbuff == 0) {
						const int read_buff_no = (buff_no - 1 + NUM_AUDIO_BUF) % NUM_AUDIO_BUF;

						const int start = read_buff_no * sv->dsd->buffSize;
						const int size = read_buff_no == NUM_AUDIO_BUF - 1 ? 
							sv->dsd->dSBufferDesc.dwBufferBytes - (NUM_AUDIO_BUF - 1) * sv->dsd->buffSize 
							: sv->dsd->buffSize;

						void *AP1, *AP2;
						DWORD AB1, AB2;
						int read = 0;

						if (DS_OK == sv->dsd->pDSBuff->Lock(start, size, &AP1, &AB1, &AP2, &AB2, 0)) {
							read = _ReadSoundData(sv, (char*)AP1, AB1);
							if (AP2) read += _ReadSoundData(sv, (char*)AP2, AB2);
							sv->dsd->pDSBuff->Unlock(AP1, AB1, AP2, AB2);
						}

						if (read < size && playEnd < 0) {
							playEnd = read_buff_no * NUM_NOTIFY_PER_BUFF + (read + sv->dsd->notifySize - sv->dsd->halfNotifySize) / sv->dsd->notifySize;
							if (playEnd >= NUM_NOTIFY) playEnd = 0;
						}
					}
				}
			}
			
			sv->th->mt.unlock();
		}//if
	}//while
	SetEvent(sv->th->hEventEnd);
}

void _PlaySoundFile(SVData *sv)
{
	LOG("Start playing: %s", sv->ogg->oggFn);

	sv->th->mt.lock();
	_StopPlaying(sv);
	sv->th->mt.unlock();
	LOG("Previous playing stopped.");

	FILE* f = fopen(sv->ogg->oggFn, "rb");
	if (f == NULL) {
		LOG("Open file failed!");
		return;
	}

	auto &ovf = sv->ogg->ovf;
	if (sv->ogg->ov_open_callbacks(f, &ovf, nullptr, 0, OV_CALLBACKS_DEFAULT) != 0) {
		fclose(f);
		LOG("Open file as ogg failed!");
		return;
	}
	LOG("Ogg file opened");
		
	vorbis_info* info = sv->ogg->ov_info(&ovf, -1);
	
	auto &waveFormatEx = sv->dsd->waveFormatEx;
	waveFormatEx.wFormatTag = WAVE_FORMAT_PCM;
	waveFormatEx.nChannels = info->channels;
	waveFormatEx.nSamplesPerSec = info->rate;
	waveFormatEx.wBitsPerSample = 16;
	waveFormatEx.nBlockAlign = info->channels * 16 / 8;
	waveFormatEx.nAvgBytesPerSec = waveFormatEx.nSamplesPerSec * waveFormatEx.nBlockAlign;
	waveFormatEx.cbSize = 0;

	auto &dSBufferDesc = sv->dsd->dSBufferDesc;
	dSBufferDesc.dwSize = sizeof(dSBufferDesc);
	dSBufferDesc.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_CTRLVOLUME;
	dSBufferDesc.dwBufferBytes = waveFormatEx.nAvgBytesPerSec * TIME_BUF;
	dSBufferDesc.dwReserved = 0;
	dSBufferDesc.lpwfxFormat = &waveFormatEx;
	dSBufferDesc.guid3DAlgorithm = GUID_NULL;

	auto &buffSize = sv->dsd->buffSize;
	auto &notifySize = sv->dsd->notifySize;
	auto &halfNotifySize = sv->dsd->halfNotifySize;
	buffSize = dSBufferDesc.dwBufferBytes / NUM_AUDIO_BUF;
	notifySize = buffSize / NUM_NOTIFY_PER_BUFF;
	halfNotifySize = notifySize / 2;

	auto &pDSBuff = sv->dsd->pDSBuff;
	auto &pDS = sv->dsd->pDS;
	if (DS_OK != pDS->CreateSoundBuffer(&dSBufferDesc, &pDSBuff, NULL)) {
		LOG("Create sound buff failed!");
		sv->ogg->ov_clear(&ovf);
		return;
	}
	LOG("Sound buff opened");

	void *AP1, *AP2;
	DWORD AB1, AB2;
	if (DS_OK == pDSBuff->Lock(0, sv->dsd->buffSize * (NUM_AUDIO_BUF - 1), &AP1, &AB1, &AP2, &AB2, 0)) {
		_ReadSoundData(sv, (char*)AP1, AB1);
		if(AP2) _ReadSoundData(sv, (char*)AP2, AB2);
		pDSBuff->Unlock(AP1, AB1, AP2, AB2);
	}
	else {
		pDSBuff->Release();
		sv->ogg->ov_clear(&ovf);
		LOG("Write first data failed!");
		return;
	}
	LOG("First data wroten");

	auto &dSNotifies = sv->dsd->dSNotifies;
	for (int i = 0; i < NUM_AUDIO_BUF; ++i) {
		for (int j = 0; j < NUM_NOTIFY_PER_BUFF; j++) {
			dSNotifies[i * NUM_NOTIFY_PER_BUFF + j].dwOffset = i * buffSize + j * notifySize + halfNotifySize;
		}
	}

	static LPDIRECTSOUNDNOTIFY pDSN = NULL;
	if(DS_OK != pDSBuff->QueryInterface(IID_IDirectSoundNotify, (LPVOID*)&pDSN)){
		pDSBuff->Release();
		sv->ogg->ov_clear(&ovf);
		LOG("Set notify failed!");
		return;
	};

	if (DS_OK != pDSN->SetNotificationPositions(NUM_NOTIFY, dSNotifies)) {
		pDSN->Release();
		pDSBuff->Release();
		sv->ogg->ov_clear(&ovf);
		LOG("Set notify failed!");
		return;
	}
	pDSN->Release();
	LOG("Notify set");

	auto &config = sv->config;
	pDSBuff->SetVolume(TO_DSVOLUME(config->Volume));
	LOG("DSVolume = %d", TO_DSVOLUME(config->Volume));

	sv->th->mt.lock();
	sv->status->playing = 1;
	if (config->DisableDududu) sv->order->disableDududu = 1;
	if (config->DisableDialogSE) sv->order->disableDialogSE = 1;
	if (config->SkipVoice) sv->order->skipVoice = 1;
	pDSBuff->Play(0, 0, DSBPLAY_LOOPING);
	sv->th->mt.unlock();

	LOG("Playing...");
}

void _StopPlaying(SVData *sv)
{
	if (!sv->status->playing) return;

	if (sv->dsd->pDSBuff) {
		sv->dsd->pDSBuff->Stop();
		sv->dsd->pDSBuff->Release();
		sv->dsd->pDSBuff = NULL;
	}
	sv->status->playing = 0;
	sv->th->playEnd = -1;
	sv->order->disableDududu = 0;
	sv->order->skipVoice = 0;

	sv->ogg->ov_clear(&sv->ogg->ovf);
}
