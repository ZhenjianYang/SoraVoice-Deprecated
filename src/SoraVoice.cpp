#define NOMINMAX 1

#include "SoraVoice.h"
#include "Log.h"
#include "mapping.h"
#include "config.h"
#include "message.h"

#include <vorbis\vorbisfile.h>
#include <dsound.h>
#include <dinput.h>
#include <d3d8/d3dx8.h>

#include <thread>
#include <mutex>
#include <memory>
#include <list>
#include <set>

#define CONFIG_FILE "ed_voice.ini"
#define VOICEFILE_PREFIX "voice\\ch"
#define VOICEFILE_ATTR	".ogg"

#define MAX_OGG_FILENAME_LEN 25
#define NUM_AUDIO_BUF 2
#define NUM_NOTIFY_PER_BUFF 8
#define NUM_NOTIFY (NUM_AUDIO_BUF * NUM_NOTIFY_PER_BUFF)

static const GUID IID_IDirectSoundNotify = { 0xb0210783, 0x89cd, 0x11d0, 0xaf, 0x8, 0x0, 0xa0, 0xc9, 0x25, 0xcd, 0x16 };

static const int VOLUME_STEP = 1;
static const int VOLUME_STEP_BIG = 5;

static const int TIME_BUF = 1;

static const int KEY_MIN = DIK_5;
static const int KEY_MAX = DIK_EQUALS;
static const int KEYS_NUM = KEY_MAX - KEY_MIN + 1;

static const int KEY_VOLUME_UP = DIK_EQUALS;
static const int KEY_VOLUME_DOWN = DIK_MINUS;
static const int KEY_VOLUME_0 = DIK_0;
static const int KEY_VOLUME_BIGSTEP1 = DIK_LSHIFT;
static const int KEY_VOLUME_BIGSTEP2 = DIK_RSHIFT;

static const int KEY_AUTOPLAY = DIK_9;
static const int KEY_SKIPVOICE = DIK_8;
static const int KEY_DLGSE = DIK_7;
static const int KEY_DU = DIK_6;
static const int KEY_INFO = DIK_5;

static const int MAX_TEXT_BUF = 63;

static const int MIN_FONT_SIZE = 25;
static const int TEXT_NUM_SCRH = 25;

static const double BOUND_WIDTH_RATE = 0.5;
static const double LINE_SPACE_RATE = 0.15;
static const double TEXT_SHADOW_POS_RATE = 0.08;

static const int INFO_TIME = 2000;
static const int HELLO_TIME = 6000;
static const int INFINITY_TIME = 0;
static const int REMAIN_TIME = 3000;
static const unsigned SHADOW_COLOR = 0x40404040;

using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
using TimeUnit = std::chrono::milliseconds;

enum class InfoType
{
	Hello,
	InfoOnoff,
	AutoPlayMark,
	
	Volume,
	AutoPlay,
	SkipVoice,
	DisableDialogSE,
	DisableDududu,

	ConfigReset,

	All
};

struct Ogg {
	decltype(::ov_open_callbacks)* const ov_open_callbacks;
	decltype(::ov_info)* const ov_info;
	decltype(::ov_read)* const ov_read;
	decltype(::ov_clear)* const ov_clear;

	OggVorbis_File ovf;
	char oggFn[MAX_OGG_FILENAME_LEN + 1];

	Ogg(InitParam* ip)
		:ov_open_callbacks((decltype(ov_open_callbacks))*ip->p_ov_open_callbacks),
		ov_info((decltype(ov_info))*ip->p_ov_info),
		ov_read((decltype(ov_read))*ip->p_ov_read),
		ov_clear((decltype(ov_clear))*ip->p_ov_clear) {
		memset(&ovf, 0, sizeof(ovf));
		memcpy(oggFn, VOICEFILE_PREFIX, sizeof(VOICEFILE_PREFIX));
		memset(oggFn + sizeof(VOICEFILE_PREFIX), 0, sizeof(oggFn) - sizeof(VOICEFILE_PREFIX));
	}
};
struct DSD {
	const LPDIRECTSOUND pDS;
	LPDIRECTSOUNDBUFFER pDSBuff;
	WAVEFORMATEX waveFormatEx;
	DSBUFFERDESC dSBufferDesc;
	DSBPOSITIONNOTIFY dSNotifies[NUM_NOTIFY];

