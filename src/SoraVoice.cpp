
#include "SoraVoice.h"

#include <SVData.h>

#include <Utils/Log.h>
#include <Utils/Clock.h>
#include <Utils/build_date.h>

#include <Message.h>
#include <Mapping.h>
#include <Config.h>

#include <Hook/Hook.h>
#include <Player/Player.h>
#include <Draw/Draw.h>

#include <string>
#include <vector>
#include <mutex>
#include <random>
#include <ctime>
#include <cstring>

#include <dinput.h>
#ifndef DID
#define DID IDirectInputDevice
#endif // !DID

using LockGuard = std::lock_guard<std::mutex>;
using Draw::InfoType;
using Player::PlayID;
using Player::StopType;
using byte = unsigned char;

static const char DateVersion[] = BUILD_DATE;

constexpr int ORIVOICEID_LEN = 4;

constexpr char ORIVOICEFILE_PREFIX[] = "data\\se\\ed7v";
constexpr char ORIVOICEFILE_ATTR[] = ".wav";
constexpr char CONFIG_FILE[] = "voice\\ed_voice.ini";
constexpr char VOICEFILE_PREFIX_ZA[] = "voice\\ogg\\v";
constexpr char VOICEFILE_PREFIX_ED6[] = "voice\\ogg\\ch";
constexpr char VOICEFILE_ATTR[] = ".ogg";

constexpr int VOLUME_STEP = 1;
constexpr int VOLUME_STEP_BIG = 5;

constexpr int KEY_VOLUME_UP = DIK_EQUALS;
constexpr int KEY_VOLUME_DOWN = DIK_MINUS;
constexpr int KEY_VOLUME_BIGSTEP1 = DIK_LSHIFT;
constexpr int KEY_VOLUME_BIGSTEP2 = DIK_RSHIFT;

constexpr int ORIVOLPCT_STEP = 10;
constexpr int KEY_ORIVOLPCT_UP = DIK_RBRACKET;
constexpr int KEY_ORIVOLPCT_DOWN = DIK_LBRACKET;

constexpr int KEY_ORIVOICE = DIK_BACKSPACE;
constexpr int KEY_AUTOPLAY = DIK_0;
constexpr int KEY_SKIPVOICE = DIK_9;
constexpr int KEY_DLGSE = DIK_8;
constexpr int KEY_DU = DIK_7;
constexpr int KEY_INFOONOFF = DIK_6;
constexpr int KEY_ALLINFO = DIK_BACKSLASH;
constexpr int KEY_RESETA = DIK_LBRACKET;
constexpr int KEY_RESETB = DIK_RBRACKET;

constexpr unsigned INFO_TIME = 3000;
constexpr unsigned HELLO_TIME = 8000;
constexpr unsigned INFINITY_TIME = Draw::ShowTimeInfinity;
constexpr unsigned REMAIN_TIME = 2000;

constexpr unsigned TIME_PREC = 16;

constexpr int KEYS_NUM = 256;

static struct Keys {
	const byte* const &keys;
	DID* const pDID;
	byte last[KEYS_NUM];
	Keys(const byte* &keys, void* pDID)
		:keys(keys), pDID((decltype(this->pDID))pDID) {
	}
} * keys;

static struct AutoPlay {
	const unsigned &now;

	unsigned &count_ch;
	unsigned &wait;
	unsigned &time_textbeg;
	unsigned time_autoplay = 0;

	unsigned &waitv;
	unsigned time_autoplayv = 0;

	AutoPlay(unsigned& now, unsigned &count_ch,
			unsigned &wait, unsigned &time_textbeg,
			unsigned &waitv)
	:now(now),
	count_ch(count_ch), wait(wait), time_textbeg(time_textbeg),
	waitv(waitv) {
	}
} * aup;

static std::mutex mt_playID;
static PlayID curPlayID;

static bool isZa;
static const char *VOICEFILE_PREFIX;

static void playRandomVoice(const char* vlist) {
	if (!vlist) return;

	std::vector<const char*> vl;
	while (*vlist) {
		vl.push_back(vlist);
		while (*vlist) vlist++;
		vlist++;
	}

	LOG("Random voice num : %d", vl.size());
	if (!vl.empty()) {
		std::default_random_engine random((unsigned)std::time(nullptr));
		std::uniform_int_distribution<int> dist(0, vl.size() - 1);

		LOG("Play Random voice.");
		Player::Play(vl[dist(random)], config.Volume);
	}
}

