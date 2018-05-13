#include "Draw.h"

#include "Draw_D3D.h"

#include <SVData.h>
#include <Config.h>

#include <Utils/Log.h>
#include <Utils/Clock.h>
#include <Utils/Encoding.h>

#include <Windows.h>

#include <list>
#include <set>
#include <memory>

constexpr int MAX_TEXT_LEN = 63;

constexpr int MIN_FONT_SIZE = 20;
constexpr int TEXT_NUM_SCRH = 25;

constexpr double VBOUND_RATE = 0.3;
constexpr double HBOUND_RATE = 0.5;
constexpr double LINE_SPACE_RATE = 0.15;
constexpr double TEXT_SHADOW_POS_RATE = 0.08;

constexpr unsigned SHADOW_COLOR = 0x20202020;

constexpr unsigned TIME_MAX = 0xFFFFFFFF;

static constexpr const unsigned DftFormatList_Sora[] = {
	DT_TOP | DT_LEFT   ,//Hello = 0,
	DT_TOP | DT_LEFT   ,//InfoOnoff,
	DT_BOTTOM | DT_LEFT,//AutoPlayMark,
	DT_TOP | DT_LEFT   ,//Volume,
	DT_TOP | DT_LEFT   ,//OriginalVoice,
	DT_TOP | DT_LEFT   ,//OriVolumePercent,
	DT_TOP | DT_LEFT   ,//AutoPlay,
	DT_TOP | DT_LEFT   ,//SkipVoice,
	DT_TOP | DT_LEFT   ,//DisableDialogSE,
	DT_TOP | DT_LEFT   ,//DisableDududu,

	DT_TOP | DT_LEFT   ,//ConfigReset,

	DT_TOP | DT_LEFT   ,//All,
};

static constexpr const unsigned DftFormatList_ZA[] = {
	DT_TOP | DT_LEFT   ,//Hello = 0,
	DT_TOP | DT_LEFT   ,//InfoOnoff,
	DT_BOTTOM | DT_RIGHT,//AutoPlayMark,
	DT_TOP | DT_LEFT   ,//Volume,
	DT_TOP | DT_LEFT   ,//OriginalVoice,
	DT_TOP | DT_LEFT   ,//OriVolumePercent,
	DT_TOP | DT_LEFT   ,//AutoPlay,
	DT_TOP | DT_LEFT   ,//SkipVoice,
	DT_TOP | DT_LEFT   ,//DisableDialogSE,
	DT_TOP | DT_LEFT   ,//DisableDududu,

	DT_TOP | DT_LEFT   ,//ConfigReset,

	DT_TOP | DT_LEFT   ,//All,
};

using namespace Draw;

struct Info
{
	WString text;
	RECT rect;
	unsigned color;
	unsigned format;
	Draw::InfoType type;

	unsigned deadTime;
};
using InfoList = std::list<Info>;

static unsigned* DR_showing;
static const unsigned* DR_dftFormatList = DftFormatList_ZA;

static D3D* DR_d3d = nullptr;
static InfoList DR_infoList;

static int DR_width = 0;
static int DR_height = 0;
static int DR_vfix = 0;
static int DR_vbound = 0;
static int DR_hbound = 0;
static int DR_linespace = 0;
static int DR_shadow = 0;

static int DR_fontSize = 0;

bool Draw::Init() {
	const bool isED6 = SERIES_IS_ED6(SV.series);

	DR_showing = &SV.status.showing;
	DR_dftFormatList = isED6 ? DftFormatList_Sora : DftFormatList_ZA;

	RECT rect;
	if (GetClientRect((HWND)*SV.addrs.p_Hwnd, &rect)) {
		DR_width = rect.right - rect.left;
		DR_height = rect.bottom - rect.top;

		if (!isED6)
			DR_vfix = (DR_height - DR_width * 9 / 16) / 2;

		DR_fontSize = (DR_height - 2 * DR_vfix) / TEXT_NUM_SCRH;
		if (DR_fontSize < MIN_FONT_SIZE) DR_fontSize = MIN_FONT_SIZE;

		DR_vbound = (int)(DR_fontSize * VBOUND_RATE + 0.5);
		DR_hbound = (int)(DR_fontSize * HBOUND_RATE + 0.5);

		DR_shadow = (int)(DR_fontSize * TEXT_SHADOW_POS_RATE + 0.5);
		DR_linespace = (int)(DR_fontSize * LINE_SPACE_RATE + 0.5);

		LOG("screen width = %d", DR_width);
		LOG("screen height = %d", DR_height);
		LOG("Font Size = %d", DR_fontSize);
	}
	else {
		LOG("Get screen size failed!");
		return false;
	}

	DR_d3d = D3D::GetD3D(SV.dxver == DX9, Config.FontName, DR_fontSize);
	return DR_d3d;
}

