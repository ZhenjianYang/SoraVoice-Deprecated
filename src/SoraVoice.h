#pragma once

#include "InitParam.h"

#include "config.h"

#include <vorbis\vorbisfile.h>
#include <dsound.h>
#include <dinput.h>
#include <thread>
#include <mutex>
#include <d3d8/d3dx8.h>

#define MAX_OGG_FILENAME_LEN 25
#define NUM_AUDIO_BUF 2
#define NUM_NOTIFY_PER_BUFF 8
#define NUM_NOTIFY (NUM_AUDIO_BUF * NUM_NOTIFY_PER_BUFF)

struct SoraVoice
{
	const InitParam* const initParam;

private:
	const byte& isAo;
	byte& ended;

	InitParam::Status* const status;
	InitParam::Order* const order;

	struct _SVInner
	{
		Config _config;
		struct _OggType {
			decltype(::ov_open_callbacks)* const ov_open_callbacks;
			decltype(::ov_info)* const ov_info;
			decltype(::ov_read)* const ov_read;
			decltype(::ov_clear)* const ov_clear;

			OggVorbis_File ovf;
			char oggFn[MAX_OGG_FILENAME_LEN + 1];

			_OggType(InitParam* ip)
				:ov_open_callbacks((decltype(ov_open_callbacks))*ip->p_ov_open_callbacks),
				ov_info((decltype(ov_info))*ip->p_ov_info),
				ov_read((decltype(ov_read))*ip->p_ov_read),
				ov_clear((decltype(ov_clear))*ip->p_ov_clear) {
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

			_DsdType(InitParam* ip)
				: pDS((decltype(pDS))*ip->p_pDS), pDSBuff(nullptr),
				buffSize(0), notifySize(0), halfNotifySize(0) {
				memset(&waveFormatEx, 0, sizeof(waveFormatEx));
				memset(&dSBufferDesc, 0, sizeof(dSBufferDesc));
				memset(dSNotifies, 0, sizeof(dSNotifies));
			}
		} _dsd;
		struct _ThType {
			HANDLE hEvents[NUM_NOTIFY];
			HANDLE hEventEnd;
			std::thread th_read;
			std::mutex mt;
			int playEnd;
			_ThType(InitParam* ip) : playEnd(-1), hEventEnd(NULL) {
				memset(hEvents, 0, sizeof(hEvents));
			}
		} _th;
		struct _InpType {
			char* const keys;
			char* const last;
			_InpType(InitParam* ip) : keys(ip->p_Keys), last(ip->keysOld) {
			}
		} _inp;

		_SVInner(InitParam* initParam) :
			_ogg(initParam), _dsd(initParam), _th(initParam), _inp(initParam) {

		}
	} _svinner;

	decltype(_svinner._ogg)* const ogg = &_svinner._ogg;
	decltype(_svinner._dsd)* const dsd = &_svinner._dsd;
	decltype(_svinner._th)* const th = &_svinner._th;
	decltype(_svinner._inp)* const inp = &_svinner._inp;
	decltype(_svinner._config)* const config = &_svinner._config;

private:
	int ReadSoundData(char* buff, int size);
	void ThreadReadData();
	void PlaySoundFile();
	void StopPlaying();

public:
	SoraVoice(InitParam* initParam)
		:initParam(initParam), isAo(initParam->isAo), ended(initParam->status.ended), status(&initParam->status), order(&initParam->order),
		_svinner(initParam)
	{
	}

	void Init();
	void Play(const char* v);
	void Stop();
	void End();
	void Input();
	void Show(IDirect3DDevice8 *D3DD);
};