inline static bool isAutoPlaying() {
	return aup->count_ch
		&& ((config.AutoPlay && (sv.status.playing || aup->waitv))
			|| config.AutoPlay == Config::AutoPlay_ALL);
}

inline static std::string GetStr(const char* first) {
	return first;
}

inline static std::string GetStr(char* first) {
	return first;
}

template<typename First, typename = std::enable_if_t<std::is_integral_v<First>>>
inline static std::string GetStr(First first) {
	return std::to_string(first);
}

template<typename First, typename... Remain>
inline static std::string GetStr(First first, Remain... remains) {
	return GetStr(first) + GetStr(remains...);
}

template<typename... Texts>
inline static void AddInfo(InfoType type, unsigned time, unsigned color, Texts... texts) {
	return Draw::AddInfo(type, time, color, GetStr(texts...).c_str());
}

static void stopCallBack(PlayID playID, StopType stopType)
{
	LOG("StopCallBack: playID = 0x%08d, stopType = %d", playID, stopType);
	LockGuard lock(mt_playID);
	if (playID == curPlayID) {
		if (stopType == StopType::PlayEnd) {
			aup->waitv = 1;
			aup->time_autoplayv = aup->now + config.WaitTimeDialogVoice - TIME_PREC / 2;
		}
		else {
			sv.order.disableDududu = 0;
			sv.order.disableDialogSE = 0;
		}
		sv.status.playing = 0;
	} //if (playID == this->playID)
}


void SoraVoice::Play(const char* t)
{
	if (!sv.status.startup) return;

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
		if(isZa) {
			LOG("Voice ID mapping is not supported in Zero/Ao, return");
			return;
		}

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
	}

	std::string oggFileName = VOICEFILE_PREFIX + str_vid + VOICEFILE_ATTR;
	int volume = config.Volume;

	if(isZa) {
		sv.status.playingOri = 0;
		if (config.OriginalVoice) {
			++t;
			while (*t == '#') {
				t++;
				const char* p = t;
				while (*p >= '0' && *p <= '9') p++;
				if (*p == 'V' && p - t == ORIVOICEID_LEN) {
					if (config.OriginalVoice == Config::OriginalVoice_OriOnly) {
						oggFileName = ORIVOICEFILE_PREFIX + str_vid.assign(t, ORIVOICEID_LEN) + ORIVOICEFILE_ATTR;
						sv.status.playingOri = 1;
						volume = config.Volume * config.OriVolumePercent / 100;
						if (volume > config.MAX_Volume) volume = config.MAX_Volume;
					}
					*(unsigned*)t = 0x39393939;
				}
				t = p + 1;
			}
		}
	}

	LOG("Sound filename: %s", oggFileName.c_str());

	LOG("Now playing new file...");
	{
		LockGuard lock(mt_playID);

		sv.status.playing = 1;
		curPlayID = Player::Play(oggFileName.c_str(), sv.status.mute ? 0 : volume);
	}

	sv.order.disableDududu = config.DisableDududu;
	sv.order.disableDialogSE = config.DisableDialogSE;

	LOG("Play called, playID = 0x%08X", curPlayID);
}

void SoraVoice::Stop()
{
	if (!sv.status.startup) return;

	LOG("Stop is called.");

	if (config.ShowInfo == Config::ShowInfo_WithMark && isAutoPlaying()) {
		AddInfo(InfoType::AutoPlayMark, REMAIN_TIME, config.FontColor, Message.AutoPlayMark);
	}

	if (config.SkipVoice) {
		Player::Stop();
	}
	sv.status.playing = 0;

	sv.order.disableDududu = 0;
	sv.order.disableDialogSE = 0;

	aup->wait = 0;
	aup->waitv = 0;
	aup->count_ch = 0;
	aup->time_autoplay = 0;
}