bool Draw::End() {
	delete DR_d3d;
	DR_d3d = nullptr;

	return false;
}

unsigned Draw::AddInfo(InfoType type, unsigned time, unsigned color, const char* text) {
	unsigned dead = time == ShowTimeInfinity ? TIME_MAX : Clock::Now() + time;
	constexpr auto NumValidType = InfoType::All;
	const unsigned format = type < NumValidType ? DR_dftFormatList[(int)type] : DR_dftFormatList[(int)InfoType::All];

	LOG("Add text, type = %d", (int)type);
	const int h = int(DR_fontSize * 1.2);

	auto it = DR_infoList.end();

	bool creatNew = true;
	if (type != InfoType::Hello) {
		for (it = DR_infoList.begin(); it != DR_infoList.end(); ++it) {
			if (it->type == type) {
				creatNew = false;
				break;
			}
		}
	}

	if (creatNew) {
		LOG("Create new Text.");
		DR_infoList.push_back({});
		it = --DR_infoList.end();
	}
	else {
		LOG("No need to create new Text.");
	}
	it->type = type;
	it->color = color;
	it->deadTime = dead;
	it->format = format;

	it->text = Encoding::Utf8ToUtf16(text);
	int cnt_ascii = 0;
	int cnt_nonasc = 0;
	for (auto p = text; *p; ++p) {
		if (*p >= 0 && *p < 0x80) cnt_ascii++;
		else cnt_nonasc++;
	}

	int text_width = (int)((3 + cnt_ascii * 0.52 + cnt_nonasc * 1.05) * h);
	LOG("Text is %s", text);
	LOG("Text width is %d", text_width);

	auto& rect = it->rect;

	if (creatNew) {
		memset(&rect, 0, sizeof(rect));
		std::set<int> invalid_bottom, invalid_top;
		for (auto it2 = DR_infoList.begin(); it2 != DR_infoList.end(); ++it2) {
			if (it == it2 || ((it->format & DT_RIGHT) != (it2->format & DT_RIGHT))) continue;
			invalid_top.insert(it2->rect.top);
			invalid_bottom.insert(it2->rect.bottom);
		}

		if (it->format & DT_BOTTOM) {
			for (int bottom = DR_height - DR_vbound - DR_vfix; ; bottom -= h + DR_linespace) {
				if (invalid_bottom.find(bottom) == invalid_bottom.end()) {
					rect.bottom = bottom;
					break;
				}
			}
			rect.top = rect.bottom - h - DR_linespace;
		}
		else {
			for (int top = DR_vbound + DR_vfix; ; top += h + DR_linespace) {
				if (invalid_top.find(top) == invalid_top.end()) {
					rect.top = top;
					break;
				}
			}

			rect.bottom = rect.top + h + DR_linespace;
		}
	} //if (creatNew)

	if (it->format & DT_RIGHT) {
		rect.right = DR_width - DR_hbound;
		rect.left = rect.right - text_width;
	}
	else {
		rect.left = DR_hbound;
		rect.right = rect.left + text_width;
	}

	LOG("top = %ld, bottom = %ld, left = %ld, right = %ld", rect.top, rect.bottom, rect.left, rect.right);

	return *DR_showing = DR_infoList.size();
}

void Draw::DrawInfos(void* pD3DD) {
	if (!DR_d3d) return;

	if (DR_d3d->BeginDraw(pD3DD)) {

		RECT rect_shadow;
		for (const auto& info : DR_infoList) {
			rect_shadow = info.rect;
			if (info.format & DT_RIGHT) rect_shadow.right += DR_shadow;
			else rect_shadow.left += DR_shadow;
			if (info.format & DT_BOTTOM) rect_shadow.bottom += DR_shadow;
			else rect_shadow.top += DR_shadow;

			unsigned color_shadow = (0xFFFFFF & SHADOW_COLOR) | (0xFF000000 & info.color);

			DR_d3d->DrawString(info.text, -1, &rect_shadow, info.format, color_shadow);
		}

		for (const auto& info : DR_infoList) {
			DR_d3d->DrawString(info.text, -1, &info.rect, info.format, info.color);
		}

		DR_d3d->EndDraw();
	}
}

unsigned Draw::RemoveInfo(InfoType type) {
	switch (type)
	{
	case InfoType::All:
		DR_infoList.clear();
		break;
	case InfoType::Dead:
		DR_infoList.remove_if([](const Info& t) { return t.deadTime < Clock::Now(); });
		break;
	default:
		DR_infoList.remove_if([&type](const Info& t) { return t.type == type; });
		break;
	}
	return *DR_showing = DR_infoList.size();
}

const unsigned& Draw::Showing() {
	return *DR_showing;
}