	int buffSize;
	int notifySize;
	int halfNotifySize;

	DSD(InitParam* ip)
		: pDS((decltype(pDS))*ip->p_pDS), pDSBuff(nullptr),
		buffSize(0), notifySize(0), halfNotifySize(0) {
		memset(&waveFormatEx, 0, sizeof(waveFormatEx));
		memset(&dSBufferDesc, 0, sizeof(dSBufferDesc));
		memset(dSNotifies, 0, sizeof(dSNotifies));
	}
};
struct Thread {
	HANDLE hEvents[NUM_NOTIFY];
	HANDLE hEventEnd;
	std::thread th_read;
	std::mutex mt_play;
	std::mutex mt_text;
	int playEnd;
	Thread(InitParam* ip) : playEnd(-1), hEventEnd(NULL) {
		memset(hEvents, 0, sizeof(hEvents));
	}
};
struct InputData {
	char* const keys;
	char* const last;
	InputData(InitParam* ip) : keys(ip->p_Keys), last(ip->keysOld) {
	}
};
struct Info {
	ID3DXFont *font;
	LOGFONT lf;
	int width;
	int height;
	int bound;
	int linespace;
	int shadow;
	unsigned fontColor;
	byte& showing;
	const HWND hwnd;

	struct InfoText
	{
		char text[MAX_TEXT_BUF + 1];
		RECT rect;
		unsigned color;
		unsigned format;
		InfoType type;

		TimePoint dead;
	};
	std::list<std::unique_ptr<InfoText>> texts;

	Info(InitParam* ip, const Config* config)
		:font(nullptr),  width(0), height(0), fontColor((unsigned)config->FontColor), showing(ip->status.showing), hwnd(*(HWND*)ip->pHwnd)
	{
		constexpr int len_cfn = std::extent<decltype(config->FontName)>::value;
		constexpr int len_lffn = std::extent<decltype(lf.lfFaceName)>::value;
		static_assert(len_cfn == len_lffn, "Font names' length not match");

		memset(&lf, 0, sizeof(lf));
		lf.lfHeight = -MIN_FONT_SIZE;
		lf.lfWeight = FW_NORMAL;
		lf.lfCharSet = DEFAULT_CHARSET;
		lf.lfOutPrecision = OUT_OUTLINE_PRECIS;
		lf.lfQuality = CLEARTYPE_QUALITY;

		memcpy(lf.lfFaceName, config->FontName, len_lffn);
		lf.lfFaceName[len_lffn - 1] = 1;
	}
	~Info() { if (font) font->Release(); }

	void addText(InfoType type, int time, const char* text, ...) {
		TimePoint now = Clock::now();
		TimePoint dead = time == 0 ? TimePoint::max() : now + TimeUnit(time);

		LOG("Add text, type = %d", type);
		const int h = lf.lfHeight < 0 ? -lf.lfHeight : lf.lfHeight;

		auto it = texts.end();

		if (type != InfoType::Hello) {
			for (it = texts.begin(); it != texts.end(); ++it) {
				if ((*it)->type == type) {
					LOG("No need to Create new Text.");
					break;
				}
			}
		}

		if (it == texts.end()) {
			LOG("Create new Text.");
			texts.push_back(std::unique_ptr<InfoText>(new InfoText));
			it = --texts.end();
		}
		(*it)->type = type;
		(*it)->color = fontColor;
		(*it)->dead = dead;
		(*it)->format = type == InfoType::AutoPlayMark ? DT_BOTTOM | DT_LEFT : DT_TOP | DT_LEFT;

		va_list argptr;
		va_start(argptr, text);
		vsnprintf((*it)->text, sizeof((*it)->text), text, argptr);
		va_end(argptr);
		int text_width = (int)(strlen((*it)->text) * h * 0.65);
		LOG("Text is %s", (*it)->text);
		LOG("Text width is %d", text_width);

		auto& rect = (*it)->rect;
		memset(&rect, 0, sizeof(rect));

		std::set<int> invalid_bottom, invalid_top;
		for (auto it2 = texts.begin(); it2 != texts.end(); ++it2) {
			if (it == it2) continue;
			invalid_top.insert((*it2)->rect.top);
			invalid_bottom.insert((*it2)->rect.bottom);
		}

		if ((*it)->format & DT_BOTTOM) {
			for (int bottom = height - bound; ; bottom -= h + bound) {
				if (invalid_bottom.find(bottom) == invalid_bottom.end()) {
					rect.bottom = bottom;
					break;
				}
			}
			rect.top = rect.bottom - h - linespace;
		}
		else {
			for (int top = bound; ; top += h + linespace) {
				if (invalid_top.find(top) == invalid_top.end()) {
					rect.top = top;
					break;
				}
			}

			rect.bottom = rect.top + h + linespace;
		}

		if ((*it)->format & DT_RIGHT) {
			rect.right = width - bound;
			rect.left = width - text_width;
		}
		else {
			rect.left = bound;
			rect.right = text_width;
		}

		LOG("top = %d, bottom = %d, left = %d, right = %d", rect.top, rect.bottom, rect.left, rect.right);

		showing = texts.size() > 0;
	}

