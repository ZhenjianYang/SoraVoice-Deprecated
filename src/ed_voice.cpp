#include "ed_voice.h"
#include "config.h"
#include "Log.h"
#include "ed_voice_type.h"

#include "mapping.h"


#include <thread>
#include <mutex>

#include <dsound.h>

#define CONFIG_FILE "ed_voice.ini"

#define VOICEFILE_PREFIX "voice\\ch"
#define VOICEFILE_PREFIX_AO "voice\\\\v"
#define VOICEFILE_ATTR	".ogg"
#define VOLUME_STEP 5


static const int NUM_AUDIO_BUF = 2;
static const int TIME_BUF = 1;

static bool _startup = false;
static bool _isAo = false;

static int *_pflagPlaying = nullptr;

static Config _config;

static VF _vf;

static HANDLE _hEvent[NUM_AUDIO_BUF];

static std::thread _th_read;
static std::mutex _mt;
static bool _isPlaying = false;
static int _playEnd = -1;

static OggVorbis_File _ovf;
static LPDIRECTSOUND _pDS = NULL;
static LPDIRECTSOUNDBUFFER _pDSBuff = NULL;
static WAVEFORMATEX _waveFormatEx;
static DSBUFFERDESC _dSBufferDesc;
static int _buffSize = 0;
static int _halfSize = 0;
static DSBPOSITIONNOTIFY _dSNotify[NUM_AUDIO_BUF];

static char _buff_vfn[sizeof(VOICEFILE_PREFIX) + MAX_VOICEID_LEN + sizeof(VOICEFILE_ATTR)];

static void _ThreadReadData();
static void _PlaySoundFile(const char* fileNm);
static void _StopPlaying();

#define TO_DSVOLUME(volume) ((volume) == 0 ? DSBVOLUME_MIN : \
	(int)(2000 * log10(double(volume) / MAX_Volume)))

SVDECL void SVCALL Init(void *p)
{
	if (_startup || nullptr == p) return;

	LOG_OPEN;
	
	InitParam* ip = (InitParam*)p;

	LOG("p = 0x%08X", ip);
	LOG("p->isAo = 0x%08X", ip->_isAo);
	LOG("p->p_pDS = 0x%08X", ip->p_pDS);
	LOG("p->p_ov_open_callbacks = 0x%08X", ip->p_ov_open_callbacks);
	LOG("p->p_ov_info = 0x%08X", ip->p_ov_info);
	LOG("p->p_ov_read = 0x%08X", ip->p_ov_read);
	LOG("p->p_ov_clear = 0x%08X", ip->p_ov_clear);

	_isAo = ip->_isAo;
	_pDS = *(ip->p_pDS);
	_pflagPlaying = &ip->flagPlaying;

	_vf.ov_open_callbacks = *(ip->p_ov_open_callbacks);
	_vf.ov_info = *(ip->p_ov_info);
	_vf.ov_read = *(ip->p_ov_read);
	_vf.ov_clear = *(ip->p_ov_clear);

	LOG("pDS = 0x%08X", _pDS);
	LOG("ov_open_callbacks = 0x%08X", _vf.ov_open_callbacks);
	LOG("ov_info = 0x%08X", _vf.ov_info);
	LOG("ov_read = 0x%08X", _vf.ov_read);
	LOG("ov_clear = 0x%08X", _vf.ov_clear);

	if (!_pDS || !_vf.ov_open_callbacks || !_vf.ov_info || !_vf.ov_read || !_vf.ov_clear) {
		LOG("nullptr exists, return!");
		return;
	}

	_startup = true;
	_config.LoadConfig(CONFIG_FILE);
	LOG("Config loaded");
	LOG("config.Volume = %d", _config.Volume);

	for (int i = 0; i < sizeof(_hEvent) / sizeof(*_hEvent); i++) {
		_hEvent[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
	}
	LOG("Event created!");

	_th_read = std::thread(_ThreadReadData);
	_th_read.detach();
	LOG("Thread created!");

	const char* pre_fix = _isAo ? VOICEFILE_PREFIX_AO : VOICEFILE_PREFIX;
	for (int i = 0; i < sizeof(VOICEFILE_PREFIX); i++) {
		_buff_vfn[i] = pre_fix[i];
	}
}

SVDECL void SVCALL End(void *p)
{
	_startup = false;
	Stop(nullptr);
}

SVDECL void SVCALL Play(void *v, void *p)
{
	if (v == nullptr) return;
	Init(p);
	if (!_startup) return;

	const char* t = (const char*)v;
	if (*t != '#') return;

	int idx = sizeof(VOICEFILE_PREFIX) - 1;
	t++;

	int len_vid = 0;
	long long vid = 0;
	for (int i = 0; i < MAX_VOICEID_LEN; i++) {
		if (*t < '0' || *t > '9') break;
		vid *= 10; vid += *t - '0';
		_buff_vfn[idx + len_vid] = *t;

		len_vid++; t++;
	}
	_buff_vfn[idx + len_vid] = 0;
	if (*t != 'V' || len_vid == 0) return;

	LOG("Input Voice ID is \"%s\"", _buff_vfn + idx);
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
			_buff_vfn[idx + len_vid] = VoiceIdMapping[vid][len_vid];
		}
		_buff_vfn[idx + len_vid] = 0;
	}
	LOG("True Voice ID is \"%s\"", _buff_vfn + idx);
	idx += len_vid;
	for (int i = 0; i < sizeof(VOICEFILE_ATTR); i++) {
		_buff_vfn[idx++] = VOICEFILE_ATTR[i];
	}

	LOG("Ogg filename: %s", _buff_vfn);
	_PlaySoundFile(_buff_vfn);
}

