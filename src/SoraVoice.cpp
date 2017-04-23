#define NOMINMAX 1

#include "SoraVoice.h"
#include "InitParam.h"
#include "Log.h"
#include "config.h"
#include "message.h"
#include "Hook.h"

#ifdef ZA
#include <d3dx9.h>
#else
#include "mapping.h"
#include <d3d8/d3dx8.h>
#endif

#include <dinput.h>
#define DID IDirectInputDevice8A

#if DIRECT3D_VERSION == 0x0900
#define D3D IDirect3D9
#define D3DD IDirect3DDevice9
#define SPRITE NULL,
#elif DIRECT3D_VERSION == 0x0800
#define D3D IDirect3D8
#define D3DD IDirect3DDevice8
#define SPRITE
#else
static_assert(DIRECT3D_VERSION == 0x0900 || DIRECT3D_VERSION == 0x0800,
	"DIRECT3D_VERSION must be 0x0900 or 0x0800")
#endif

#include <vorbis\vorbisfile.h>
#include <dsound.h>

#include <thread>
#include <mutex>
#include <memory>
#include <list>
#include <set>
#include <map>

#ifndef MAX_VOICEID_LEN
#define MAX_VOICEID_LEN 10
#endif // !MAX_VOICEID_LEN

#ifndef MAX_VOICEID_LEN_NEED_MAPPING
#define MAX_VOICEID_LEN_NEED_MAPPING 5
#endif // !MAX_VOICEID_LEN_NEED_MAPPING


const GUID IID_IDirectSoundNotify = { 0xb0210783, 0x89cd, 0x11d0, 0xaf, 0x8, 0x0, 0xa0, 0xc9, 0x25, 0xcd, 0x16 };


#ifdef ZA
constexpr char CONFIG_FILE[] = "za_voice.ini";
constexpr char VOICEFILE_PREFIX[] = "v";
#else
constexpr char CONFIG_FILE[] = "ed_voice.ini";
constexpr char VOICEFILE_PREFIX[] = "ch";
#endif
constexpr char VOICEFILE_DIR[] = "voice\\";
constexpr char VOICEFILE_ATTR[] = ".ogg";

constexpr int MAX_OGG_FILENAME_LEN = 25;

constexpr int LEN_PREFIX = sizeof(VOICEFILE_DIR) - 1 + sizeof(VOICEFILE_PREFIX) - 1;
static_assert(LEN_PREFIX + MAX_VOICEID_LEN + sizeof(VOICEFILE_ATTR) - 1 <= MAX_OGG_FILENAME_LEN, "MAX_OGG_FILENAME_LEN not enought!");

constexpr int NUM_AUDIO_BUF = 2;
constexpr int NUM_NOTIFY_PER_BUFF = 8;
constexpr int NUM_NOTIFY = (NUM_AUDIO_BUF * NUM_NOTIFY_PER_BUFF);

constexpr int VOLUME_STEP = 1;
constexpr int VOLUME_STEP_BIG = 5;

constexpr int TIME_BUF = 1;

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

constexpr int MAX_TEXT_BUF = 63;

constexpr int MIN_FONT_SIZE = 25;
constexpr int TEXT_NUM_SCRH = 25;

constexpr double BOUND_WIDTH_RATE = 0.5;
constexpr double LINE_SPACE_RATE = 0.15;
constexpr double TEXT_SHADOW_POS_RATE = 0.08;

constexpr unsigned SHADOW_COLOR = 0x40404040;

constexpr unsigned INFO_TIME = 2000;
constexpr unsigned HELLO_TIME = 6000;
constexpr unsigned INFINITY_TIME = 0;
constexpr unsigned REMAIN_TIME = 1000;

constexpr unsigned TIME_PREC = 16;
constexpr unsigned TIME_PRECV = 62;

constexpr unsigned TIME_MAX = 0xFFFFFFFF;

#ifdef ZA
constexpr byte SCODE_TEXT = 0x55;
constexpr byte SCODE_SAY = 0x5C;
constexpr byte SCODE_TALK = 0x5D;
constexpr byte SCODE_MENU = 0x5E;
#else
constexpr byte SCODE_TEXT = 0x54;
constexpr byte SCODE_SAY = 0x5B;
constexpr byte SCODE_TALK = 0x5C;
constexpr byte SCODE_MENU = 0x5D;
#endif // ZA