	void removeText(InfoType type) {
		if (type == InfoType::All) {
			texts.clear();
		}
		else {
			texts.remove_if([&type](const std::unique_ptr<InfoText>& t) { return t->type == type; });
		}
		showing = texts.size() > 0;
	}

	void removeDead() {
		TimePoint now = Clock::now();
		for (auto it = texts.begin(); it != texts.end(); ++it) {
			if ((*it)->dead < now) {
				texts.erase(it);
			}
		}
		showing = texts.size() > 0;
	}

	void drawTexts(IDirect3DDevice8 *D3DD) {
		if (width == 0) return;

		D3DD->BeginScene();

		if (!font) {
			D3DXCreateFontIndirect(D3DD, &lf, &font);
		}

		if (font) {
			RECT rect_shadow;

			for (const auto& text : texts) {
				rect_shadow = text->rect;
				if (text->format & DT_RIGHT) rect_shadow.right += shadow;
				else rect_shadow.left += shadow;
				if (text->format & DT_BOTTOM) rect_shadow.bottom += shadow;
				else rect_shadow.top += shadow;

				unsigned color_shadow = (0xFFFFFF & SHADOW_COLOR) | (((text->color >> 24) * 3 / 4) << 24);
				font->DrawTextA(text->text, -1, &rect_shadow, text->format, color_shadow);
			}

			for (const auto& text : texts) {
				font->DrawTextA(text->text, -1, &text->rect, text->format, text->color);
			}
		}

		D3DD->EndScene();
		//if(font) font->Release();
	}
};

static inline int TO_DSVOLUME(int volume) {
	return (volume) == 0 ?
		DSBVOLUME_MIN :
		(int)(2000 * log10(double(volume) / Config::MAX_Volume));
}

SoraVoice::SoraVoice(InitParam* initParam)
	:ended(initParam->status.ended), status(&initParam->status), order(&initParam->order),
	config(new Config(CONFIG_FILE)), ogg(new Ogg(initParam)), dsd(new DSD(initParam)),
	th(new Thread(initParam)), ipt(new InputData(initParam)), inf(new Info(initParam, config))
{
	static_assert(NUM_NOTIFY <= MAXIMUM_WAIT_OBJECTS, "Notifies exceeds the maxmin number");
	static_assert(sizeof(initParam->keysOld) >= KEY_MAX - KEY_MIN + 1, "Size of keys old is not enough");
	static_assert(D3D_SDK_VERSION == 220, "SDKVersion must be 220");

	LOG("p = 0x%08X", initParam);
	LOG("p->pHwnd = 0x%08X", initParam->pHwnd);
	LOG("p->p_pDS = 0x%08X", initParam->p_pDS);
	LOG("p->p_Keys = 0x%08X", initParam->p_Keys);
	LOG("p->p_ov_open_callbacks = 0x%08X", initParam->p_ov_open_callbacks);
	LOG("p->p_ov_info = 0x%08X", initParam->p_ov_info);
	LOG("p->p_ov_read = 0x%08X", initParam->p_ov_read);
	LOG("p->p_ov_clear = 0x%08X", initParam->p_ov_clear);

	LOG("Hwnd = 0x%08X", *(unsigned*)initParam->pHwnd);
	LOG("pDS = 0x%08X", *initParam->p_pDS);
	LOG("ov_open_callbacks = 0x%08X", *initParam->p_ov_open_callbacks);
	LOG("ov_info = 0x%08X", *initParam->p_ov_info);
	LOG("ov_read = 0x%08X", *initParam->p_ov_read);
	LOG("ov_clear = 0x%08X", *initParam->p_ov_clear);

	init();
}

