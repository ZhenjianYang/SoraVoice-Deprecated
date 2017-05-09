
#include "SoraVoiceImpl.h"

#include "Log.h"
#include "Message.h"
#include "Clock.h"
#include "Hook.h"
#ifndef ZA
#include "Mapping.h"
#endif

#include <string>

#ifndef MAX_VOICEID_LEN
#define MAX_VOICEID_LEN 10
#endif // !MAX_VOICEID_LEN

#ifndef MAX_VOICEID_LEN_NEED_MAPPING
#define MAX_VOICEID_LEN_NEED_MAPPING 5
#endif // !MAX_VOICEID_LEN_NEED_MAPPING

constexpr int ORIVOICEID_LEN = 4;

#ifdef ZA
constexpr char ORIVOICEFILE_PREFIX[] = "data\\se\\ed7v";
constexpr char ORIVOICEFILE_ATTR[] = ".wav";
constexpr char CONFIG_FILE[] = "za_voice.ini";
constexpr char VOICEFILE_PREFIX[] = "voice\\ogg\\v";
#else
constexpr char CONFIG_FILE[] = "ed_voice.ini";
constexpr char VOICEFILE_PREFIX[] = "voice\\ogg\\ch";
#endif
constexpr char VOICEFILE_ATTR[] = ".ogg";


constexpr int VOLUME_STEP = 1;
constexpr int VOLUME_STEP_BIG = 5;

constexpr int KEY_VOLUME_UP = DIK_EQUALS;
constexpr int KEY_VOLUME_DOWN = DIK_MINUS;
constexpr int KEY_VOLUME_BIGSTEP1 = DIK_LSHIFT;
constexpr int KEY_VOLUME_BIGSTEP2 = DIK_RSHIFT;

constexpr int KEY_ORIVOICE = DIK_BACKSPACE;
constexpr int KEY_AUTOPLAY = DIK_0;
constexpr int KEY_SKIPVOICE = DIK_9;
constexpr int KEY_DLGSE = DIK_8;
constexpr int KEY_DU = DIK_7;
constexpr int KEY_INFOONOFF = DIK_6;
constexpr int KEY_ALLINFO = DIK_BACKSLASH;
constexpr int KEY_RESETA = DIK_LBRACKET;
constexpr int KEY_RESETB = DIK_RBRACKET;

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

constexpr unsigned INFO_TIME = 3000;
constexpr unsigned HELLO_TIME = 8000;
constexpr unsigned INFINITY_TIME = Draw::ShowTimeInfinity;
constexpr unsigned REMAIN_TIME = 2000;

constexpr unsigned TIME_PREC = 16;

void SoraVoiceImpl::stopCallBack(PlayID playID, StopType stopType)
{
	LOG("StopCallBack: playID = 0x%08d, stopType = %d", playID, stopType);
	LockGuard lock(mt_playID);
	if (playID == this->playID) {
		if (stopType == StopType::PlayEnd) {
			aup->waitv = 1;
			aup->time_autoplayv = aup->now + config->WaitTimeDialogVoice - TIME_PREC / 2;
		}
		else {
			order->disableDududu = 0;
			order->disableDialogSE = 0;
		}
		status->playing = 0;
	} //if (playID == this->playID)
}