using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
using TimeUnit = std::chrono::milliseconds;

class SoraVoiceImpl : private SoraVoice
{
	friend SoraVoice;

private:
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

	struct Info
	{
		char text[MAX_TEXT_BUF + 1];
		RECT rect;
		unsigned color;
		unsigned format;
		InfoType type;

		unsigned dead;
	};
	using PtrInfo = std::unique_ptr<Info>;
	using PtrInfoList = std::list<PtrInfo>;
	using MapVid = std::map<std::string, int>;

	const char* const Comment;
	byte& ended;

	InitParam::Status* const status;
	InitParam::Order* const order;

	Config _config;

	struct Time
	{
		const TimePoint base;
		unsigned &now;
		unsigned &recent;

		void UpdateTime() {
			TimePoint newNow = Clock::now();
			recent = now;
			now = (unsigned)std::chrono::duration_cast<TimeUnit>(newNow - base).count();
		}

		Time(InitParam *ip) : now(ip->rcd.now), recent(ip->rcd.recent), base(Clock::now()) {
			now = recent = 0;
		}
	} _tm;

	struct Ogg {
		decltype(::ov_open_callbacks)* const ov_open_callbacks;
		decltype(::ov_info)* const ov_info;
		decltype(::ov_read)* const ov_read;
		decltype(::ov_clear)* const ov_clear;

		OggVorbis_File ovf;
		const char* oggFn;
		char buff_oggFn[MAX_OGG_FILENAME_LEN + 1];

		Ogg(InitParam* ip)
			:ov_open_callbacks((decltype(ov_open_callbacks))*ip->addrs.p_ov_open_callbacks),
			ov_info((decltype(ov_info))*ip->addrs.p_ov_info),
			ov_read((decltype(ov_read))*ip->addrs.p_ov_read),
			ov_clear((decltype(ov_clear))*ip->addrs.p_ov_clear),
			oggFn(nullptr) {
			memset(&ovf, 0, sizeof(ovf));

			strcpy(buff_oggFn, VOICEFILE_DIR);
			strcpy(buff_oggFn + sizeof(VOICEFILE_DIR) - 1, VOICEFILE_PREFIX);
		}

		void setOggFileName(const char* str_vid = nullptr) {
			if (str_vid) {
				int idx = LEN_PREFIX;
				for (int i = 0; str_vid[i]; i++, idx++) {
					buff_oggFn[idx] = str_vid[i];
				}
				strcpy(buff_oggFn + idx, VOICEFILE_ATTR);
				oggFn = buff_oggFn;
			}
			else {
				oggFn = nullptr;
			}
		}

