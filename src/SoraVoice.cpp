
#include "SoraVoice.h"
#include "InitParam.h"
#include "Log.h"
#include "Config.h"
#include "Message.h"
#include "Hook.h"
#include "Clock.h"
#include "Draw.h"
#include "SoundPlayer.h"

#ifndef ZA
#include "Mapping.h"
#endif

#include <dinput.h>
#define DID IDirectInputDevice8A

#include <string>
#include <mutex>

#ifndef MAX_VOICEID_LEN
#define MAX_VOICEID_LEN 10
#endif // !MAX_VOICEID_LEN

#ifndef MAX_VOICEID_LEN_NEED_MAPPING
#define MAX_VOICEID_LEN_NEED_MAPPING 5
#endif // !MAX_VOICEID_LEN_NEED_MAPPING

#ifdef ZA
constexpr char CONFIG_FILE[] = "za_voice.ini";
constexpr char VOICEFILE_PREFIX[] = "voice\\v";
#else
constexpr char CONFIG_FILE[] = "ed_voice.ini";
constexpr char VOICEFILE_PREFIX[] = "voice\\ch";
#endif
constexpr char VOICEFILE_ATTR[] = ".ogg";

constexpr int VOLUME_STEP = 1;
constexpr int VOLUME_STEP_BIG = 5;

constexpr int KEY_MIN = DIK_5;
constexpr int KEY_MAX = DIK_EQUALS;
constexpr int KEYS_NUM = KEY_MAX - KEY_MIN + 1;

constexpr int KEY_VOLUME_UP = DIK_EQUALS;
constexpr int KEY_VOLUME_DOWN = DIK_MINUS;
constexpr int KEY_VOLUME_0 = DIK_0;
constexpr int KEY_VOLUME_BIGSTEP1 = DIK_LSHIFT;
constexpr int KEY_VOLUME_BIGSTEP2 = DIK_RSHIFT;

constexpr int KEY_AUTOPLAY = DIK_9;
constexpr int KEY_SKIPVOICE = DIK_8;
constexpr int KEY_DLGSE = DIK_7;
constexpr int KEY_DU = DIK_6;
constexpr int KEY_INFO = DIK_5;

#ifdef ZA
constexpr char SCODE_TEXT = 0x55;
constexpr char SCODE_SAY = 0x5C;
constexpr char SCODE_TALK = 0x5D;
constexpr char SCODE_MENU = 0x5E;
#else
constexpr char SCODE_TEXT = 0x54;
constexpr char SCODE_SAY = 0x5B;
constexpr char SCODE_TALK = 0x5C;
constexpr char SCODE_MENU = 0x5D;
#endif // ZA

constexpr unsigned INFO_TIME = 2000;
constexpr unsigned HELLO_TIME = 6000;
constexpr unsigned INFINITY_TIME = Draw::ShowTimeInfinity;
constexpr unsigned REMAIN_TIME = 1000;

constexpr unsigned TIME_PREC = 16;

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
	void stopCallBack(PlayID playID, StopType stopType) {
		LOG("StopCallBack: playID = 0x%08d, stopType = %d", playID, stopType);
		LockGuard lock(mt_playID);
		if (playID == this->playID) {
			if (stopType == StopType::PlayEnd) {
				aup->waitv = 1;
				aup->time_autoplayv = aup->now + config->WaitTimeDialogVoice - TIME_PREC / 2;
			}
			status->playing = 0;

			order->disableDududu = 0;
			order->disableDialogSE = 0;
			order->skipVoice = 0;
		} //if (playID == this->playID)
	}

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