void SoraVoice::Play(const char* t)
{
	if (*t != '#') return;

	int idx = sizeof(VOICEFILE_PREFIX) - 1;
	t++;

	int len_vid = 0;
	long long vid = 0;
	for (int i = 0; i < MAX_VOICEID_LEN; i++) {
		if (*t < '0' || *t > '9') break;
		vid *= 10; vid += *t - '0';
		ogg->oggFn[idx + len_vid] = *t;

		len_vid++; t++;
	}
	ogg->oggFn[idx + len_vid] = 0;
	if (*t != 'V' || len_vid == 0) return;

	LOG("iptut Voice ID is \"%s\"", ogg->oggFn + idx);
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
			ogg->oggFn[idx + len_vid] = VoiceIdMapping[vid][len_vid];
		}
		ogg->oggFn[idx + len_vid] = 0;
	}
	LOG("True Voice ID is \"%s\"", ogg->oggFn + idx);
	idx += len_vid;
	for (int i = 0; i < sizeof(VOICEFILE_ATTR); i++) {
		ogg->oggFn[idx++] = VOICEFILE_ATTR[i];
	}

	LOG("Ogg filename: %s", ogg->oggFn);
	playSoundFile();
}

void SoraVoice::Stop()
{
	LOG("Force stopping is called.");

	th->mt_play.lock();
	stopPlaying();
	th->mt_play.unlock();
}