		int readOggData(char * buff, int size)
		{
			if (buff == nullptr || size <= 0) return 0;

			for (int i = 0; i < size; i++) buff[i] = 0;
			int total = 0;

			constexpr int block = 4096;
			int bitstream = 0;

			while (total < size)
			{
				int request = size - total < block ? size - total : block;
				int read = ov_read(&ovf, buff + total, request, 0, 2, 1, &bitstream);
				if (read <= 0) return total;

				total += read;
			}

			return total;
		}
	} _ogg;

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
			:pDS((decltype(pDS))*ip->addrs.p_pDS), pDSBuff(nullptr),
			buffSize(0), notifySize(0), halfNotifySize(0) {
			memset(&waveFormatEx, 0, sizeof(waveFormatEx));
			memset(&dSBufferDesc, 0, sizeof(dSBufferDesc));
			memset(dSNotifies, 0, sizeof(dSNotifies));
		}
	} _dsd;

	struct Thread {
		static constexpr int evEndid_Read = 0;
		static constexpr int evEndid_Open = 1;
		HANDLE hEvents[NUM_NOTIFY];
		HANDLE hEventOpenFile;
		HANDLE hEventEnd[2];
		std::thread th_open;
		std::thread th_read;
		std::mutex mt_play;
		int playEnd;
		enum OpenStatus
		{
			Openning,
			Failed,
			Succeeded
		} opening;
		Thread(InitParam* ip)
			:hEventOpenFile(NULL),
			playEnd(-1), opening(Succeeded) {
			memset(hEvents, 0, sizeof(hEvents));
			memset(hEventEnd, 0, sizeof(hEventEnd));
		}
	} _th;

	struct IPT {
		const char* const &keys;
		DID* const pDID;
		char last[KEYS_NUM];
		IPT(InitParam* ip)
			:keys(ip->addrs.p_keys), pDID((decltype(pDID))*ip->addrs.p_did) {
		}
	} _ipt;

	struct Draw {
		byte& showing;
		const unsigned& now;

		const HWND hwnd;
		D3DD* const pD3DD;
		ID3DXFont *font;

#if DIRECT3D_VERSION == 0x900
		D3DXFONT_DESCA desca;
#else
		LOGFONT lf;
		struct _DESCA {
			LONG &Height;
			LONG &Width;
			LONG &Weight;
			UINT MipLevels;
			BYTE &Italic;
			BYTE &CharSet;
			BYTE &OutputPrecision;
			BYTE &Quality;
			BYTE &PitchAndFamily;
			CHAR (&FaceName)[LF_FACESIZE];

			_DESCA(LOGFONT& lf) :
				Height(lf.lfHeight),
				Width(lf.lfWidth),
				Weight(lf.lfWeight),
				MipLevels(0),
				Italic(lf.lfItalic),
				CharSet(lf.lfCharSet),
				OutputPrecision(lf.lfOutPrecision),
				Quality(lf.lfQuality),
				PitchAndFamily(lf.lfPitchAndFamily),
				FaceName(lf.lfFaceName) { }
		} desca;
#endif // DIRECT3D_VERSION == 0x900

		int width;
		int height;
		int bound;
		int linespace;
		int shadow;
		unsigned fontColor;

		PtrInfoList infos;

		Draw(InitParam* ip)
			:showing(ip->status.showing), now(ip->rcd.now), 
			hwnd(*(HWND*)ip->addrs.p_Hwnd), pD3DD(*(decltype(pD3DD)*)ip->addrs.p_d3dd), font(nullptr),
#if DIRECT3D_VERSION == 0x900
			desca{ 0 },
#else
			lf{ 0 }, 
			desca(lf),
#endif
			width(0), height(0), bound(0), linespace(0), shadow(0), fontColor(0)
		{
		}
		~Draw() { if (font) font->Release(); }

		void drawInfos() {
			if (width == 0 || !pD3DD) return;

			pD3DD->BeginScene();
			//D3DXFONT_DESCA desca;

			if (!font) {
#if DIRECT3D_VERSION == 0x900
				D3DXCreateFontIndirect(pD3DD, &desca, &font);
#else
				D3DXCreateFontIndirect(pD3DD, &lf, &font);
#endif	
			}

			if (font) {
				RECT rect_shadow;

				for (const auto& info : infos) {
					rect_shadow = info->rect;
					if (info->format & DT_RIGHT) rect_shadow.right += shadow;
					else rect_shadow.left += shadow;
					if (info->format & DT_BOTTOM) rect_shadow.bottom += shadow;
					else rect_shadow.top += shadow;

					unsigned color_shadow = (0xFFFFFF & SHADOW_COLOR) | (((info->color >> 24) * 3 / 4) << 24);

					font->DrawTextA(SPRITE info->text, -1, &rect_shadow, info->format, color_shadow);
				}

				for (const auto& info : infos) {
					font->DrawTextA(SPRITE info->text, -1, &info->rect, info->format, fontColor);
				}
			}

			pD3DD->EndScene();
			//if (font) {
			//	font->Release(); font = NULL;
			//}
		}

		void addInfo(InfoType type, unsigned time, const char* text, ...) {
			unsigned dead = time == INFINITY_TIME ? TIME_MAX : now + time;

			LOG("Add text, type = %d", type);
			const int h = desca.Height < 0 ? -desca.Height : desca.Height;

			auto it = infos.end();

			if (type != InfoType::Hello) {
				for (it = infos.begin(); it != infos.end(); ++it) {
					if ((*it)->type == type) {
						LOG("No need to Create new Text.");
						break;
					}
				}
			}

			if (it == infos.end()) {
				LOG("Create new Text.");
				infos.push_back(PtrInfo(new Info));
				it = --infos.end();
			}
			(*it)->type = type;
			(*it)->color = fontColor;
			(*it)->dead = dead;
			(*it)->format = type == InfoType::AutoPlayMark ? DT_BOTTOM | DT_LEFT : DT_TOP | DT_LEFT;

			va_list argptr;
			va_start(argptr, text);
			vsnprintf((*it)->text, sizeof((*it)->text), text, argptr);
			va_end(argptr);
			int text_width = (int)(strlen((*it)->text) * h * 0.6);
			LOG("Text is %s", (*it)->text);
			LOG("Text width is %d", text_width);

			auto& rect = (*it)->rect;
			memset(&rect, 0, sizeof(rect));

			std::set<int> invalid_bottom, invalid_top;
			for (auto it2 = infos.begin(); it2 != infos.end(); ++it2) {
				if (it == it2 || (((*it)->format & DT_RIGHT) != ((*it2)->format & DT_RIGHT))) continue;
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
				rect.left = rect.right - text_width;
			}
			else {
				rect.left = bound;
				rect.right = rect.left + text_width;
			}

			LOG("top = %d, bottom = %d, left = %d, right = %d", rect.top, rect.bottom, rect.left, rect.right);

			showing = infos.size() > 0;
		}

		void removeInfo(InfoType type) {
			if (type == InfoType::All) {
				infos.clear();
			}
			else {
				infos.remove_if([&type](const PtrInfo& t) { return t->type == type; });
			}
			showing = infos.size() > 0;
		}

		void removeDeadInfos() {
			if (infos.empty()) {
				showing = 0;
				return;
			}

			infos.remove_if([this](const PtrInfo& t) { return t->dead < now; });

			showing = infos.size() > 0;
		}
	} _draw;

	struct AutoPlay {
		const unsigned &now;

		unsigned &count_ch;
		byte &wait;
		unsigned &time_textbeg;
		unsigned time_autoplay;
		
		byte &waitv;
		unsigned time_autoplayv;
		
		AutoPlay(InitParam* ip)
		:now(ip->rcd.now), count_ch(ip->rcd.count_ch), wait(ip->status.wait), time_textbeg(ip->rcd.time_textbeg),
		time_autoplay(0), waitv(ip->status.waitv), time_autoplayv(0){
		}
	} _aup;

	Config* const config = &_config;
	Ogg* const ogg = &_ogg;
	DSD* const dsd = &_dsd;
	Thread* const th = &_th;
	IPT* const ipt = &_ipt;
	Draw* const draw = &_draw;
	Time* const tm = &_tm;
	AutoPlay* const aup = &_aup;