SoraVoiceImpl::SoraVoiceImpl(InitParam* initParam)
	:
	_config(CONFIG_FILE),
	_keys(initParam->addrs.p_keys, *initParam->addrs.p_did),
	_aup(initParam->rcd.now, initParam->rcd.count_ch, initParam->status.wait, initParam->rcd.time_textbeg, initParam->status.waitv),
	Comment(initParam->Comment),
	ended(initParam->status.ended), status(&initParam->status), order(&initParam->order), 
	draw(Draw::CreateDraw(initParam->status.showing, *initParam->addrs.p_Hwnd, *initParam->addrs.p_d3dd, _config.FontName)),
	player(SoundPlayer::CreatSoundPlayer(*initParam->addrs.p_pDS, 
		std::bind(&SoraVoiceImpl::stopCallBack, this, std::placeholders::_1, std::placeholders::_2)))
{
	LOG("Config loaded");
	LOG("config.Volume = %d", config->Volume);
	LOG("config.OriginalVoice = %d", config->OriginalVoice);
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

	static_assert(Config::MAX_Volume == SoundPlayer::MaxVolume, "Max Volume not same!");

	InitHook_SetInitParam(initParam);

	draw->AddInfo(InfoType::Hello, INFINITY_TIME, 0, Message::Mute);
	draw->AddInfo(InfoType::Hello, INFINITY_TIME, 0, Message::Volume, 1234567890);
	draw->AddInfo(InfoType::Hello, INFINITY_TIME, 0, Message::OriginalVoice, Message::Switch[0]);
	draw->AddInfo(InfoType::Hello, INFINITY_TIME, 0, Message::AutoPlay, Message::Switch[1]);
	draw->AddInfo(InfoType::Hello, INFINITY_TIME, 0, Message::SkipVoice, Message::ShowInfoSwitch[2]);
	draw->AddInfo(InfoType::Hello, INFINITY_TIME, 0, Message::DisableDialogSE, Message::AutoPlaySwitch[1]);
	draw->AddInfo(InfoType::Hello, INFINITY_TIME, 0, Message::DisableDududu, Message::AutoPlaySwitch[2]);
	draw->AddInfo(InfoType::Hello, INFINITY_TIME, 0, Message::ShowInfo, Message::OriginalVoiceSwitch[0]);
	draw->AddInfo(InfoType::Hello, INFINITY_TIME, 0, Message::OriginalVoiceSwitch[1]);
	draw->AddInfo(InfoType::Hello, INFINITY_TIME, 0, Message::OriginalVoiceSwitch[2]);
	draw->AddInfo(InfoType::Hello, INFINITY_TIME, 0, Message::AutoPlayMark);
	draw->DrawInfos();
	draw->RemoveInfo(InfoType::All);

	
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
	if (*t != 'v' || str_vid.empty()) return;

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

#ifdef ZA
	if (config->OriginalVoice) {
		++t;
		while (*t == '#') {
			t++;
			const char* p = t;
			while (*p >= '0' && *p <= '9') p++;
			if (*p == 'V' && p - t == ORIVOICEID_LEN) {
				if (config->OriginalVoice == Config::OriginalVoice_OriOnly) {
					oggFileName = ORIVOICEFILE_PREFIX + str_vid.assign(t, ORIVOICEID_LEN) + ORIVOICEFILE_ATTR;
				}
				*(unsigned*)t = 0x39393939;
			}
			t = p + 1;
		}
	}
#endif // ZA
	LOG("Sound filename: %s", oggFileName.c_str());

	LOG("Now playing new file...");
	{
		LockGuard lock(mt_playID);

		status->playing = 1;
		playID = player->Play(oggFileName.c_str(), status->mute ? 0 : config->Volume);
	}

	order->disableDududu = config->DisableDududu;
	order->disableDialogSE = config->DisableDialogSE;

	LOG("Play called, playID = 0x%08X", playID);
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

	aup->wait = 0;
	aup->waitv = 0;
	aup->count_ch = 0;
	aup->time_autoplay = 0;
}