void SoraVoice::Input()
{
	if (!config.EnableKeys || !keys->keys) return;

	auto keys = ::keys->keys;
	auto last = ::keys->last;

	bool needsave = false;
	bool needsetvolume = false;
	int volume_old = config.Volume;

	unsigned info_time = INFO_TIME;

	if (keys[KEY_ALLINFO]) {
		info_time = INFINITY_TIME;
		if (!last[KEY_ALLINFO]) {
			Draw::RemoveInfo(InfoType::Volume);
			if(sv.game == SVData::AO) {
				Draw::RemoveInfo(InfoType::OriVolumePercent);
				Draw::RemoveInfo(InfoType::OriginalVoice);
			}

			Draw::RemoveInfo(InfoType::AutoPlay);
			Draw::RemoveInfo(InfoType::SkipVoice);
			Draw::RemoveInfo(InfoType::DisableDialogSE);
			Draw::RemoveInfo(InfoType::DisableDududu);
			Draw::RemoveInfo(InfoType::InfoOnoff);

			if(sv.status.mute) AddInfo(InfoType::Volume, INFINITY_TIME, config.FontColor, Message.Mute);
			else AddInfo(InfoType::Volume, INFINITY_TIME, config.FontColor, Message.Volume, config.Volume);

			if(sv.game == SVData::AO) {
				AddInfo(InfoType::OriVolumePercent, INFINITY_TIME, config.FontColor, Message.OriVolumePercent, config.OriVolumePercent, "%");
				AddInfo(InfoType::OriginalVoice, INFINITY_TIME, config.FontColor, Message.OriginalVoice, Message.OriginalVoiceSwitch[config.OriginalVoice]);
			}
			AddInfo(InfoType::AutoPlay, INFINITY_TIME, config.FontColor, Message.AutoPlay, Message.AutoPlaySwitch[config.AutoPlay]);
			AddInfo(InfoType::SkipVoice, INFINITY_TIME, config.FontColor, Message.SkipVoice, Message.Switch[config.SkipVoice]);
			AddInfo(InfoType::DisableDialogSE, INFINITY_TIME, config.FontColor, Message.DisableDialogSE, Message.Switch[config.DisableDialogSE]);
			AddInfo(InfoType::DisableDududu, INFINITY_TIME, config.FontColor, Message.DisableDududu, Message.Switch[config.DisableDududu]);
			AddInfo(InfoType::InfoOnoff, INFINITY_TIME, config.FontColor, Message.ShowInfo, Message.ShowInfoSwitch[config.ShowInfo]);
		}
	}
	else if (last[KEY_ALLINFO]) {
		Draw::RemoveInfo(InfoType::Volume);
		if(sv.game == SVData::AO) {
			Draw::RemoveInfo(InfoType::OriVolumePercent);
			Draw::RemoveInfo(InfoType::OriginalVoice);
		}
		Draw::RemoveInfo(InfoType::AutoPlay);
		Draw::RemoveInfo(InfoType::SkipVoice);
		Draw::RemoveInfo(InfoType::DisableDialogSE);
		Draw::RemoveInfo(InfoType::DisableDududu);
		Draw::RemoveInfo(InfoType::InfoOnoff);
	}//keys[KEY_ALLINFO]

	if ((keys[KEY_RESETA] && keys[KEY_RESETB])
		&& !(last[KEY_RESETA] && last[KEY_RESETB])) {
		if (config.SaveChange) {
			config.Reset();
			needsave = true;
			needsetvolume = true;
		}
		else {
			config.LoadConfig(CONFIG_FILE);
			config.EnableKeys = 1;
			config.SaveChange = 0;
			needsetvolume = true;
		}
		sv.status.mute = 0;
		if (sv.status.playing) {
			sv.order.disableDialogSE = config.DisableDialogSE;
			sv.order.disableDududu = config.DisableDududu;
		}
		LOG("Reset config");

		if (config.ShowInfo || info_time == INFINITY_TIME) {
			//Draw::AddText(InfoType::ConfigReset, INFO_TIME, config.FontColor, Message.Reset);
			AddInfo(InfoType::Volume, info_time, config.FontColor, Message.Volume, config.Volume);
			if(sv.game == SVData::AO) {
				AddInfo(InfoType::OriVolumePercent, info_time, config.FontColor, Message.OriVolumePercent, config.OriVolumePercent, "%");
				AddInfo(InfoType::OriginalVoice, info_time, config.FontColor, Message.OriginalVoice, Message.OriginalVoiceSwitch[config.OriginalVoice]);
			}
			AddInfo(InfoType::AutoPlay, info_time, config.FontColor, Message.AutoPlay, Message.AutoPlaySwitch[config.AutoPlay]);
			AddInfo(InfoType::SkipVoice, info_time, config.FontColor, Message.SkipVoice, Message.Switch[config.SkipVoice]);
			AddInfo(InfoType::DisableDialogSE, info_time, config.FontColor, Message.DisableDialogSE, Message.Switch[config.DisableDialogSE]);
			AddInfo(InfoType::DisableDududu, info_time, config.FontColor, Message.DisableDududu, Message.Switch[config.DisableDududu]);
			AddInfo(InfoType::InfoOnoff, info_time, config.FontColor, Message.ShowInfo, Message.ShowInfoSwitch[config.ShowInfo]);

			if (config.ShowInfo == Config::ShowInfo_WithMark && isAutoPlaying()) {
				AddInfo(InfoType::AutoPlayMark, INFINITY_TIME, config.FontColor, Message.AutoPlayMark);
			}
			else {
				Draw::RemoveInfo(InfoType::AutoPlayMark);
			}
		}
		else if (info_time != INFINITY_TIME) {
			Draw::RemoveInfo(InfoType::All);
		}
		else {
			Draw::RemoveInfo(InfoType::AutoPlayMark);
			Draw::RemoveInfo(InfoType::Hello);
		}
	} //keys[KEY_RESETA] && keys[KEY_RESETB]
	else {
		bool show_info = config.ShowInfo || info_time == INFINITY_TIME;
		if (keys[KEY_VOLUME_UP] && !last[KEY_VOLUME_UP] && !keys[KEY_VOLUME_DOWN]) {
			if (keys[KEY_VOLUME_BIGSTEP1] || keys[KEY_VOLUME_BIGSTEP2]) config.Volume += VOLUME_STEP_BIG;
			else config.Volume += VOLUME_STEP;

			if (config.Volume > Config::MAX_Volume) config.Volume = Config::MAX_Volume;
			sv.status.mute = 0;
			needsetvolume = volume_old != config.Volume;
			needsave = needsetvolume;

			if (show_info) {
				AddInfo(InfoType::Volume, info_time, config.FontColor, Message.Volume, config.Volume);
			}

			LOG("Set Volume : %d", config.Volume);
		} //if(KEY_VOLUME_UP)
		else if (keys[KEY_VOLUME_DOWN] && !last[KEY_VOLUME_DOWN] && !keys[KEY_VOLUME_UP]) {
			if (keys[KEY_VOLUME_BIGSTEP1] || keys[KEY_VOLUME_BIGSTEP2]) config.Volume -= VOLUME_STEP_BIG;
			else config.Volume -= VOLUME_STEP;

			if (config.Volume < 0) config.Volume = 0;
			sv.status.mute = 0;
			needsetvolume = volume_old != config.Volume;
			needsave = needsetvolume;

			if (show_info) {
				AddInfo(InfoType::Volume, info_time, config.FontColor, Message.Volume, config.Volume);
			}

			LOG("Set Volume : %d", config.Volume);
		}//if(KEY_VOLUME_DOWN)
		else if (keys[KEY_VOLUME_UP] && keys[KEY_VOLUME_DOWN] && !(last[KEY_VOLUME_UP] && last[KEY_VOLUME_DOWN])) {
			sv.status.mute = 1;
			needsetvolume = true;

			if (show_info) {
				AddInfo(InfoType::Volume, info_time, config.FontColor, Message.Mute);
			}

			LOG("Set mute : %d", sv.status.mute);
		}//keys[KEY_VOLUME_UP] && keys[KEY_VOLUME_DOWN]

		if(sv.game == SVData::AO) {
			if (keys[KEY_ORIVOLPCT_UP] && !last[KEY_ORIVOLPCT_UP] && !keys[KEY_ORIVOLPCT_DOWN]) {
				needsetvolume = config.OriVolumePercent != ORIVOLPCT_STEP;
				needsave = needsetvolume;

				config.OriVolumePercent += ORIVOLPCT_STEP;
				if (config.OriVolumePercent > config.MAX_OriVolumePercent) config.OriVolumePercent = config.MAX_OriVolumePercent;

				if (show_info) {
					AddInfo(InfoType::OriVolumePercent, info_time, config.FontColor, Message.OriVolumePercent, config.OriVolumePercent, "%");
				}

				LOG("Set OriVolumePercent : %d", config.OriVolumePercent);
			} //keys[KEY_ORIVOLPCT_UP]
			else if (keys[KEY_ORIVOLPCT_DOWN] && !last[KEY_ORIVOLPCT_DOWN] && !keys[KEY_ORIVOLPCT_UP]) {
				needsetvolume = config.OriVolumePercent != 0;
				needsave = needsetvolume;

				config.OriVolumePercent -= ORIVOLPCT_STEP;
				if (config.OriVolumePercent < 0) config.OriVolumePercent = 0;

				if (show_info) {
					AddInfo(InfoType::OriVolumePercent, info_time, config.FontColor, Message.OriVolumePercent, config.OriVolumePercent, "%");
				}

				LOG("Set OriVolumePercent : %d", config.OriVolumePercent);
			}

			if (keys[KEY_ORIVOICE] && !last[KEY_ORIVOICE]) {
				(config.OriginalVoice += 1) %= (Config::MAX_OriginalVoice + 1);
				needsave = true;

				if (show_info) {
					AddInfo(InfoType::OriginalVoice, info_time, config.FontColor, Message.OriginalVoice,
						Message.OriginalVoiceSwitch[config.OriginalVoice]);
				}

				LOG("Set OriginalVoice : %d", config.OriginalVoice);
			}//if(KEY_ORIVOICE)
		}

		if (keys[KEY_AUTOPLAY] && !last[KEY_AUTOPLAY]) {
			(config.AutoPlay += 1) %= (Config::MAX_AutoPlay + 1);
			needsave = true;

			if (show_info) {
				AddInfo(InfoType::AutoPlay, info_time, config.FontColor, Message.AutoPlay, Message.AutoPlaySwitch[config.AutoPlay]);
				if (config.ShowInfo == Config::ShowInfo_WithMark && isAutoPlaying()) {
					AddInfo(InfoType::AutoPlayMark, INFINITY_TIME, config.FontColor, Message.AutoPlayMark);
				}
				else {
					Draw::RemoveInfo(InfoType::AutoPlayMark);
				}
			}
			LOG("Set AutoPlay : %d", config.AutoPlay);

			if (config.AutoPlay && !config.SkipVoice) {
				config.SkipVoice = 1;

				if (show_info) {
					AddInfo(InfoType::SkipVoice, info_time, config.FontColor, Message.SkipVoice, Message.Switch[config.SkipVoice]);
				}
				LOG("Set SkipVoice : %d", config.SkipVoice);
			}
		}//if(KEY_AUTOPLAY)

		if (keys[KEY_SKIPVOICE] && !last[KEY_SKIPVOICE]) {
			config.SkipVoice = 1 - config.SkipVoice;
			needsave = true;

			if (show_info) {
				AddInfo(InfoType::SkipVoice, info_time, config.FontColor, Message.SkipVoice, Message.Switch[config.SkipVoice]);
			}

			LOG("Set SkipVoice : %d", config.SkipVoice);

			if (!config.SkipVoice && config.AutoPlay) {
				config.AutoPlay = 0;
				if (show_info) {
					AddInfo(InfoType::AutoPlay, info_time, config.FontColor, Message.AutoPlay, Message.AutoPlaySwitch[config.AutoPlay]);
				}
				Draw::RemoveInfo(InfoType::AutoPlayMark);
				LOG("Set AutoPlay : %d", config.AutoPlay);
			}
		}//if(KEY_SKIPVOICE)

		if (keys[KEY_DLGSE] && !last[KEY_DLGSE]) {
			config.DisableDialogSE = 1 - config.DisableDialogSE;
			if (sv.status.playing) {
				sv.order.disableDialogSE = config.DisableDialogSE;
			}
			needsave = true;

			if (show_info) {
				AddInfo(InfoType::DisableDialogSE, info_time, config.FontColor, Message.DisableDialogSE, Message.Switch[config.DisableDialogSE]);
			}

			LOG("Set DisableDialogSE : %d", config.DisableDialogSE);
		}//if(KEY_DLGSE)

		if (keys[KEY_DU] && !last[KEY_DU]) {
			config.DisableDududu = 1 - config.DisableDududu;
			if (sv.status.playing) {
				sv.order.disableDududu = config.DisableDududu;
			}
			needsave = true;

			if (show_info) {
				AddInfo(InfoType::DisableDududu, info_time, config.FontColor, Message.DisableDududu, Message.Switch[config.DisableDududu]);
			}

			LOG("Set DisableDududu : %d", config.DisableDududu);
		}//if(KEY_DU)

		if (keys[KEY_INFOONOFF] && !last[KEY_INFOONOFF]) {
			config.ShowInfo = (config.ShowInfo + 1) % (Config::MAX_ShowInfo + 1);
			needsave = true;

			if (config.ShowInfo) {
				if (config.ShowInfo == Config::ShowInfo_WithMark && isAutoPlaying()) {
					AddInfo(InfoType::AutoPlayMark, INFINITY_TIME, config.FontColor, Message.AutoPlayMark);
				}
				else {
					Draw::RemoveInfo(InfoType::AutoPlayMark);
				}
			}
			else if (info_time == INFINITY_TIME) {
				Draw::RemoveInfo(InfoType::AutoPlayMark);
				Draw::RemoveInfo(InfoType::Hello);
			}
			else { 
				Draw::RemoveInfo(InfoType::All);
			}
			AddInfo(InfoType::InfoOnoff, info_time, config.FontColor, Message.ShowInfo, Message.ShowInfoSwitch[config.ShowInfo]);

			LOG("Set ShowInfo : %d", config.ShowInfo);
		}//if(KEY_INFO)
	}

	if (needsetvolume) {
		if (sv.status.playing) {
			if (!sv.status.mute) {
				int volume = sv.status.playingOri ?
							config.Volume * config.OriVolumePercent / 100 :
							config.Volume;
				if (volume > config.MAX_Volume) volume = config.MAX_Volume;
				Player::SetVolume(volume);
			}
			else Player::SetVolume(0);
		}
	}

	if (needsave && config.SaveChange) {
		config.SaveConfig(CONFIG_FILE);
		LOG("Config file saved");
	}

	std::memcpy(last, keys, KEYS_NUM);
}