private:
	void init();
	void destory();

	void threadOpenFile();
	void threadReadData();
	void playSoundFile();
	void stopPlaying();

	SoraVoiceImpl(InitParam* initParam);
	~SoraVoiceImpl() override { this->destory(); }

private:
	bool isAutoPlaying() {
		return aup->count_ch 
			&& (config->AutoPlay && (status->playing || aup->waitv)
				|| config->AutoPlay == Config::AutoPlay_ALL);
	}

public:
	void Play(const char* v) override;
	void Stop() override;
	void Input() override;
	void Show() override;
};

static inline int TO_DSVOLUME(int volume) {
	return (volume) == 0 ?
		DSBVOLUME_MIN :
		(int)(2000 * log10(double(volume) / Config::MAX_Volume));
}

SoraVoiceImpl::SoraVoiceImpl(InitParam* initParam)
	:Comment(initParam->Comment),
	ended(initParam->status.ended), status(&initParam->status), order(&initParam->order), 
	_config(CONFIG_FILE),
	_tm(initParam), _ogg(initParam), _dsd(initParam),
	_th(initParam), _ipt(initParam), _draw(initParam),
	_aup(initParam)
{
	static_assert(NUM_NOTIFY <= MAXIMUM_WAIT_OBJECTS, "Notifies exceeds the maxmin number");

	LOG("p = 0x%08X", initParam);
	LOG("p->p_ov_open_callbacks = 0x%08X", initParam->addrs.p_ov_open_callbacks);
	LOG("p->p_ov_info = 0x%08X", initParam->addrs.p_ov_info);
	LOG("p->p_ov_read = 0x%08X", initParam->addrs.p_ov_read);
	LOG("p->p_ov_clear = 0x%08X", initParam->addrs.p_ov_clear);
	LOG("p->p_d3dd = 0x%08X", initParam->addrs.p_d3dd);
	LOG("p->p_did = 0x%08X", initParam->addrs.p_did);
	LOG("p->p_Hwnd = 0x%08X", initParam->addrs.p_Hwnd);
	LOG("p->p_pDS = 0x%08X", initParam->addrs.p_pDS);
	LOG("p->p_mute = 0x%08X", initParam->addrs.p_mute);
	LOG("p->p_keys = 0x%08X", initParam->addrs.p_keys);

	LOG("ov_open_callbacks = 0x%08X", *initParam->addrs.p_ov_open_callbacks);
	LOG("ov_info = 0x%08X", *initParam->addrs.p_ov_info);
	LOG("ov_read = 0x%08X", *initParam->addrs.p_ov_read);
	LOG("ov_clear = 0x%08X", *initParam->addrs.p_ov_clear);
	LOG("d3dd = 0x%08X", *initParam->addrs.p_d3dd);
	LOG("did = 0x%08X", *initParam->addrs.p_did);
	LOG("Hwnd = 0x%08X", *initParam->addrs.p_Hwnd);
	LOG("pDS = 0x%08X", *initParam->addrs.p_pDS);

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
	init();
}