void SoraVoice::Input()
{
	if (!config->EnableKeys) return;

	const char* keys = ipt->keys;
	char* last = ipt->last;

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

			th->mt_text.lock();
			if (config->ShowInfo) {
				//inf->addText(InfoType::ConfigReset, INFO_TIME, Message::Reset);
				inf->addText(InfoType::Volume, INFO_TIME, Message::Volume, config->Volume);
				inf->addText(InfoType::AutoPlay, INFO_TIME, Message::AutoPlay, Message::Switch[config->AutoPlay]);
				inf->addText(InfoType::SkipVoice, INFO_TIME, Message::SkipVoice, Message::Switch[config->SkipVoice]);
				inf->addText(InfoType::DisableDialogSE, INFO_TIME, Message::DisableDialogSE, Message::Switch[config->DisableDialogSE]);
				inf->addText(InfoType::DisableDududu, INFO_TIME, Message::DisableDududu, Message::Switch[config->DisableDududu]);
				inf->addText(InfoType::InfoOnoff, INFO_TIME, Message::ShowInfo, Message::ShowInfoSwitch[config->ShowInfo]);

				if (config->AutoPlay && config->ShowInfo == 2 && status->playing) {
					inf->addText(InfoType::AutoPlayMark, INFINITY_TIME, Message::AutoPlayMark);
				}
			}
			else {
				inf->removeText(InfoType::All);
			}
			th->mt_text.unlock();
		}
	} //if(KEY_VOLUME_UP | KEY_VOLUME_DOWN | KEY_VOLUME_0)
	else {
		if (keys[KEY_VOLUME_UP] && !last[KEY_VOLUME_UP - KEY_MIN] && !keys[KEY_VOLUME_DOWN] && !keys[KEY_VOLUME_0]) {
			if (keys[KEY_VOLUME_BIGSTEP1] || keys[KEY_VOLUME_BIGSTEP2]) config->Volume += VOLUME_STEP_BIG;
			else config->Volume += VOLUME_STEP;

			if (config->Volume > Config::MAX_Volume) config->Volume = Config::MAX_Volume;
			status->mute = 0;
			if (dsd->pDSBuff) {
				dsd->pDSBuff->SetVolume(TO_DSVOLUME(config->Volume));
			}
			needsetvolume = volume_old != config->Volume;
			needsave = needsetvolume;

			th->mt_text.lock();
			if (config->ShowInfo) {
				inf->addText(InfoType::Volume, INFO_TIME, Message::Volume, config->Volume);
			}
			th->mt_text.unlock();

			LOG("Set Volume : %d", config->Volume);
		} //if(KEY_VOLUME_UP)
		else if (keys[KEY_VOLUME_DOWN] && !last[KEY_VOLUME_DOWN - KEY_MIN] && !keys[KEY_VOLUME_UP] && !keys[KEY_VOLUME_0]) {
			if (keys[KEY_VOLUME_BIGSTEP1] || keys[KEY_VOLUME_BIGSTEP2]) config->Volume -= VOLUME_STEP_BIG;
			else config->Volume -= VOLUME_STEP;

			if (config->Volume < 0) config->Volume = 0;
			needsetvolume = volume_old != config->Volume;
			needsave = needsetvolume;

			th->mt_text.lock();
			if (config->ShowInfo) {
				inf->addText(InfoType::Volume, INFO_TIME, Message::Volume, config->Volume);
			}
			th->mt_text.unlock();

			LOG("Set Volume : %d", config->Volume);
		}//if(KEY_VOLUME_DOWN)
		else if (keys[KEY_VOLUME_0] && !last[KEY_VOLUME_0 - KEY_MIN] && !keys[KEY_VOLUME_UP] && !keys[KEY_VOLUME_DOWN]) {
			status->mute = 1 - status->mute;
			needsetvolume = true;

			th->mt_text.lock();
			if (config->ShowInfo && status->mute) {
				inf->addText(InfoType::Volume, INFO_TIME, Message::Mute);
			}
			else {
				inf->addText(InfoType::Volume, INFO_TIME, Message::Volume, config->Volume);
			}
			th->mt_text.unlock();

			LOG("Set mute : %d", status->mute);
		}//if(KEY_VOLUME_0)

		if (keys[KEY_AUTOPLAY] && !last[KEY_AUTOPLAY - KEY_MIN]) {
			config->AutoPlay = 1 - config->AutoPlay;
			needsave = true;

			th->mt_text.lock();
			if (config->ShowInfo) {
				inf->addText(InfoType::AutoPlay, INFO_TIME, Message::AutoPlay, Message::Switch[config->AutoPlay]);
				if (config->AutoPlay && config->ShowInfo == 2 && status->playing) {
					inf->addText(InfoType::AutoPlayMark, INFINITY_TIME, Message::AutoPlayMark);
				}
				else {
					inf->removeText(InfoType::AutoPlayMark);
				}
			}
			th->mt_text.unlock();

			LOG("Set AutoPlay : %d", config->AutoPlay);
		}//if(KEY_AUTOPLAY)

		if (keys[KEY_SKIPVOICE] && !last[KEY_SKIPVOICE - KEY_MIN]) {
			config->SkipVoice = 1 - config->SkipVoice;
			needsave = true;
			if (status->playing) {
				order->skipVoice = config->SkipVoice;
			}

			th->mt_text.lock();
			if (config->ShowInfo) {
				inf->addText(InfoType::SkipVoice, INFO_TIME, Message::SkipVoice, Message::Switch[config->SkipVoice]);
			}
			th->mt_text.unlock();

			LOG("Set SkipVoice : %d", config->SkipVoice);
		}//if(KEY_SKIPVOICE)

		if (keys[KEY_DLGSE] && !last[KEY_DLGSE - KEY_MIN]) {
			config->DisableDialogSE = 1 - config->DisableDialogSE;
			if (status->playing) {
				order->disableDialogSE = config->DisableDialogSE;
			}
			needsave = true;

			th->mt_text.lock();
			if (config->ShowInfo) {
				inf->addText(InfoType::DisableDialogSE, INFO_TIME, Message::DisableDialogSE, Message::Switch[config->DisableDialogSE]);
			}
			th->mt_text.unlock();

			LOG("Set DisableDialogSE : %d", config->DisableDialogSE);
		}//if(KEY_DLGSE)

		if (keys[KEY_DU] && !last[KEY_DU - KEY_MIN]) {
			config->DisableDududu = 1 - config->DisableDududu;
			if (status->playing) {
				order->disableDududu = config->DisableDududu;
			}
			needsave = true;

			th->mt_text.lock();
			if (config->ShowInfo) {
				inf->addText(InfoType::DisableDududu, INFO_TIME, Message::DisableDududu, Message::Switch[config->DisableDududu]);
			}
			th->mt_text.unlock();

			LOG("Set DisableDududu : %d", config->DisableDududu);
		}//if(KEY_DU)

		if (keys[KEY_INFO] && !last[KEY_INFO - KEY_MIN]) {
			config->ShowInfo = (config->ShowInfo + 1) % (Config::MAX_ShowInfo + 1);
			needsave = true;

			th->mt_text.lock();
			if (config->ShowInfo) {
				if (config->ShowInfo == 2 && config->AutoPlay && status->playing) {
					inf->addText(InfoType::AutoPlayMark, INFINITY_TIME, Message::AutoPlayMark);
				}
			}
			else {
				inf->removeText(InfoType::All);
			}
			inf->addText(InfoType::InfoOnoff, INFO_TIME, Message::ShowInfo, Message::ShowInfoSwitch[config->ShowInfo]);
			th->mt_text.unlock();

			LOG("Set ShowInfo : %d", config->ShowInfo);
		}//if(KEY_DU)
	}

	if (needsetvolume) {
		th->mt_play.lock();
		if (status->playing) {
			if (status->mute) dsd->pDSBuff->SetVolume(TO_DSVOLUME(0));
			else dsd->pDSBuff->SetVolume(TO_DSVOLUME(config->Volume));
		}
		th->mt_play.unlock();
	}

	if (needsave && config->SaveChange) {
		config->SaveConfig(CONFIG_FILE);
		LOG("Config file saved");
	}

	memcpy(last, keys + KEY_MIN, KEYS_NUM);
}