void SoraVoiceImpl::Input()
{
	if (!config->EnableKeys || !this->keys->keys) return;

	auto keys = this->keys->keys;
	auto last = this->keys->last;

	bool needsave = false;
	bool needsetvolume = false;
	int volume_old = config->Volume;

	unsigned info_time = INFO_TIME;

	if (keys[KEY_ALLINFO]) {
		info_time = INFINITY_TIME;
		if (!last[KEY_ALLINFO]) {
			draw->RemoveInfo(InfoType::Volume);
#ifdef ZA
			draw->RemoveInfo(InfoType::OriginalVoice);
#endif // ZA
			draw->RemoveInfo(InfoType::AutoPlay);
			draw->RemoveInfo(InfoType::SkipVoice);
			draw->RemoveInfo(InfoType::DisableDialogSE);
			draw->RemoveInfo(InfoType::DisableDududu);
			draw->RemoveInfo(InfoType::InfoOnoff);

			if(status->mute) draw->AddInfo(InfoType::Volume, INFINITY_TIME, config->FontColor, Message::Mute);
			else draw->AddInfo(InfoType::Volume, INFINITY_TIME, config->FontColor, Message::Volume, config->Volume);
#ifdef ZA
			draw->AddInfo(InfoType::OriginalVoice, INFINITY_TIME, config->FontColor, Message::OriginalVoice, Message::OriginalVoiceSwitch[config->OriginalVoice]);
#endif // ZA
			draw->AddInfo(InfoType::AutoPlay, INFINITY_TIME, config->FontColor, Message::AutoPlay, Message::AutoPlaySwitch[config->AutoPlay]);
			draw->AddInfo(InfoType::SkipVoice, INFINITY_TIME, config->FontColor, Message::SkipVoice, Message::Switch[config->SkipVoice]);
			draw->AddInfo(InfoType::DisableDialogSE, INFINITY_TIME, config->FontColor, Message::DisableDialogSE, Message::Switch[config->DisableDialogSE]);
			draw->AddInfo(InfoType::DisableDududu, INFINITY_TIME, config->FontColor, Message::DisableDududu, Message::Switch[config->DisableDududu]);
			draw->AddInfo(InfoType::InfoOnoff, INFINITY_TIME, config->FontColor, Message::ShowInfo, Message::ShowInfoSwitch[config->ShowInfo]);
		}
	}
	else if (last[KEY_ALLINFO]) {
		draw->RemoveInfo(InfoType::Volume);
#ifdef ZA
		draw->RemoveInfo(InfoType::OriginalVoice);
#endif // ZA
		draw->RemoveInfo(InfoType::AutoPlay);
		draw->RemoveInfo(InfoType::SkipVoice);
		draw->RemoveInfo(InfoType::DisableDialogSE);
		draw->RemoveInfo(InfoType::DisableDududu);
		draw->RemoveInfo(InfoType::InfoOnoff);
	}//keys[KEY_ALLINFO]

	if ((keys[KEY_RESETA] && keys[KEY_RESETB])
		&& !(last[KEY_RESETA] && last[KEY_RESETB])) {
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
			order->disableDialogSE = config->DisableDialogSE;
			order->disableDududu = config->DisableDududu;
		}
		LOG("Reset config");

		if (config->ShowInfo || info_time == INFINITY_TIME) {
			//draw->AddText(InfoType::ConfigReset, INFO_TIME, config->FontColor, Message::Reset);
			draw->AddInfo(InfoType::Volume, info_time, config->FontColor, Message::Volume, config->Volume);
#ifdef ZA
			draw->AddInfo(InfoType::OriginalVoice, info_time, config->FontColor, Message::OriginalVoice, Message::OriginalVoiceSwitch[config->OriginalVoice]);
#endif // ZA
			draw->AddInfo(InfoType::AutoPlay, info_time, config->FontColor, Message::AutoPlay, Message::AutoPlaySwitch[config->AutoPlay]);
			draw->AddInfo(InfoType::SkipVoice, info_time, config->FontColor, Message::SkipVoice, Message::Switch[config->SkipVoice]);
			draw->AddInfo(InfoType::DisableDialogSE, info_time, config->FontColor, Message::DisableDialogSE, Message::Switch[config->DisableDialogSE]);
			draw->AddInfo(InfoType::DisableDududu, info_time, config->FontColor, Message::DisableDududu, Message::Switch[config->DisableDududu]);
			draw->AddInfo(InfoType::InfoOnoff, info_time, config->FontColor, Message::ShowInfo, Message::ShowInfoSwitch[config->ShowInfo]);

			if (config->ShowInfo == Config::ShowInfo_WithMark && isAutoPlaying()) {
				draw->AddInfo(InfoType::AutoPlayMark, INFINITY_TIME, config->FontColor, Message::AutoPlayMark);
			}
			else {
				draw->RemoveInfo(InfoType::AutoPlayMark);
			}
		}
		else if (info_time != INFINITY_TIME) {
			draw->RemoveInfo(InfoType::All);
		}
		else {
			draw->RemoveInfo(InfoType::AutoPlayMark);
			draw->RemoveInfo(InfoType::Hello);
		}
	} //keys[KEY_RESETA] && keys[KEY_RESETB]
	else {
		bool show_info = config->ShowInfo || info_time == INFINITY_TIME;
		if (keys[KEY_VOLUME_UP] && !last[KEY_VOLUME_UP] && !keys[KEY_VOLUME_DOWN]) {
			if (keys[KEY_VOLUME_BIGSTEP1] || keys[KEY_VOLUME_BIGSTEP2]) config->Volume += VOLUME_STEP_BIG;
			else config->Volume += VOLUME_STEP;

			if (config->Volume > Config::MAX_Volume) config->Volume = Config::MAX_Volume;
			status->mute = 0;
			needsetvolume = volume_old != config->Volume;
			needsave = needsetvolume;

			if (show_info) {
				draw->AddInfo(InfoType::Volume, info_time, config->FontColor, Message::Volume, config->Volume);
			}

			LOG("Set Volume : %d", config->Volume);
		} //if(KEY_VOLUME_UP)
		else if (keys[KEY_VOLUME_DOWN] && !last[KEY_VOLUME_DOWN] && !keys[KEY_VOLUME_UP]) {
			if (keys[KEY_VOLUME_BIGSTEP1] || keys[KEY_VOLUME_BIGSTEP2]) config->Volume -= VOLUME_STEP_BIG;
			else config->Volume -= VOLUME_STEP;

			if (config->Volume < 0) config->Volume = 0;
			status->mute = 0;
			needsetvolume = volume_old != config->Volume;
			needsave = needsetvolume;

			if (show_info) {
				draw->AddInfo(InfoType::Volume, info_time, config->FontColor, Message::Volume, config->Volume);
			}

			LOG("Set Volume : %d", config->Volume);
		}//if(KEY_VOLUME_DOWN)
		else if (keys[KEY_VOLUME_UP] && keys[KEY_VOLUME_DOWN] && !(last[KEY_VOLUME_UP] && last[KEY_VOLUME_DOWN])) {
			status->mute = 1;
			needsetvolume = true;

			if (show_info) {
				draw->AddInfo(InfoType::Volume, info_time, config->FontColor, Message::Mute);
			}

			LOG("Set mute : %d", status->mute);
		}//keys[KEY_VOLUME_UP] && keys[KEY_VOLUME_DOWN]

#ifdef ZA
		if (keys[KEY_ORIVOICE] && !last[KEY_ORIVOICE]) {
			(config->OriginalVoice += 1) %= (Config::MAX_OriginalVoice + 1);
			needsave = true;

			if (show_info) {
				draw->AddInfo(InfoType::OriginalVoice, info_time, config->FontColor, Message::OriginalVoice,
					Message::OriginalVoiceSwitch[config->OriginalVoice]);
			}

			LOG("Set OriginalVoice : %d", config->OriginalVoice);
		}//if(KEY_ORIVOICE)
#endif // ZA

		if (keys[KEY_AUTOPLAY] && !last[KEY_AUTOPLAY]) {
			(config->AutoPlay += 1) %= (Config::MAX_AutoPlay + 1);
			needsave = true;

			if (show_info) {
				draw->AddInfo(InfoType::AutoPlay, info_time, config->FontColor, Message::AutoPlay, Message::AutoPlaySwitch[config->AutoPlay]);
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

				if (show_info) {
					draw->AddInfo(InfoType::SkipVoice, info_time, config->FontColor, Message::SkipVoice, Message::Switch[config->SkipVoice]);
				}
				LOG("Set SkipVoice : %d", config->SkipVoice);
			}
		}//if(KEY_AUTOPLAY)

		if (keys[KEY_SKIPVOICE] && !last[KEY_SKIPVOICE]) {
			config->SkipVoice = 1 - config->SkipVoice;
			needsave = true;

			if (show_info) {
				draw->AddInfo(InfoType::SkipVoice, info_time, config->FontColor, Message::SkipVoice, Message::Switch[config->SkipVoice]);
			}

			LOG("Set SkipVoice : %d", config->SkipVoice);

			if (!config->SkipVoice && config->AutoPlay) {
				config->AutoPlay = 0;
				if (show_info) {
					draw->AddInfo(InfoType::AutoPlay, info_time, config->FontColor, Message::AutoPlay, Message::AutoPlaySwitch[config->AutoPlay]);
				}
				draw->RemoveInfo(InfoType::AutoPlayMark);
				LOG("Set AutoPlay : %d", config->AutoPlay);
			}
		}//if(KEY_SKIPVOICE)

		if (keys[KEY_DLGSE] && !last[KEY_DLGSE]) {
			config->DisableDialogSE = 1 - config->DisableDialogSE;
			if (status->playing) {
				order->disableDialogSE = config->DisableDialogSE;
			}
			needsave = true;

			if (show_info) {
				draw->AddInfo(InfoType::DisableDialogSE, info_time, config->FontColor, Message::DisableDialogSE, Message::Switch[config->DisableDialogSE]);
			}

			LOG("Set DisableDialogSE : %d", config->DisableDialogSE);
		}//if(KEY_DLGSE)

		if (keys[KEY_DU] && !last[KEY_DU]) {
			config->DisableDududu = 1 - config->DisableDududu;
			if (status->playing) {
				order->disableDududu = config->DisableDududu;
			}
			needsave = true;

			if (show_info) {
				draw->AddInfo(InfoType::DisableDududu, info_time, config->FontColor, Message::DisableDududu, Message::Switch[config->DisableDududu]);
			}

			LOG("Set DisableDududu : %d", config->DisableDududu);
		}//if(KEY_DU)

		if (keys[KEY_INFOONOFF] && !last[KEY_INFOONOFF]) {
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
			else if (info_time == INFINITY_TIME) {
				draw->RemoveInfo(InfoType::AutoPlayMark);
				draw->RemoveInfo(InfoType::Hello);
			}
			else { 
				draw->RemoveInfo(InfoType::All);
			}
			draw->AddInfo(InfoType::InfoOnoff, info_time, config->FontColor, Message::ShowInfo, Message::ShowInfoSwitch[config->ShowInfo]);

			LOG("Set ShowInfo : %d", config->ShowInfo);
		}//if(KEY_INFO)
	}

	if (needsetvolume) {
		if (status->playing) {
			if (!status->mute) player->SetVolume(config->Volume);
			else player->SetVolume(0);
		}
	}

	if (needsave && config->SaveChange) {
		config->SaveConfig(CONFIG_FILE);
		LOG("Config file saved");
	}

	memcpy(last, keys, KEYS_NUM);
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

			order->disableDududu = 0;
		}
		else if (aup->wait && !aup->time_autoplay) {
			aup->time_autoplay = aup->time_textbeg
				+ (aup->count_ch - 1) * config->WaitTimePerChar + config->WaitTimeDialog - TIME_PREC / 2;

			order->disableDududu = 0;
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
				order->autoPlay = 1;
				LOG("Auto play set.");

				SetThreadExecutionState(ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);

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
	return new SoraVoiceImpl((InitParam*)initParam);
}

void SoraVoice::DestoryInstance(SoraVoice * sv)
{
	delete sv;
}