void SoraVoiceImpl::Play(const char* t)
{
	static bool ogg_fun_ok = ogg->ov_clear && ogg->ov_info && ogg->ov_open_callbacks && ogg->ov_read;
	if (*t != '#' || !dsd->pDS || !ogg_fun_ok) return;
	
	t++;

	std::string str_vid;
	int num_vid = 0;
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

	th->opening = th->Openning;
	ogg->setOggFileName(str_vid.c_str());
	LOG("Ogg filename: %s", ogg->oggFn ? ogg->oggFn : "nullptr");

	th->mt_play.lock();
	stopPlaying();
	th->mt_play.unlock();
	LOG("Previous playing stopped.");

	LOG("Now call opening thread...");
	SetEvent(th->hEventOpenFile);
	LOG("Called.");

	status->playing = 1;
	status->code5 = 0;
	aup->wait = 0;
	aup->waitv = 0;
	aup->count_ch = 0;
	aup->time_autoplay = 0;

	if (config->DisableDududu) order->disableDududu = 1;
	if (config->DisableDialogSE) order->disableDialogSE = 1;
	if (config->SkipVoice) order->skipVoice = 1;
}

void SoraVoiceImpl::Stop()
{
	LOG("Stop is called.");

	if (config->ShowInfo == Config::ShowInfo_WithMark && isAutoPlaying()) {
		draw->addInfo(InfoType::AutoPlayMark, REMAIN_TIME, Message::AutoPlayMark);
	}

	if (config->SkipVoice) {
		th->mt_play.lock();
		stopPlaying();
		th->mt_play.unlock();
	}
	
	status->code5 = 0;
	aup->wait = 0;
	aup->waitv = 0;
	aup->count_ch = 0;
	aup->time_autoplay = 0;
}