void SoraVoice::Show()
{
	if (!sv.status.startup) return;

	Clock::UpdateTime();

	if(!isZa) SoraVoice::Input();

	if (sv.status.showing) {
		Draw::DrawInfos();
		Draw::RemoveInfo(InfoType::Dead);
	}

	if (aup->count_ch == 1) {
		aup->count_ch++;
		if (config.ShowInfo == Config::ShowInfo_WithMark && isAutoPlaying()) {
			AddInfo(InfoType::AutoPlayMark, Draw::ShowTimeInfinity, config.FontColor ,Message.AutoPlayMark);
		}
	}

	if (!sv.status.playing) {
		if (aup->wait
			&& sv.status.scode != sv.scode.SAY && sv.status.scode != sv.scode.TALK && sv.status.scode != sv.scode.TEXT) {
			aup->count_ch = 0;
			aup->wait = 0;
			aup->waitv = 0;
			aup->time_autoplay = 0;

			if (config.ShowInfo == Config::ShowInfo_WithMark) {
				Draw::RemoveInfo(InfoType::AutoPlayMark);
			}

			sv.order.disableDududu = 0;
		}
		else if (aup->wait && !aup->time_autoplay) {
			aup->time_autoplay = aup->time_textbeg
				+ (aup->count_ch - 1) * config.WaitTimePerChar + config.WaitTimeDialog - TIME_PREC / 2;

			sv.order.disableDududu = 0;
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
				sv.order.autoPlay = 1;
				LOG("Auto play set.");

				SetThreadExecutionState(ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);

				if (config.ShowInfo == Config::ShowInfo_WithMark) {
					AddInfo(InfoType::AutoPlayMark, REMAIN_TIME, config.FontColor, Message.AutoPlayMark);
				}
			}

			aup->count_ch = 0;
			aup->wait = 0;
			aup->waitv = 0;
			aup->time_autoplay = 0;
		}
	}
}