void SoraVoice::Show(void * D3DD)
{
	if (!D3DD) return;
	IDirect3DDevice8* d3dd = (IDirect3DDevice8*)D3DD;

	inf->removeDead();
	if (!status->showing) return;
	inf->drawTexts(d3dd);
}

void SoraVoice::init()
{
	LOG("Config loaded");
	LOG("config.Volume = %d", config->Volume);
	LOG("config.AutoPlay = %d", config->AutoPlay);
	LOG("config.SkipVoice = %d", config->SkipVoice);
	LOG("config.DisableDududu = %d", config->DisableDududu);
	LOG("config.DisableDialogSE = %d", config->DisableDialogSE);
	LOG("config.ShowInfo = %d", config->ShowInfo);
	LOG("config.FontName = %s", config->FontName);
	LOG("config.FontColor = 0x%08X", config->FontColor);

	LOG("config.EnableKeys = %d", config->EnableKeys);
	LOG("config.SaveChange = %d", config->SaveChange);

	for (int i = 0; i < NUM_NOTIFY; i++) {
		th->hEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
		dsd->dSNotifies[i].hEventNotify = th->hEvents[i];
	}
	th->hEventEnd = CreateEvent(NULL, FALSE, FALSE, NULL);
	LOG("Event created!");

	ended = 0;

	th->th_read = std::thread(&SoraVoice::threadReadData, this);
	th->th_read.detach();
	LOG("Thread created!");


	RECT rect;
	if (GetClientRect(inf->hwnd, &rect)) {
		inf->width = rect.right - rect.left;
		inf->height = rect.bottom - rect.top;
		int fontSize = inf->height / TEXT_NUM_SCRH;
		if (fontSize < MIN_FONT_SIZE) fontSize = MIN_FONT_SIZE;
		inf->lf.lfHeight = -fontSize;
		inf->bound = (int)(fontSize * BOUND_WIDTH_RATE + 0.5);
		inf->shadow = (int)(fontSize * TEXT_SHADOW_POS_RATE + 0.5);
		inf->linespace = (int)(fontSize * LINE_SPACE_RATE + 0.5);

		LOG("screen width = %d", inf->width);
		LOG("screen height = %d", inf->height);
		LOG("Font Size = %d", fontSize);

		if (config->ShowInfo) {
			inf->addText(InfoType::Hello, 5000, Message::Title);
			inf->addText(InfoType::Hello, 5000, Message::Version, VERSION);
		}
	}
}

void SoraVoice::destory()
{
	Stop();

	memset(order, 0, sizeof(*order));
	ended = 1;

	SetEvent(th->hEvents[0]);
	WaitForSingleObject(th->hEventEnd, INFINITE);

	delete config;
	delete ogg;
	delete dsd;
	delete th;
	delete ipt;
	delete inf;
}