void SoraVoiceImpl::Input()
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

			if (config->ShowInfo) {
				//inf->addText(InfoType::ConfigReset, INFO_TIME, Message::Reset);
				draw->addInfo(InfoType::Volume, INFO_TIME, Message::Volume, config->Volume);
				draw->addInfo(InfoType::AutoPlay, INFO_TIME, Message::AutoPlay, Message::AutoPlaySwitch[config->AutoPlay]);
				draw->addInfo(InfoType::SkipVoice, INFO_TIME, Message::SkipVoice, Message::Switch[config->SkipVoice]);
				draw->addInfo(InfoType::DisableDialogSE, INFO_TIME, Message::DisableDialogSE, Message::Switch[config->DisableDialogSE]);
				draw->addInfo(InfoType::DisableDududu, INFO_TIME, Message::DisableDududu, Message::Switch[config->DisableDududu]);
				draw->addInfo(InfoType::InfoOnoff, INFO_TIME, Message::ShowInfo, Message::ShowInfoSwitch[config->ShowInfo]);

				if (config->ShowInfo == Config::ShowInfo_WithMark && isAutoPlaying()) {
					draw->addInfo(InfoType::AutoPlayMark, INFINITY_TIME, Message::AutoPlayMark);
				}
				else {
					draw->removeInfo(InfoType::AutoPlayMark);
				}
			}
			else {
				draw->removeInfo(InfoType::All);
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
				draw->addInfo(InfoType::Volume, INFO_TIME, Message::Volume, config->Volume);
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
				draw->addInfo(InfoType::Volume, INFO_TIME, Message::Volume, config->Volume);
			}

			LOG("Set Volume : %d", config->Volume);
		}//if(KEY_VOLUME_DOWN)
		else if (keys[KEY_VOLUME_0] && !last[KEY_VOLUME_0 - KEY_MIN] && !keys[KEY_VOLUME_UP] && !keys[KEY_VOLUME_DOWN]) {
			status->mute = 1 - status->mute;
			needsetvolume = true;

			if (config->ShowInfo && status->mute) {
				draw->addInfo(InfoType::Volume, INFO_TIME, Message::Mute);
			}
			else {
				draw->addInfo(InfoType::Volume, INFO_TIME, Message::Volume, config->Volume);
			}

			LOG("Set mute : %d", status->mute);
		}//if(KEY_VOLUME_0)

		if (keys[KEY_AUTOPLAY] && !last[KEY_AUTOPLAY - KEY_MIN]) {
			(config->AutoPlay += 1) %= (Config::MAX_AutoPlay + 1);
			needsave = true;

			if (config->ShowInfo) {
				draw->addInfo(InfoType::AutoPlay, INFO_TIME, Message::AutoPlay, Message::AutoPlaySwitch[config->AutoPlay]);
				if (config->ShowInfo == Config::ShowInfo_WithMark && isAutoPlaying()) {
					draw->addInfo(InfoType::AutoPlayMark, INFINITY_TIME, Message::AutoPlayMark);
				}
				else {
					draw->removeInfo(InfoType::AutoPlayMark);
				}
			}

			if (config->AutoPlay && !config->SkipVoice) {
				config->SkipVoice = 1;
				if (config->ShowInfo) {
					draw->addInfo(InfoType::SkipVoice, INFO_TIME, Message::SkipVoice, Message::Switch[config->SkipVoice]);
				}
			}

			LOG("Set AutoPlay : %d", config->AutoPlay);
		}//if(KEY_AUTOPLAY)

		if (keys[KEY_SKIPVOICE] && !last[KEY_SKIPVOICE - KEY_MIN]) {
			config->SkipVoice = 1 - config->SkipVoice;
			needsave = true;
			if (status->playing) {
				order->skipVoice = config->SkipVoice;
			}
;
			if (config->ShowInfo) {
				draw->addInfo(InfoType::SkipVoice, INFO_TIME, Message::SkipVoice, Message::Switch[config->SkipVoice]);
			}

			if (!config->SkipVoice && config->AutoPlay) {
				config->AutoPlay = 0;
				if (config->ShowInfo) {
					draw->addInfo(InfoType::AutoPlay, INFO_TIME, Message::AutoPlay, Message::AutoPlaySwitch[config->AutoPlay]);
				}
			}

			LOG("Set SkipVoice : %d", config->SkipVoice);
		}//if(KEY_SKIPVOICE)

		if (keys[KEY_DLGSE] && !last[KEY_DLGSE - KEY_MIN]) {
			config->DisableDialogSE = 1 - config->DisableDialogSE;
			if (status->playing) {
				order->disableDialogSE = config->DisableDialogSE;
			}
			needsave = true;

			if (config->ShowInfo) {
				draw->addInfo(InfoType::DisableDialogSE, INFO_TIME, Message::DisableDialogSE, Message::Switch[config->DisableDialogSE]);
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
				draw->addInfo(InfoType::DisableDududu, INFO_TIME, Message::DisableDududu, Message::Switch[config->DisableDududu]);
			}

			LOG("Set DisableDududu : %d", config->DisableDududu);
		}//if(KEY_DU)

		if (keys[KEY_INFO] && !last[KEY_INFO - KEY_MIN]) {
			config->ShowInfo = (config->ShowInfo + 1) % (Config::MAX_ShowInfo + 1);
			needsave = true;

			if (config->ShowInfo) {
				if (config->ShowInfo == 2 && config->AutoPlay && status->playing) {
					draw->addInfo(InfoType::AutoPlayMark, INFINITY_TIME, Message::AutoPlayMark);
				}
			}
			else {
				draw->removeInfo(InfoType::All);
			}
			draw->addInfo(InfoType::InfoOnoff, INFO_TIME, Message::ShowInfo, Message::ShowInfoSwitch[config->ShowInfo]);

			LOG("Set ShowInfo : %d", config->ShowInfo);
		}//if(KEY_INFO)
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

