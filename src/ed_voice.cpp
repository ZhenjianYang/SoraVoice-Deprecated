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

int _ReadSoundData(char* buff, int size);
void _ThreadReadData();
void _PlaySoundFile();
void _StopPlaying();

static struct 
{
	InitParam* initParam = nullptr;
	struct {
		byte isAo;
		byte startup;
	} *base = nullptr;

	InitParam::Status* status;
	InitParam::Order* order;

private:
	Config _config;

	struct {
		VF_ov_open_callbacks* ov_open_callbacks;
		VF_ov_info* ov_info;
		VF_ov_read* ov_read;
		VF_ov_clear* ov_clear;

		OggVorbis_File ovf;
		char oggFn[sizeof(VOICEFILE_PREFIX) + MAX_VOICEID_LEN + sizeof(VOICEFILE_ATTR)];
	} _ogg;

	struct {
		LPDIRECTSOUND pDS;
		LPDIRECTSOUNDBUFFER pDSBuff;
		WAVEFORMATEX waveFormatEx;
		DSBUFFERDESC dSBufferDesc;
		DSBPOSITIONNOTIFY dSNotifies[NUM_NOTIFY];

		int buffSize;
		int notifySize;
		int halfNotifySize;
	} _dsd;

	struct {
		HANDLE hEvents[NUM_NOTIFY];
		std::thread th_read;
		std::mutex mt;
		int playEnd = -1;
	} _th;

	struct {
		char* keys;
	} _inp;

public:
	decltype(_ogg)* const ogg = &_ogg;
	decltype(_dsd)* const dsd = &_dsd;
	decltype(_th)* const th = &_th;
	decltype(_inp)* const inp = &_inp;
	decltype(_config)* const config = &_config;

	void SetSV(InitParam* initParam) {
		if (!initParam) return;

		this->initParam = initParam;
		this->base = (decltype(this->base))initParam;
		this->status = &initParam->status;
		this->order = &initParam->order;

		this->ogg->ov_open_callbacks = *(initParam->p_ov_open_callbacks);
		this->ogg->ov_info = *(initParam->p_ov_info);
		this->ogg->ov_read = *(initParam->p_ov_read);
		this->ogg->ov_clear = *(initParam->p_ov_clear);

		this->dsd->pDS = *(initParam->p_pDS);

		this->inp->keys = initParam->p_Keys;
	}
} sv;
const auto psv = &sv;

static inline int TO_DSVOLUME(int volume) {
	return (volume) == 0 ? 
		DSBVOLUME_MIN : 
		(int)(2000 * log10(double(volume) / MAX_Volume));
}

SVDECL void SVCALL Init(void *p)
{
	if (psv->base && psv->base->startup || nullptr == p) return;

	LOG_OPEN;
	
	psv->SetSV((InitParam*)p);

	LOG("p = 0x%08X", psv->initParam);
	LOG("p->isAo = 0x%08X", psv->initParam->isAo);
	LOG("p->p_pDS = 0x%08X", psv->initParam->p_pDS);
	LOG("p->p_ov_open_callbacks = 0x%08X", psv->initParam->p_ov_open_callbacks);
	LOG("p->p_ov_info = 0x%08X", psv->initParam->p_ov_info);
	LOG("p->p_ov_read = 0x%08X", psv->initParam->p_ov_read);
	LOG("p->p_ov_clear = 0x%08X", psv->initParam->p_ov_clear);

	LOG("pDS = 0x%08X", psv->dsd->pDS);
	LOG("ov_open_callbacks = 0x%08X", psv->ogg->ov_open_callbacks);
	LOG("ov_info = 0x%08X", psv->ogg->ov_info);
	LOG("ov_read = 0x%08X", psv->ogg->ov_read);
	LOG("ov_clear = 0x%08X", psv->ogg->ov_clear);

	if (!psv->dsd->pDS || !psv->ogg->ov_open_callbacks || !psv->ogg->ov_info || !psv->ogg->ov_read || !psv->ogg->ov_clear) {
		LOG("nullptr exists, return!");
		return;
	}

	psv->base->startup = 1;

	psv->config->LoadConfig(CONFIG_FILE);
	LOG("Config loaded");
	LOG("config.Volume = %d", psv->config->Volume);
	LOG("config.DisableDududu = %d", psv->config->DisableDududu);
	LOG("config.DisableDialogSE = %d", psv->config->DisableDialogSE);
	LOG("config.SkipVoice = %d", psv->config->SkipVoice);

	for (int i = 0; i < sizeof(psv->th->hEvents) / sizeof(*psv->th->hEvents); i++) {
		psv->th->hEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
		psv->dsd->dSNotifies[i].hEventNotify = psv->th->hEvents[i];
	}
	LOG("Event created!");

	psv->th->th_read = std::thread(_ThreadReadData);
	psv->th->th_read.detach();
	LOG("Thread created!");

	const char* pre_fix = psv->base->isAo ? VOICEFILE_PREFIX_AO : VOICEFILE_PREFIX;
	for (int i = 0; i < sizeof(VOICEFILE_PREFIX); i++) {
		psv->ogg->oggFn[i] = pre_fix[i];
	}

	//LOG("Go test!");
	//Test(nullptr);
}