SoraVoiceImpl::SoraVoiceImpl(InitParam* initParam)
	:
	_config(CONFIG_FILE),
	_keys(initParam->addrs.p_keys, *initParam->addrs.p_did),
	_aup(initParam->rcd.now, initParam->rcd.count_ch, initParam->status.wait, initParam->rcd.time_textbeg, initParam->status.waitv),
	Comment(initParam->Comment),
	ended(initParam->status.ended), status(&initParam->status), order(&initParam->order), 
	draw(Draw::CreateDraw(initParam->status.showing, *initParam->addrs.p_Hwnd, *initParam->addrs.p_d3dd, _config.FontName)),
	player(SoundPlayer::CreatSoundPlayer(*initParam->addrs.p_pDS, 
		*initParam->addrs.p_ov_open_callbacks, *initParam->addrs.p_ov_info, *initParam->addrs.p_ov_read, *initParam->addrs.p_ov_clear,
		std::bind(&SoraVoiceImpl::stopCallBack, this, std::placeholders::_1, std::placeholders::_2)))
{
	LOG("Config loaded");
	LOG("config.Volume = %d", config->Volume);
	LOG("config.AutoPlay = %d", config->AutoPlay);
	LOG("config.WaitTimePerChar = %d", config->WaitTimePerChar);
	LOG("config.WaitTimeDialog = %d", config->WaitTimeDialog);
	LOG("config.WaitTimeDialogVoice = %d", config->WaitTimeDialogVoice);
	LOG("config.SkipVoice = %d", config->SkipVoice);
	LOG("config.DisableDududu = %d", config->DisableDududu);
	LOG("config.DisableDialogSE = %d", config->DisableDialogSE);
	LOG("config.ShowInfo = %d", config->ShowInfo);
	LOG("config.FontName = %s", config->FontName);
	LOG("config.FontColor = 0x%08X", config->FontColor);

	LOG("config.EnableKeys = %d", config->EnableKeys);
	LOG("config.SaveChange = %d", config->SaveChange);

	InitHook_SetInitParam(initParam);
	
	void* pPresent = Hook_D3D_Present(*initParam->addrs.p_d3dd);
	if (pPresent) {
		LOG("Present hooked, old Present = 0x%08X", pPresent);
	}
	else {
		LOG("Hook Present failed.");
	}

	void* pGetDeviceState = Hook_DI_GetDeviceState(*initParam->addrs.p_did);
	if (pGetDeviceState) {
		LOG("GetDeviceState hooked, old GetDeviceState = 0x%08X", pGetDeviceState);
	}
	else {
		LOG("Hook GetDeviceState failed.");
	}

	if (config->ShowInfo) {
		draw->AddInfo(InfoType::Hello, HELLO_TIME, config->FontColor, Message::Title);
		draw->AddInfo(InfoType::Hello, HELLO_TIME, config->FontColor, Message::Version, Message::VersionNum);
		draw->AddInfo(InfoType::Hello, HELLO_TIME, config->FontColor, Message::GameTitle, Comment);
	}
}

void SoraVoiceImpl::Play(const char* t)
{
	if (*t != '#') return;
	t++;

	std::string str_vid;
	unsigned num_vid = 0;
	for (int i = 0; i < MAX_VOICEID_LEN; i++) {
		if (*t < '0' || *t > '9') break;
		num_vid *= 10; num_vid += *t - '0';
		str_vid.push_back(*t);

		t++;
	}
	if (*t != 'V' || str_vid.empty()) return;

	LOG("iptut Voice ID is \"%s\"", str_vid.c_str());
	LOG("The max length of voice id need mapping is %d", MAX_VOICEID_LEN_NEED_MAPPING);

	if (str_vid.length() <= MAX_VOICEID_LEN_NEED_MAPPING) {
#ifdef ZA
		LOG("Voice ID mapping is not supported in Zero/Ao, return");
		return;
#else
		num_vid += VoiceIdAdjustAdder[str_vid.length()];
		LOG("Adjusted Voice ID is %d", num_vid);
		LOG("Number of mapping is %d", NUM_MAPPING);

		if (num_vid >= NUM_MAPPING) {
			LOG("Adjusted Voice ID is out of the range of Mapping", NUM_MAPPING);
			return;
		}

		str_vid = VoiceIdMapping[num_vid];
		if (str_vid.empty()) {
			LOG("Mapping Voice ID is empty");
			return;
		}
#endif // ZA
	}

	std::string oggFileName = VOICEFILE_PREFIX + str_vid + VOICEFILE_ATTR;
	LOG("Ogg filename: %s", oggFileName.c_str());

	LOG("Now playing new file...");
	{
		LockGuard lock(mt_playID);

		playID = player->Play(oggFileName.c_str(), config->Volume);

		status->playing = 1;
//		status->code5 = 0;
//		aup->wait = 0;
//		aup->waitv = 0;
//		aup->count_ch = 0;
//		aup->time_autoplay = 0;

		order->disableDududu = config->DisableDududu;
		order->disableDialogSE = config->DisableDialogSE;
		order->skipVoice = config->SkipVoice;

		LOG("Play called, playID = 0x%08X", playID);
	}
}