void SoraVoiceImpl::Show()
{
	tm->UpdateTime();

	if (status->showing) {
		draw->drawInfos();
		draw->removeDeadInfos();
	}

	if (ogg->oggFn && th->opening != th->Openning) {
		ogg->setOggFileName();
		if (th->opening == th->Succeeded) {
			LOG("Ogg file opened, now going to play...");
			playSoundFile();
		}
		else {
			order->disableDududu = 0;
			order->disableDialogSE = 0;
			order->skipVoice = 0;
			status->playing = 0;
			LOG("Ogg file open failed, no need to play.");
		}
	}

	if (aup->count_ch == 1) {
		aup->count_ch++;
		if (config->ShowInfo == Config::ShowInfo_WithMark && isAutoPlaying()) {
			draw->addInfo(InfoType::AutoPlayMark, INFINITY_TIME, Message::AutoPlayMark);
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
				draw->removeInfo(InfoType::AutoPlayMark);
			}
		}
		else if (aup->wait && !aup->time_autoplay) {
			aup->time_autoplay = aup->time_textbeg
				+ (aup->count_ch - 1) * config->WaitTimePerChar + config->WaitTimeDialog - TIME_PREC / 2;
		}

		if (aup->waitv && aup->time_autoplayv <= aup->now
			|| !aup->waitv && aup->wait && aup->time_autoplay <= aup->now) {
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
					draw->addInfo(InfoType::AutoPlayMark, REMAIN_TIME, Message::AutoPlayMark);
				}
			}

			aup->count_ch = 0;
			aup->wait = 0;
			aup->waitv = 0;
			aup->time_autoplay = 0;
		}
	}
}

void SoraVoiceImpl::init()
{
	constexpr int len_cfn = sizeof(config->FontName);
	constexpr int len_lffn = sizeof(draw->desca.FaceName);
	static_assert(len_cfn == len_lffn, "Font names' length not match");

	for (int i = 0; i < NUM_NOTIFY; i++) {
		th->hEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
		dsd->dSNotifies[i].hEventNotify = th->hEvents[i];
	}
	th->hEventOpenFile = CreateEvent(NULL, FALSE, FALSE, NULL);
	th->hEventEnd[th->evEndid_Read] = CreateEvent(NULL, TRUE, FALSE, NULL);
	th->hEventEnd[th->evEndid_Open] = CreateEvent(NULL, TRUE, FALSE, NULL);
	LOG("Event created!");

	ended = 0;

	th->th_read = std::thread(&SoraVoiceImpl::threadReadData, this);
	th->th_read.detach();

	th->th_open = std::thread(&SoraVoiceImpl::threadOpenFile, this);
	th->th_open.detach();

	LOG("Thread created!");

	void* pPresent = Hook_D3D_Present(draw->pD3DD);
	if (pPresent) {
		LOG("Present hooked, old Present = 0x%08X", pPresent);
	}
	else {
		LOG("Hook Present failed.");
	}

	void* pGetDeviceState = Hook_DI_GetDeviceState(ipt->pDID);
	if (pGetDeviceState) {
		LOG("GetDeviceState hooked, old GetDeviceState = 0x%08X", pGetDeviceState);
	}
	else {
		LOG("Hook GetDeviceState failed.");
	}

	RECT rect;
	if (GetClientRect(draw->hwnd, &rect)) {
		draw->width = rect.right - rect.left;
		draw->height = rect.bottom - rect.top;

		int fontSize = draw->height / TEXT_NUM_SCRH;
		if (fontSize < MIN_FONT_SIZE) fontSize = MIN_FONT_SIZE;
		draw->desca.Height = -fontSize;
		draw->desca.Weight = FW_NORMAL;
		draw->desca.CharSet = DEFAULT_CHARSET;
		draw->desca.OutputPrecision = OUT_OUTLINE_PRECIS;
		draw->desca.Quality = CLEARTYPE_QUALITY;

		memcpy(draw->desca.FaceName, config->FontName, len_lffn);
		draw->desca.FaceName[len_lffn - 1] = 0;

		draw->fontColor = config->FontColor;
		draw->bound = (int)(fontSize * BOUND_WIDTH_RATE + 0.5);
		draw->shadow = (int)(fontSize * TEXT_SHADOW_POS_RATE + 0.5);
		draw->linespace = (int)(fontSize * LINE_SPACE_RATE + 0.5);

		LOG("screen width = %d", draw->width);
		LOG("screen height = %d", draw->height);
		LOG("Font Size = %d", fontSize);
		LOG("Font Name = %s", draw->desca.FaceName);

		if (config->ShowInfo) {
			draw->addInfo(InfoType::Hello, HELLO_TIME, Message::Title);
			draw->addInfo(InfoType::Hello, HELLO_TIME, Message::Version, Message::VersionNum);
			draw->addInfo(InfoType::Hello, HELLO_TIME, Message::GameTitle, Comment);
		}
	}
}