int SoraVoice::readSoundData(char * buff, int size)
{
	if (buff == nullptr || size <= 0) return 0;

	for (int i = 0; i < size; i++) buff[i] = 0;
	int total = 0;

	static const int block = 4096;
	int bitstream = 0;

	while (total < size)
	{
		int request = size - total < block ? size - total : block;
		int read = ogg->ov_read(&ogg->ovf, buff + total, request, 0, 2, 1, &bitstream);
		if (read <= 0) return total;

		total += read;
	}

	return total;
}

void SoraVoice::threadReadData()
{
	auto &playEnd = th->playEnd;
	while (!ended)
	{
		DWORD rst = WaitForMultipleObjects(NUM_NOTIFY, th->hEvents, FALSE, INFINITE);
		if (rst >= WAIT_OBJECT_0 && rst < WAIT_OBJECT_0 + NUM_NOTIFY) {
			th->mt_play.lock();

			if (status->playing) {
				const int id = rst - WAIT_OBJECT_0;
				if (id == playEnd) {
					LOG("Voice end, stop!");
					stopPlaying();
					if (config->AutoPlay) {
						order->autoPlay = 1;
						th->mt_text.lock();
						if (config->ShowInfo == 2) {
							inf->addText(InfoType::AutoPlayMark, REMAIN_TIME, Message::AutoPlayMark);
						}
						th->mt_text.unlock();
						SetThreadExecutionState(ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
					}
				}
				else {
					const int buff_no = id / NUM_NOTIFY_PER_BUFF;
					const int notify_no_inbuff = id % NUM_NOTIFY_PER_BUFF;

					if (notify_no_inbuff == 0) {
						const int read_buff_no = (buff_no - 1 + NUM_AUDIO_BUF) % NUM_AUDIO_BUF;

						const int start = read_buff_no * dsd->buffSize;
						const int size = read_buff_no == NUM_AUDIO_BUF - 1 ?
							dsd->dSBufferDesc.dwBufferBytes - (NUM_AUDIO_BUF - 1) * dsd->buffSize
							: dsd->buffSize;

						void *AP1, *AP2;
						DWORD AB1, AB2;
						int read = 0;

						if (DS_OK == dsd->pDSBuff->Lock(start, size, &AP1, &AB1, &AP2, &AB2, 0)) {
							read = readSoundData((char*)AP1, AB1);
							if (AP2) read += readSoundData((char*)AP2, AB2);
							dsd->pDSBuff->Unlock(AP1, AB1, AP2, AB2);
						}

						if (read < size && playEnd < 0) {
							playEnd = read_buff_no * NUM_NOTIFY_PER_BUFF + (read + dsd->notifySize - dsd->halfNotifySize) / dsd->notifySize;
							if (playEnd >= NUM_NOTIFY) playEnd = 0;
						}
					}
				}
			}

			th->mt_play.unlock();
		}//if
	}//while
	SetEvent(th->hEventEnd);
}

void SoraVoice::playSoundFile()
{
	LOG("Start playing: %s", ogg->oggFn);

	th->mt_play.lock();
	stopPlaying();
	th->mt_play.unlock();
	LOG("Previous playing stopped.");

	FILE* f = fopen(ogg->oggFn, "rb");
	if (f == NULL) {
		LOG("Open file failed!");
		return;
	}

	auto &ovf = ogg->ovf;
	if (ogg->ov_open_callbacks(f, &ovf, nullptr, 0, OV_CALLBACKS_DEFAULT) != 0) {
		fclose(f);
		LOG("Open file as ogg failed!");
		return;
	}
	LOG("Ogg file opened");

	vorbis_info* info = ogg->ov_info(&ovf, -1);

	auto &waveFormatEx = dsd->waveFormatEx;
	waveFormatEx.wFormatTag = WAVE_FORMAT_PCM;
	waveFormatEx.nChannels = info->channels;
	waveFormatEx.nSamplesPerSec = info->rate;
	waveFormatEx.wBitsPerSample = 16;
	waveFormatEx.nBlockAlign = info->channels * 16 / 8;
	waveFormatEx.nAvgBytesPerSec = waveFormatEx.nSamplesPerSec * waveFormatEx.nBlockAlign;
	waveFormatEx.cbSize = 0;

	auto &dSBufferDesc = dsd->dSBufferDesc;
	dSBufferDesc.dwSize = sizeof(dSBufferDesc);
	dSBufferDesc.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_CTRLVOLUME;
	dSBufferDesc.dwBufferBytes = waveFormatEx.nAvgBytesPerSec * TIME_BUF;
	dSBufferDesc.dwReserved = 0;
	dSBufferDesc.lpwfxFormat = &waveFormatEx;
	dSBufferDesc.guid3DAlgorithm = GUID_NULL;

	auto &buffSize = dsd->buffSize;
	auto &notifySize = dsd->notifySize;
	auto &halfNotifySize = dsd->halfNotifySize;
	buffSize = dSBufferDesc.dwBufferBytes / NUM_AUDIO_BUF;
	notifySize = buffSize / NUM_NOTIFY_PER_BUFF;
	halfNotifySize = notifySize / 2;

	auto &pDSBuff = dsd->pDSBuff;
	auto &pDS = dsd->pDS;
	if (DS_OK != pDS->CreateSoundBuffer(&dSBufferDesc, &pDSBuff, NULL)) {
		LOG("Create sound buff failed!");
		ogg->ov_clear(&ovf);
		return;
	}
	LOG("Sound buff opened");

	void *AP1, *AP2;
	DWORD AB1, AB2;
	if (DS_OK == pDSBuff->Lock(0, dsd->buffSize * (NUM_AUDIO_BUF - 1), &AP1, &AB1, &AP2, &AB2, 0)) {
		readSoundData((char*)AP1, AB1);
		if (AP2) readSoundData((char*)AP2, AB2);
		pDSBuff->Unlock(AP1, AB1, AP2, AB2);
	}
	else {
		pDSBuff->Release();
		ogg->ov_clear(&ovf);
		LOG("Write first data failed!");
		return;
	}
	LOG("First data wroten");

	auto &dSNotifies = dsd->dSNotifies;
	for (int i = 0; i < NUM_AUDIO_BUF; ++i) {
		for (int j = 0; j < NUM_NOTIFY_PER_BUFF; j++) {
			dSNotifies[i * NUM_NOTIFY_PER_BUFF + j].dwOffset = i * buffSize + j * notifySize + halfNotifySize;
		}
	}

	static LPDIRECTSOUNDNOTIFY pDSN = NULL;
	if (DS_OK != pDSBuff->QueryInterface(IID_IDirectSoundNotify, (LPVOID*)&pDSN)) {
		pDSBuff->Release();
		ogg->ov_clear(&ovf);
		LOG("Set notify failed!");
		return;
	};

	if (DS_OK != pDSN->SetNotificationPositions(NUM_NOTIFY, dSNotifies)) {
		pDSN->Release();
		pDSBuff->Release();
		ogg->ov_clear(&ovf);
		LOG("Set notify failed!");
		return;
	}
	pDSN->Release();
	LOG("Notify set");

	if (!status->mute) {
		pDSBuff->SetVolume(TO_DSVOLUME(config->Volume));
		LOG("DSVolume = %d", TO_DSVOLUME(config->Volume));
	}
	else {
		pDSBuff->SetVolume(TO_DSVOLUME(0));
		LOG("Mute");
	}

	th->mt_play.lock();
	status->playing = 1;
	if (config->DisableDududu) order->disableDududu = 1;
	if (config->DisableDialogSE) order->disableDialogSE = 1;
	if (config->SkipVoice) order->skipVoice = 1;
	th->mt_text.lock();
	if (config->AutoPlay && config->ShowInfo == 2) {
		inf->addText(InfoType::AutoPlayMark, INFINITY_TIME, Message::AutoPlayMark);
	}
	th->mt_text.unlock();
	pDSBuff->Play(0, 0, DSBPLAY_LOOPING);
	th->mt_play.unlock();

	LOG("Playing...");
}

void SoraVoice::stopPlaying()
{
	if (!status->playing) return;

	if (dsd->pDSBuff) {
		dsd->pDSBuff->Stop();
		dsd->pDSBuff->Release();
		dsd->pDSBuff = NULL;
	}
	status->playing = 0;
	th->playEnd = -1;
	order->disableDududu = 0;
	order->skipVoice = 0;

	ogg->ov_clear(&ogg->ovf);
}
