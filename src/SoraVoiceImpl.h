#pragma once

#include "SoraVoice.h"

#include "Type.h"
#include "InitParam.h"
#include "Config.h"
#include "Draw.h"
#include "SoundPlayer.h"
#include <mutex>

#include <dinput.h>
#ifndef DID
#define DID IDirectInputDevice
#endif // !DID

constexpr int KEYS_NUM = 256;

class SoraVoiceImpl : private SoraVoice
{
	friend SoraVoice;

	using LockGuard = std::lock_guard<std::mutex>;
	using InfoType = Draw::InfoType;
	using PlayID = SoundPlayer::PlayID;
	using StopType = SoundPlayer::StopType;

private:
	Config _config;
	
	struct Keys {
		const u8* const &keys;
		DID* const pDID;
		u8 last[KEYS_NUM];
		Keys(const u8* &keys, void* pDID)
			:keys(keys), pDID((decltype(this->pDID))pDID) {
		}
	} _keys;

	struct AutoPlay {
		const u32 &now;

		u32 &count_ch;
		u8 &wait;
		u32 &time_textbeg;
		u32 time_autoplay = 0;
		
		u8 &waitv;
		u32 time_autoplayv = 0;
		
		AutoPlay(u32& now, u32 &count_ch,
			u8 &wait, u32 &time_textbeg,
			u8 &waitv)
		:now(now), 
		count_ch(count_ch), wait(wait), time_textbeg(time_textbeg),
		waitv(waitv) {
		}
	} _aup;

private:
	const char* const Comment;
	u8& ended;

	InitParam::Status* const status;
	InitParam::Order* const order;

	Config* const config = &_config;
	Keys* const keys = &_keys;
	AutoPlay* const aup = &_aup;
	Draw* const draw;
	SoundPlayer* const player;

	mutable std::mutex mt_playID;
	PlayID playID;
	void stopCallBack(PlayID playID, StopType stopType);

private:
	SoraVoiceImpl(InitParam* initParam);
	~SoraVoiceImpl() override { 
		SoundPlayer::DestorySoundPlayer(player);
		Draw::DestoryDraw(draw);
	}

private:
	void playRandomVoice(const char* vlist);
	bool isAutoPlaying() {
		return aup->count_ch 
			&& ((config->AutoPlay && (status->playing || aup->waitv))
				|| config->AutoPlay == Config::AutoPlay_ALL);
	}

public:
	void Play(const char* v) override;
	void Stop() override;
	void Input() override;
	void Show() override;
};