SVDECL void SVCALL End(void *p)
{
	psv->base->startup = false;
	Stop(psv);
	psv->initParam = nullptr;
	psv->base = nullptr;
	psv->status = nullptr;
	psv->order = nullptr;
}

SVDECL void SVCALL Play(void *v, void *p)
{
	if (v == nullptr) return;
	Init(p);
	if (!psv->base || !psv->base->startup) return;

	const char* t = (const char*)v;
	if (*t != '#') return;

	int idx = sizeof(VOICEFILE_PREFIX) - 1;
	t++;

	int len_vid = 0;
	long long vid = 0;
	for (int i = 0; i < MAX_VOICEID_LEN; i++) {
		if (*t < '0' || *t > '9') break;
		vid *= 10; vid += *t - '0';
		psv->ogg->oggFn[idx + len_vid] = *t;

		len_vid++; t++;
	}
	psv->ogg->oggFn[idx + len_vid] = 0;
	if (*t != 'V' || len_vid == 0) return;

	LOG("Input Voice ID is \"%s\"", psv->ogg->oggFn + idx);
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
			psv->ogg->oggFn[idx + len_vid] = VoiceIdMapping[vid][len_vid];
		}
		psv->ogg->oggFn[idx + len_vid] = 0;
	}
	LOG("True Voice ID is \"%s\"", psv->ogg->oggFn + idx);
	idx += len_vid;
	for (int i = 0; i < sizeof(VOICEFILE_ATTR); i++) {
		psv->ogg->oggFn[idx++] = VOICEFILE_ATTR[i];
	}

	LOG("Ogg filename: %s", psv->ogg->oggFn);
	_PlaySoundFile();
}

SVDECL void SVCALL Stop(void *p)
{
	if (!psv->config->SkipVoice && !p) return;

	LOG("Force stopping is called.");

	psv->th->mt.lock();
	_StopPlaying();
	psv->th->mt.unlock();
}

int _ReadSoundData(char* buff, int size) {
	if (buff == nullptr || size <= 0) return 0;

	for (int i = 0; i < size; i++) buff[i] = 0;
	int total = 0;

	static const int block = 4096;
	int bitstream = 0;

	while (total < size)
	{
		int request = size - total < block ? size - total : block;
		int read = psv->ogg->ov_read(&psv->ogg->ovf, buff + total, request, 0, 2, 1, &bitstream);
		if (read <= 0) return total;

		total += read;
	}

	return total;
}

void _ThreadReadData()
{
	auto &playEnd = psv->th->playEnd;
	while (psv->base && psv->base->startup)
	{
		DWORD rst = WaitForMultipleObjects(NUM_NOTIFY, psv->th->hEvents, FALSE, INFINITE);
		if (rst >= WAIT_OBJECT_0 && rst < WAIT_OBJECT_0 + NUM_NOTIFY) {
			psv->th->mt.lock();
			
			if (psv->status->playing) {
				const int id = rst - WAIT_OBJECT_0;
				if (id == playEnd) {
					LOG("Voice end, stop!");
					_StopPlaying();
				}
				else {
					const int buff_no = id / NUM_NOTIFY_PER_BUFF;
					const int notify_no_inbuff = id % NUM_NOTIFY_PER_BUFF;

					if (notify_no_inbuff == 0) {
						const int read_buff_no = (buff_no - 1 + NUM_AUDIO_BUF) % NUM_AUDIO_BUF;

						const int start = read_buff_no * psv->dsd->buffSize;
						const int size = read_buff_no == NUM_AUDIO_BUF - 1 ? 
							psv->dsd->dSBufferDesc.dwBufferBytes - (NUM_AUDIO_BUF - 1) * psv->dsd->buffSize 
							: psv->dsd->buffSize;

						void *AP1, *AP2;
						DWORD AB1, AB2;
						int read = 0;

						if (DS_OK == psv->dsd->pDSBuff->Lock(start, size, &AP1, &AB1, &AP2, &AB2, 0)) {
							read = _ReadSoundData((char*)AP1, AB1);
							if (AP2) read += _ReadSoundData((char*)AP2, AB2);
							psv->dsd->pDSBuff->Unlock(AP1, AB1, AP2, AB2);
						}

						if (read < size && playEnd < 0) {
							playEnd = read_buff_no * NUM_NOTIFY_PER_BUFF + (read + psv->dsd->notifySize - psv->dsd->halfNotifySize) / psv->dsd->notifySize;
							if (playEnd >= NUM_NOTIFY) playEnd = 0;
						}
					}
				}
			}
			
			psv->th->mt.unlock();
		}
	}
}