void SoraVoiceImpl::destory()
{
	th->mt_play.lock();
	stopPlaying();
	th->mt_play.unlock();

	memset(order, 0, sizeof(*order));
	ended = 1;

	SetEvent(th->hEvents[0]);
	SetEvent(th->hEventOpenFile);
	WaitForMultipleObjects(sizeof(th->hEventEnd) / sizeof(*th->hEventEnd), th->hEventEnd, TRUE, 100);
}

void SoraVoiceImpl::threadOpenFile()
{
	while (!ended)
	{
		DWORD rst = WaitForSingleObject(th->hEventOpenFile, INFINITE);
		if (rst == WAIT_OBJECT_0 && !ended) {
			FILE* f = NULL;
			LOG("[Thread_Open]Open %s ...", ogg->oggFn);
			f = fopen(ogg->oggFn, "rb");

			if (f == NULL) {
				th->opening = th->Failed;
				LOG("Open file failed.");
				break;
			}

			auto &ovf = ogg->ovf;
			if (ogg->ov_open_callbacks(f, &ovf, nullptr, 0, OV_CALLBACKS_DEFAULT) != 0) {
				fclose(f);
				th->opening = th->Failed;
				LOG("[Thread_Open]Open file as ogg failed!");
				break;
			}
			LOG("[Thread_Open]Ogg file opened");
			th->opening = th->Succeeded;
		}
	}//while
	SetEvent(th->hEventEnd[th->evEndid_Open]);
}

void SoraVoiceImpl::threadReadData()
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
					LOG("[Thread_Read]Voice end, stop!");
					
					aup->waitv = 1;
					aup->time_autoplayv = aup->now + config->WaitTimeDialogVoice - TIME_PRECV / 2;

					LOG("[Thread_Read]now = %d", tm->now);
					LOG("[Thread_Read]time_autoplayv = %d", aup->time_autoplayv);

					stopPlaying();
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
							read = ogg->readOggData((char*)AP1, AB1);
							if (AP2) read += ogg->readOggData((char*)AP2, AB2);
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
	SetEvent(th->hEventEnd[th->evEndid_Read]);
}

void SoraVoiceImpl::playSoundFile()
{
	vorbis_info* info = ogg->ov_info(&ogg->ovf, -1);

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
		ogg->ov_clear(&ogg->ovf);
		return;
	}
	LOG("Sound buff opened");

	void *AP1, *AP2;
	DWORD AB1, AB2;
	if (DS_OK == pDSBuff->Lock(0, dsd->buffSize * (NUM_AUDIO_BUF - 1), &AP1, &AB1, &AP2, &AB2, 0)) {
		ogg->readOggData((char*)AP1, AB1);
		if (AP2) ogg->readOggData((char*)AP2, AB2);
		pDSBuff->Unlock(AP1, AB1, AP2, AB2);
	}
	else {
		pDSBuff->Release();
		ogg->ov_clear(&ogg->ovf);
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
		ogg->ov_clear(&ogg->ovf);
		LOG("Set notify failed!");
		return;
	};

	if (DS_OK != pDSN->SetNotificationPositions(NUM_NOTIFY, dSNotifies)) {
		pDSN->Release();
		pDSBuff->Release();
		ogg->ov_clear(&ogg->ovf);
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
	pDSBuff->Play(0, 0, DSBPLAY_LOOPING);
	th->mt_play.unlock();

	LOG("Playing...");
}

void SoraVoiceImpl::stopPlaying()
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

SoraVoice * SoraVoice::CreateInstance(void * initParam)
{
	return new SoraVoiceImpl((InitParam*)initParam);
}

void SoraVoice::DestoryInstance(SoraVoice * sv)
{
	delete sv;
}

