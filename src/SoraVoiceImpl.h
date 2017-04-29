
#include "SoraVoice.h"

#include "InitParam.h"
#include "Config.h"
#include "Draw.h"
#include "SoundPlayer.h"
#include <mutex>

#include <dinput.h>
#ifndef DID
#define DID IDirectInputDevice
#endif // !DID

constexpr int KEYS_NUM = 16;

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
		const char* const &keys;
		DID* const pDID;
		char last[KEYS_NUM];
		Keys(const char* &keys, void* pDID)
			:keys(keys), pDID((decltype(this->pDID))pDID) {
		}
	} _keys;

	struct AutoPlay {
		const unsigned &now;

		unsigned &count_ch;
		char &wait;
		unsigned &time_textbeg;
		unsigned time_autoplay = 0;
		
		char &waitv;
		unsigned time_autoplayv = 0;
		
		AutoPlay(unsigned& now, unsigned &count_ch, 
			char &wait, unsigned &time_textbeg,
			char &waitv)
		:now(now), 
		count_ch(count_ch), wait(wait), time_textbeg(time_textbeg),
		waitv(waitv) {
		}
	} _aup;

private:
	const char* const Comment;
	char& ended;

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