void SoraVoiceImpl::Stop()
{
	LOG("Stop is called.");

	if (config->ShowInfo == Config::ShowInfo_WithMark && isAutoPlaying()) {
		draw->AddInfo(InfoType::AutoPlayMark, REMAIN_TIME, config->FontColor, Message::AutoPlayMark);
	}

	if (config->SkipVoice) {
		player->Stop();
	}
	status->playing = 0;

	order->disableDududu = 0;
	order->disableDialogSE = 0;
	order->skipVoice = 0;
	
	status->code5 = 0;
	aup->wait = 0;
	aup->waitv = 0;
	aup->count_ch = 0;
	aup->time_autoplay = 0;
}

void SoraVoiceImpl::Input()
{
	if (!config->EnableKeys) return;

	const char* keys = this->keys->keys;
	char* last = this->keys->last;

	bool needsave = false;
	bool needsetvolume = false;
	int volume_old = config->Volume;

	if (keys[KEY_VOLUME_UP] && keys[KEY_VOLUME_DOWN]) {
		if (!last[KEY_VOLUME_UP - KEY_MIN] || !last[KEY_VOLUME_DOWN - KEY_MIN]) {
			if (config->SaveChange) {
				config->Reset();
				needsave = true;
				needsetvolume = true;
			}
			else {
				config->LoadConfig(CONFIG_FILE);
				config->EnableKeys = 1;
				config->SaveChange = 0;
				needsetvolume = true;
			}
			status->mute = 0;
			if (status->playing) {
				order->skipVoice = config->SkipVoice;
				order->disableDialogSE = config->DisableDialogSE;
				order->disableDududu = config->DisableDududu;
			}
			LOG("Reset config");

			if (config->ShowInfo) {
				//inf->addText(InfoType::ConfigReset, INFO_TIME, Message::Reset);
				draw->AddInfo(InfoType::Volume, INFO_TIME, config->FontColor, Message::Volume, config->Volume);
				draw->AddInfo(InfoType::AutoPlay, INFO_TIME, config->FontColor, Message::AutoPlay, Message::AutoPlaySwitch[config->AutoPlay]);
				draw->AddInfo(InfoType::SkipVoice, INFO_TIME, config->FontColor, Message::SkipVoice, Message::Switch[config->SkipVoice]);
				draw->AddInfo(InfoType::DisableDialogSE, INFO_TIME, config->FontColor, Message::DisableDialogSE, Message::Switch[config->DisableDialogSE]);
				draw->AddInfo(InfoType::DisableDududu, INFO_TIME, config->FontColor, Message::DisableDududu, Message::Switch[config->DisableDududu]);
				draw->AddInfo(InfoType::InfoOnoff, INFO_TIME, config->FontColor, Message::ShowInfo, Message::ShowInfoSwitch[config->ShowInfo]);

				if (config->ShowInfo == Config::ShowInfo_WithMark && isAutoPlaying()) {
					draw->AddInfo(InfoType::AutoPlayMark, INFINITY_TIME, config->FontColor, Message::AutoPlayMark);
				}
				else {
					draw->RemoveInfo(InfoType::AutoPlayMark);
				}
			}
			else {
				draw->RemoveInfo(InfoType::All);
			}
		}
	} //if(KEY_VOLUME_UP & KEY_VOLUME_DOWN)
	else {
		if (keys[KEY_VOLUME_UP] && !last[KEY_VOLUME_UP - KEY_MIN] && !keys[KEY_VOLUME_DOWN] && !keys[KEY_VOLUME_0]) {
			if (keys[KEY_VOLUME_BIGSTEP1] || keys[KEY_VOLUME_BIGSTEP2]) config->Volume += VOLUME_STEP_BIG;
			else config->Volume += VOLUME_STEP;

			if (config->Volume > Config::MAX_Volume) config->Volume = Config::MAX_Volume;
			status->mute = 0;
			needsetvolume = volume_old != config->Volume;
			needsave = needsetvolume;

			if (config->ShowInfo) {
				draw->AddInfo(InfoType::Volume, INFO_TIME, config->FontColor, Message::Volume, config->Volume);
			}

			LOG("Set Volume : %d", config->Volume);
		} //if(KEY_VOLUME_UP)
		else if (keys[KEY_VOLUME_DOWN] && !last[KEY_VOLUME_DOWN - KEY_MIN] && !keys[KEY_VOLUME_UP] && !keys[KEY_VOLUME_0]) {
			if (keys[KEY_VOLUME_BIGSTEP1] || keys[KEY_VOLUME_BIGSTEP2]) config->Volume -= VOLUME_STEP_BIG;
			else config->Volume -= VOLUME_STEP;

			if (config->Volume < 0) config->Volume = 0;
			status->mute = 0;
			needsetvolume = volume_old != config->Volume;
			needsave = needsetvolume;

			if (config->ShowInfo) {
				draw->AddInfo(InfoType::Volume, INFO_TIME, config->FontColor, Message::Volume, config->Volume);
			}

			LOG("Set Volume : %d", config->Volume);
		}//if(KEY_VOLUME_DOWN)
		else if (keys[KEY_VOLUME_0] && !last[KEY_VOLUME_0 - KEY_MIN] && !keys[KEY_VOLUME_UP] && !keys[KEY_VOLUME_DOWN]) {
			status->mute = 1 - status->mute;
			needsetvolume = true;

			if (config->ShowInfo && status->mute) {
				draw->AddInfo(InfoType::Volume, INFO_TIME, config->FontColor, Message::Mute);
			}
			else {
				draw->AddInfo(InfoType::Volume, INFO_TIME, config->FontColor, Message::Volume, config->Volume);
			}

			LOG("Set mute : %d", status->mute);
		}//if(KEY_VOLUME_0)

		if (keys[KEY_AUTOPLAY] && !last[KEY_AUTOPLAY - KEY_MIN]) {
			(config->AutoPlay += 1) %= (Config::MAX_AutoPlay + 1);
			needsave = true;

			if (config->ShowInfo) {
				draw->AddInfo(InfoType::AutoPlay, INFO_TIME, config->FontColor, Message::AutoPlay, Message::AutoPlaySwitch[config->AutoPlay]);
				if (config->ShowInfo == Config::ShowInfo_WithMark && isAutoPlaying()) {
					draw->AddInfo(InfoType::AutoPlayMark, INFINITY_TIME, config->FontColor, Message::AutoPlayMark);
				}
				else {
					draw->RemoveInfo(InfoType::AutoPlayMark);
				}
			}
			LOG("Set AutoPlay : %d", config->AutoPlay);

			if (config->AutoPlay && !config->SkipVoice) {
				config->SkipVoice = 1;
				if (status->playing) {
					order->skipVoice = config->SkipVoice;
				}
				if (config->ShowInfo) {
					draw->AddInfo(InfoType::SkipVoice, INFO_TIME, config->FontColor, Message::SkipVoice, Message::Switch[config->SkipVoice]);
				}
				LOG("Set SkipVoice : %d", config->SkipVoice);
			}
		}//if(KEY_AUTOPLAY)

		if (keys[KEY_SKIPVOICE] && !last[KEY_SKIPVOICE - KEY_MIN]) {
			config->SkipVoice = 1 - config->SkipVoice;
			needsave = true;
			if (status->playing) {
				order->skipVoice = config->SkipVoice;
			}

			if (config->ShowInfo) {
				draw->AddInfo(InfoType::SkipVoice, INFO_TIME, config->FontColor, Message::SkipVoice, Message::Switch[config->SkipVoice]);
			}

			LOG("Set SkipVoice : %d", config->SkipVoice);

			if (!config->SkipVoice && config->AutoPlay) {
				config->AutoPlay = 0;
				if (config->ShowInfo) {
					draw->AddInfo(InfoType::AutoPlay, INFO_TIME, config->FontColor, Message::AutoPlay, Message::AutoPlaySwitch[config->AutoPlay]);
					draw->RemoveInfo(InfoType::AutoPlayMark);
				}
				LOG("Set AutoPlay : %d", config->AutoPlay);
			}
		}//if(KEY_SKIPVOICE)

		if (keys[KEY_DLGSE] && !last[KEY_DLGSE - KEY_MIN]) {
			config->DisableDialogSE = 1 - config->DisableDialogSE;
			if (status->playing) {
				order->disableDialogSE = config->DisableDialogSE;
			}
			needsave = true;

			if (config->ShowInfo) {
				draw->AddInfo(InfoType::DisableDialogSE, INFO_TIME, config->FontColor, Message::DisableDialogSE, Message::Switch[config->DisableDialogSE]);
			}

			LOG("Set DisableDialogSE : %d", config->DisableDialogSE);
		}//if(KEY_DLGSE)

		if (keys[KEY_DU] && !last[KEY_DU - KEY_MIN]) {
			config->DisableDududu = 1 - config->DisableDududu;
			if (status->playing) {
				order->disableDududu = config->DisableDududu;
			}
			needsave = true;

			if (config->ShowInfo) {
				draw->AddInfo(InfoType::DisableDududu, INFO_TIME, config->FontColor, Message::DisableDududu, Message::Switch[config->DisableDududu]);
			}

			LOG("Set DisableDududu : %d", config->DisableDududu);
		}//if(KEY_DU)

		if (keys[KEY_INFO] && !last[KEY_INFO - KEY_MIN]) {
			config->ShowInfo = (config->ShowInfo + 1) % (Config::MAX_ShowInfo + 1);
			needsave = true;

			if (config->ShowInfo) {
				if (config->ShowInfo == Config::ShowInfo_WithMark && isAutoPlaying()) {
					draw->AddInfo(InfoType::AutoPlayMark, INFINITY_TIME, config->FontColor, Message::AutoPlayMark);
				}
				else {
					draw->RemoveInfo(InfoType::AutoPlayMark);
				}
			}
			else {
				draw->RemoveInfo(InfoType::All);
			}
			draw->AddInfo(InfoType::InfoOnoff, INFO_TIME, config->FontColor, Message::ShowInfo, Message::ShowInfoSwitch[config->ShowInfo]);

			LOG("Set ShowInfo : %d", config->ShowInfo);
		}//if(KEY_INFO)
	}

	if (needsetvolume) {
		if (status->playing) {
			if (status->mute) player->SetVolume(config->Volume);
			else player->SetVolume(0);
		}
	}

	if (needsave && config->SaveChange) {
		config->SaveConfig(CONFIG_FILE);
		LOG("Config file saved");
	}

	memcpy(last, keys + KEY_MIN, KEYS_NUM);
}