bool SoraVoice::Init() {
	if (sv.status.startup || sv.status.ended) return false;

	config.LoadConfig(CONFIG_FILE, true);

	Clock::InitClock(sv.rcd.now, sv.rcd.recent);
	Draw::Init();
	Player::Init(*sv.addrs.p_pDS, stopCallBack);

	keys = new Keys(sv.addrs.p_keys, *sv.addrs.p_did);
	aup = new AutoPlay(sv.rcd.now, sv.rcd.count_ch, sv.status.wait,
						sv.rcd.time_textbeg, sv.status.waitv);

	LOG("Config loaded");
	LOG("config.Volume = %d", config.Volume);
	LOG("config.OriginalVoice = %d", config.OriginalVoice);
	LOG("config.AutoPlay = %d", config.AutoPlay);
	LOG("config.WaitTimePerChar = %d", config.WaitTimePerChar);
	LOG("config.WaitTimeDialog = %d", config.WaitTimeDialog);
	LOG("config.WaitTimeDialogVoice = %d", config.WaitTimeDialogVoice);
	LOG("config.SkipVoice = %d", config.SkipVoice);
	LOG("config.DisableDududu = %d", config.DisableDududu);
	LOG("config.DisableDialogSE = %d", config.DisableDialogSE);
	LOG("config.ShowInfo = %d", config.ShowInfo);
	LOG("config.FontName = %s", config.FontName);
	LOG("config.FontColor = 0x%08X", config.FontColor);

	LOG("config.EnableKeys = %d", config.EnableKeys);
	LOG("config.SaveChange = %d", config.SaveChange);

	static_assert(Config::MAX_Volume == Player::MaxVolume, "Max Volume not same!");

	if(GAME_IS_DX9(sv.game)) {
		AddInfo(InfoType::Hello, INFINITY_TIME, 0, Message.Mute);
		AddInfo(InfoType::Hello, INFINITY_TIME, 0, Message.Volume);
		AddInfo(InfoType::Hello, INFINITY_TIME, 0, Message.OriginalVoice);
		AddInfo(InfoType::Hello, INFINITY_TIME, 0, Message.AutoPlay);
		AddInfo(InfoType::Hello, INFINITY_TIME, 0, Message.SkipVoice);
		AddInfo(InfoType::Hello, INFINITY_TIME, 0, Message.DisableDialogSE);
		AddInfo(InfoType::Hello, INFINITY_TIME, 0, Message.DisableDududu);
		AddInfo(InfoType::Hello, INFINITY_TIME, 0, Message.ShowInfo);
		for(unsigned i = 0; i < std::extent_v<decltype(Message.Switch)>; i++)
			AddInfo(InfoType::Hello, INFINITY_TIME, 0, Message.Switch[i]);
		for (unsigned i = 0; i < std::extent_v<decltype(Message.AutoPlaySwitch)>; i++)
			AddInfo(InfoType::Hello, INFINITY_TIME, 0, Message.AutoPlaySwitch[i]);
		for (unsigned i = 0; i < std::extent_v<decltype(Message.OriginalVoiceSwitch)>; i++)
			AddInfo(InfoType::Hello, INFINITY_TIME, 0, Message.OriginalVoiceSwitch[i]);
		for (unsigned i = 0; i < std::extent_v<decltype(Message.ShowInfoSwitch)>; i++)
			AddInfo(InfoType::Hello, INFINITY_TIME, 0, Message.ShowInfoSwitch[i]);
		AddInfo(InfoType::Hello, INFINITY_TIME, 0, Message.AutoPlayMark);
		Draw::DrawInfos();
		Draw::RemoveInfo(InfoType::All);
	}

	void* pPresent = Hook::Hook_D3D_Present(*sv.addrs.p_d3dd, GAME_IS_DX9(sv.game), SoraVoice::Show);
	if (pPresent) {
		LOG("Present hooked, old Present = 0x%08X", pPresent);
	}
	else {
		LOG("Hook Present failed.");
	}

	if(GAME_IS_ZA(sv.game)) {
		if (config.EnableKeys) {
			LOG("Now going to hook GetDeviceState...");
			void* pGetDeviceState = Hook::Hook_DI_GetDeviceState(*sv.addrs.p_did, SoraVoice::Input, (void**)&sv.addrs.p_keys);
			if (pGetDeviceState) {
				LOG("GetDeviceState hooked, old GetDeviceState = 0x%08X", pGetDeviceState);
			}
			else {
				LOG("Hook GetDeviceState failed.");
			}
		}
	}

	if (config.ShowInfo) {
		AddInfo(InfoType::Hello, HELLO_TIME, config.FontColor, Message.Title);
		AddInfo(InfoType::Hello, HELLO_TIME, config.FontColor, Message.Version, DateVersion);
		AddInfo(InfoType::Hello, HELLO_TIME, config.FontColor, Message.CurrentTitle, sv.Comment);
	}

	playRandomVoice(sv.p_rnd_vlst);

	VOICEFILE_PREFIX = GAME_IS_ED6(sv.game) ? VOICEFILE_PREFIX_ED6 : VOICEFILE_PREFIX_ZA;
	sv.status.startup = true;
	return true;
}

bool SoraVoice::End() {
	if (!sv.status.startup) return false;

	Player::End();
	Draw::End();
	delete keys; keys = nullptr;
	delete aup; aup = nullptr;

	sv.status.startup = false;
	sv.status.ended = true;
	return true;
}