SVDECL void SVCALL Stop(void *p)
{
	_mt.lock();
	_StopPlaying();
	_mt.unlock();
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
		int read = _vf.ov_read(&_ovf, buff + total, request, 0, 2, 1, &bitstream);
		if (read <= 0) return total;

		total += read;
	}

	return total;
}

void _ThreadReadData()
{
	while (_startup)
	{
		DWORD rst = WaitForMultipleObjects(NUM_AUDIO_BUF, _hEvent, FALSE, INFINITE);
		if (rst >= WAIT_OBJECT_0 && rst < WAIT_OBJECT_0 + NUM_AUDIO_BUF) {
			_mt.lock();
			
			if (_isPlaying) {
				int id = rst - WAIT_OBJECT_0;
				if (_playEnd >= 0) {
					if (id == _playEnd) {
						LOG("Voice end, stop!");
						_StopPlaying();
					}
				}
				else {
					int size = id != 0 ? _buffSize : _dSBufferDesc.dwBufferBytes - (NUM_AUDIO_BUF - 1) * _buffSize;
					int start = (id - 1 + NUM_AUDIO_BUF) % NUM_AUDIO_BUF * _buffSize;

					void *AP1, *AP2;
					DWORD AB1, AB2;
					int read = 0;

					if (DS_OK == _pDSBuff->Lock(start, size, &AP1, &AB1, &AP2, &AB2, 0)) {
						read = _ReadSoundData((char*)AP1, AB1);
						if (AP2) read += _ReadSoundData((char*)AP2, AB2);
						_pDSBuff->Unlock(AP1, AB1, AP2, AB2);
					}

					if (read < _halfSize) {
						_playEnd = (id - 1 + NUM_AUDIO_BUF) % NUM_AUDIO_BUF;
					}
				}
			}
			
			_mt.unlock();
		}
	}
}