void SoraVoiceImpl::Show()
{
	Clock::UpdateTime();

	if (status->showing) {
		draw->DrawInfos();
		draw->RemoveInfo(InfoType::Dead);
	}

	if (aup->count_ch == 1) {
		aup->count_ch++;
		if (config->ShowInfo == Config::ShowInfo_WithMark && isAutoPlaying()) {
			draw->AddInfo(InfoType::AutoPlayMark, Draw::ShowTimeInfinity, config->FontColor ,Message::AutoPlayMark);
		}
	}

	if (!status->playing) {
		if (aup->wait 
			&& status->scode != SCODE_SAY && status->scode != SCODE_TALK && status->scode != SCODE_TEXT) {
			aup->count_ch = 0;
			aup->wait = 0;
			aup->waitv = 0;
			aup->time_autoplay = 0;

			if (config->ShowInfo == Config::ShowInfo_WithMark) {
				draw->RemoveInfo(InfoType::AutoPlayMark);
			}
		}
		else if (aup->wait && !aup->time_autoplay) {
			aup->time_autoplay = aup->time_textbeg
				+ (aup->count_ch - 1) * config->WaitTimePerChar + config->WaitTimeDialog - TIME_PREC / 2;
		}

		if ((aup->waitv && aup->time_autoplayv <= aup->now)
			|| (!aup->waitv && aup->wait && aup->time_autoplay <= aup->now)) {
			LOG("now = %d", aup->now);
			LOG("waitv = %d", aup->waitv);
			LOG("autoplayv = %d", aup->time_autoplayv);

			LOG("wait = %d", aup->wait);
			LOG("time_textbeg = %d", aup->time_textbeg);
			LOG("cnt = %d", aup->count_ch - 1);
			LOG("autoplay = %d", aup->time_autoplay);

			if (isAutoPlaying()) {
				if (!status->code5) {
					order->autoPlay = 1;
					LOG("Auto play set.");

					SetThreadExecutionState(ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
				}

				if (config->ShowInfo == Config::ShowInfo_WithMark) {
					draw->AddInfo(InfoType::AutoPlayMark, REMAIN_TIME, config->FontColor, Message::AutoPlayMark);
				}
			}

			aup->count_ch = 0;
			aup->wait = 0;
			aup->waitv = 0;
			aup->time_autoplay = 0;
		}
	}
}

SoraVoice * SoraVoice::CreateInstance(void * initParam)
{
	InitParam* ip = (InitParam*)initParam;

	LOG("p = 0x%08X", ip);
	LOG("p->p_ov_open_callbacks = 0x%08X", ip->addrs.p_ov_open_callbacks);
	LOG("p->p_ov_info = 0x%08X", ip->addrs.p_ov_info);
	LOG("p->p_ov_read = 0x%08X", ip->addrs.p_ov_read);
	LOG("p->p_ov_clear = 0x%08X", ip->addrs.p_ov_clear);
	LOG("p->p_d3dd = 0x%08X", ip->addrs.p_d3dd);
	LOG("p->p_did = 0x%08X", ip->addrs.p_did);
	LOG("p->p_Hwnd = 0x%08X", ip->addrs.p_Hwnd);
	LOG("p->p_pDS = 0x%08X", ip->addrs.p_pDS);
	LOG("p->p_mute = 0x%08X", ip->addrs.p_mute);
	LOG("p->p_keys = 0x%08X", ip->addrs.p_keys);

	LOG("ov_open_callbacks = 0x%08X", *ip->addrs.p_ov_open_callbacks);
	LOG("ov_info = 0x%08X", *ip->addrs.p_ov_info);
	LOG("ov_read = 0x%08X", *ip->addrs.p_ov_read);
	LOG("ov_clear = 0x%08X", *ip->addrs.p_ov_clear);
	LOG("d3dd = 0x%08X", *ip->addrs.p_d3dd);
	LOG("did = 0x%08X", *ip->addrs.p_did);
	LOG("Hwnd = 0x%08X", *ip->addrs.p_Hwnd);
	LOG("pDS = 0x%08X", *ip->addrs.p_pDS);

	if (!*ip->addrs.p_ov_open_callbacks || !*ip->addrs.p_ov_info || !*ip->addrs.p_ov_read || !*ip->addrs.p_ov_clear
		|| !*ip->addrs.p_pDS) {
		LOG("nullptr exists, failed to init SoraVoice");
		return nullptr;
	}

	Clock::InitClock(ip->rcd.now, ip->rcd.recent);
	return new SoraVoiceImpl((InitParam*)initParam);
}

void SoraVoice::DestoryInstance(SoraVoice * sv)
{
	delete sv;
}