void _PlaySoundFile()
{
	LOG("Start playing: %s", psv->ogg->oggFn);

	psv->th->mt.lock();
	_StopPlaying();
	psv->th->mt.unlock();
	LOG("Previous playing stopped.");

	FILE* f = fopen(psv->ogg->oggFn, "rb");
	if (f == NULL) {
		LOG("Open file failed!");
		return;
	}

	auto &ovf = psv->ogg->ovf;
	if (psv->ogg->ov_open_callbacks(f, &ovf, nullptr, 0, OV_CALLBACKS_DEFAULT) != 0) {
		fclose(f);
		LOG("Open file as ogg failed!");
		return;
	}
	LOG("Ogg file opened");
		
	vorbis_info* info = psv->ogg->ov_info(&ovf, -1);
	
	auto &waveFormatEx = psv->dsd->waveFormatEx;
	waveFormatEx.wFormatTag = WAVE_FORMAT_PCM;
	waveFormatEx.nChannels = info->channels;
	waveFormatEx.nSamplesPerSec = info->rate;
	waveFormatEx.wBitsPerSample = 16;
	waveFormatEx.nBlockAlign = info->channels * 16 / 8;
	waveFormatEx.nAvgBytesPerSec = waveFormatEx.nSamplesPerSec * waveFormatEx.nBlockAlign;
	waveFormatEx.cbSize = 0;

	auto &dSBufferDesc = psv->dsd->dSBufferDesc;
	dSBufferDesc.dwSize = sizeof(dSBufferDesc);
	dSBufferDesc.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_CTRLVOLUME;
	dSBufferDesc.dwBufferBytes = waveFormatEx.nAvgBytesPerSec * TIME_BUF;
	dSBufferDesc.dwReserved = 0;
	dSBufferDesc.lpwfxFormat = &waveFormatEx;
	dSBufferDesc.guid3DAlgorithm = GUID_NULL;

	auto &buffSize = psv->dsd->buffSize;
	auto &notifySize = psv->dsd->notifySize;
	auto &halfNotifySize = psv->dsd->halfNotifySize;
	buffSize = dSBufferDesc.dwBufferBytes / NUM_AUDIO_BUF;
	notifySize = buffSize / NUM_NOTIFY_PER_BUFF;
	halfNotifySize = notifySize / 2;

	auto &pDSBuff = psv->dsd->pDSBuff;
	auto &pDS = psv->dsd->pDS;
	if (DS_OK != pDS->CreateSoundBuffer(&dSBufferDesc, &pDSBuff, NULL)) {
		LOG("Create sound buff failed!");
		psv->ogg->ov_clear(&ovf);
		return;
	}
	LOG("Sound buff opened");

	void *AP1, *AP2;
	DWORD AB1, AB2;
	if (DS_OK == pDSBuff->Lock(0, psv->dsd->buffSize * (NUM_AUDIO_BUF - 1), &AP1, &AB1, &AP2, &AB2, 0)) {
		_ReadSoundData((char*)AP1, AB1);
		if(AP2) _ReadSoundData((char*)AP2, AB2);
		pDSBuff->Unlock(AP1, AB1, AP2, AB2);
	}
	else {
		pDSBuff->Release();
		psv->ogg->ov_clear(&ovf);
		LOG("Write first data failed!");
		return;
	}
	LOG("First data wroten");

	auto &dSNotifies = psv->dsd->dSNotifies;
	for (int i = 0; i < NUM_AUDIO_BUF; ++i) {
		for (int j = 0; j < NUM_NOTIFY_PER_BUFF; j++) {
			dSNotifies[i * NUM_NOTIFY_PER_BUFF + j].dwOffset = i * buffSize + j * notifySize + halfNotifySize;
		}
	}

	static LPDIRECTSOUNDNOTIFY pDSN = NULL;
	if(DS_OK != pDSBuff->QueryInterface(IID_IDirectSoundNotify, (LPVOID*)&pDSN)){
		pDSBuff->Release();
		psv->ogg->ov_clear(&ovf);
		LOG("Set notify failed!");
		return;
	};

	if (DS_OK != pDSN->SetNotificationPositions(NUM_NOTIFY, dSNotifies)) {
		pDSN->Release();
		pDSBuff->Release();
		psv->ogg->ov_clear(&ovf);
		LOG("Set notify failed!");
		return;
	}
	pDSN->Release();
	LOG("Notify set");

	auto &config = psv->config;
	pDSBuff->SetVolume(TO_DSVOLUME(config->Volume));
	LOG("DSVolume = %d", TO_DSVOLUME(config->Volume));

	psv->th->mt.lock();
	psv->status->playing = 1;
	if (config->DisableDududu) psv->order->disableDududu = 1;
	if (config->DisableDialogSE) psv->order->disableDialogSE = 1;
	pDSBuff->Play(0, 0, DSBPLAY_LOOPING);
	psv->th->mt.unlock();

	LOG("Playing...");
}

void _StopPlaying()
{
	if (!psv->status) return;

	psv->status->playing = 0;
	psv->th->playEnd = -1;
	psv->order->disableDududu = 0;

	if (psv->dsd->pDSBuff) {
		psv->dsd->pDSBuff->Stop();
		psv->dsd->pDSBuff->Release();
		psv->dsd->pDSBuff = NULL;
	}
	psv->ogg->ov_clear(&psv->ogg->ovf);
}