void _PlaySoundFile(const char* fileNm)
{
	LOG("Start playing: %s", fileNm);

	_mt.lock();
	_StopPlaying();
	_mt.unlock();
	LOG("Previous playing stopped.");

	FILE* f = fopen(fileNm, "rb");
	if (f == NULL) {
		LOG("Open file failed!");
		return;
	}

	if (_vf.ov_open_callbacks(f, &_ovf, nullptr, 0, OV_CALLBACKS_DEFAULT) != 0) {
		fclose(f);
		LOG("Open file as ogg failed!");
		return;
	}
	LOG("Ogg file opened");
		
	vorbis_info* info = _vf.ov_info(&_ovf, -1);
	
	_waveFormatEx.wFormatTag = WAVE_FORMAT_PCM;
	_waveFormatEx.nChannels = info->channels;
	_waveFormatEx.nSamplesPerSec = info->rate;
	_waveFormatEx.wBitsPerSample = 16;
	_waveFormatEx.nBlockAlign = info->channels * 16 / 8;
	_waveFormatEx.nAvgBytesPerSec = _waveFormatEx.nSamplesPerSec * _waveFormatEx.nBlockAlign;
	_waveFormatEx.cbSize = 0;

	_dSBufferDesc.dwSize = sizeof(DSBUFFERDESC);
	_dSBufferDesc.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_CTRLVOLUME;
	_dSBufferDesc.dwBufferBytes = _waveFormatEx.nAvgBytesPerSec * TIME_BUF;
	_dSBufferDesc.dwReserved = 0;
	_dSBufferDesc.lpwfxFormat = &_waveFormatEx;
	_dSBufferDesc.guid3DAlgorithm = GUID_NULL;

	_buffSize = _dSBufferDesc.dwBufferBytes / NUM_AUDIO_BUF;
	_halfSize = _buffSize / 2;

	if (DS_OK != _pDS->CreateSoundBuffer(&_dSBufferDesc, &_pDSBuff, NULL)) {
		LOG("Create sound buff failed!");
		_vf.ov_clear(&_ovf);
		return;
	}
	LOG("Sound buff opened");

	void *AP1, *AP2;
	DWORD AB1, AB2;
	if (DS_OK == _pDSBuff->Lock(0, _dSBufferDesc.dwBufferBytes / NUM_AUDIO_BUF * (NUM_AUDIO_BUF - 1), &AP1, &AB1, &AP2, &AB2, 0)) {
		_ReadSoundData((char*)AP1, AB1);
		if(AP2) _ReadSoundData((char*)AP2, AB2);
		_pDSBuff->Unlock(AP1, AB1, AP2, AB2);
	}
	else {
		_pDSBuff->Release();
		_vf.ov_clear(&_ovf);
		LOG("Write first data failed!");
		return;
	}
	LOG("First data wroten");

	for (int i = 0; i < NUM_AUDIO_BUF; ++i) {
		_dSNotify[i].hEventNotify = _hEvent[i];
		_dSNotify[i].dwOffset = i * _buffSize + _halfSize;
	}

	static LPDIRECTSOUNDNOTIFY pDSN = NULL;
	if(DS_OK != _pDSBuff->QueryInterface(IID_IDirectSoundNotify, (LPVOID*)&pDSN)){
		_pDSBuff->Release();
		_vf.ov_clear(&_ovf);
		LOG("Set notify failed!");
		return;
	};

	if (DS_OK != pDSN->SetNotificationPositions(NUM_AUDIO_BUF, _dSNotify)) {
		pDSN->Release();
		_pDSBuff->Release();
		_vf.ov_clear(&_ovf);
		LOG("Set notify failed!");
		return;
	}
	pDSN->Release();
	LOG("Notify set");

	_pDSBuff->SetVolume(TO_DSVOLUME(_config.Volume));
	LOG("DSVolume = %d", TO_DSVOLUME(_config.Volume));

	_mt.lock();
	_isPlaying = true;
	if (_pflagPlaying && _config.DisableDududu) *_pflagPlaying = 1;
	_pDSBuff->Play(0, 0, DSBPLAY_LOOPING);
	_mt.unlock();

	LOG("Playing...");
}

void _StopPlaying()
{
	if (!_isPlaying) return;

	_isPlaying = false;
	_playEnd = -1;
	if(_pflagPlaying) *_pflagPlaying = 0;
	if (_pDSBuff) {
		_pDSBuff->Stop();
		_pDSBuff->Release();
	}
	_vf.ov_clear(&_ovf);
}
