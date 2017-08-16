#include "Draw.h"

#include "Draw_D3D.h"

#include <InitParam.h>

#include <Utils/Log.h>
#include <Utils/Clock.h>
#include <Utils/EncodeHelper.h>

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

constexpr char STR_D3DXCreateFontIndirect[] = "D3DXCreateFontIndirect";
constexpr char STR_D3DXCreateSprite[] = "D3DXCreateSprite";

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
	WChar text[MAX_TEXT_LEN + 1];
	RECT rect;
	unsigned color;
	unsigned format;
	InfoType type;

	unsigned deadTime;
};
using PtrInfo = std::unique_ptr<Info>;
using PtrInfoList = std::list<PtrInfo>;

static struct {
	int dx9;
	int sora;

	unsigned* showing;
	const unsigned* dftFormatList;

	D3D* d3d = nullptr;
	PtrInfoList infoList;

	int width = 0;
	int height = 0;
	int vfix = 0;
	int vbound = 0;
	int hbound = 0;
	int linespace = 0;
	int shadow = 0;

	int fontSize = 0;
} draw;

bool Draw::Init(void* initParam, const char* fontName) {
	InitParam* ip = (InitParam*)initParam;

	draw.dx9 = ip->dx9;
	draw.sora = GAME_IS_SORA(ip->game);

	draw.dftFormatList = draw.sora ? DftFormatList_Sora : DftFormatList_ZA;

	RECT rect;
	if (GetClientRect((HWND)*ip->addrs.p_Hwnd, &rect)) {
		draw.width = rect.right - rect.left;
		draw.height = rect.bottom - rect.top;

		if (!draw.sora)
			draw.vfix = (draw.height - draw.width * 9 / 16) / 2;

		draw.fontSize = (draw.height - 2 * draw.vfix) / TEXT_NUM_SCRH;
		if (draw.fontSize < MIN_FONT_SIZE) draw.fontSize = MIN_FONT_SIZE;

		draw.vbound = (int)(draw.fontSize * VBOUND_RATE + 0.5);
		draw.hbound = (int)(draw.fontSize * HBOUND_RATE + 0.5);

		draw.shadow = (int)(draw.fontSize * TEXT_SHADOW_POS_RATE + 0.5);
		draw.linespace = (int)(draw.fontSize * LINE_SPACE_RATE + 0.5);

		LOG("screen width = %d", draw.width);
		LOG("screen height = %d", draw.height);
		LOG("Font Size = %d", draw.fontSize);
	}
	else {
		LOG("Get screen size failed!");
		return false;
	}

	draw.d3d = D3D::GetD3D(draw.dx9, *ip->addrs.p_d3dd, fontName, draw.fontSize);
	return draw.d3d;
}
bool Draw::End() {
	delete draw.d3d;
	draw.d3d = nullptr;

	return false;
}

void Draw::AddInfo(InfoType type, unsigned time, unsigned color, const char* text) {
	unsigned dead = time == ShowTimeInfinity ? TIME_MAX : Clock::Now() + time;
	constexpr auto NumValidType = InfoType::All;
	const unsigned format = NumValidType < NumValidType ? draw.dftFormatList[(int)type] : draw.dftFormatList[(int)InfoType::All];

	LOG("Add text, type = %d", type);
	const int h = int(draw.fontSize * 1.2);

	auto it = draw.infoList.end();

	bool creatNew = true;
	if (type != InfoType::Hello) {
		for (it = draw.infoList.begin(); it != draw.infoList.end(); ++it) {
			if ((*it)->type == type) {
				creatNew = false;
				break;
			}
		}
	}

	if (creatNew) {
		LOG("Create new Text.");
		draw.infoList.push_back(PtrInfo(new Info));
		it = --draw.infoList.end();
	}
	else {
		LOG("No need to create new Text.");
	}
	(*it)->type = type;
	(*it)->color = color;
	(*it)->deadTime = dead;
	(*it)->format = format;

	auto conRst = ConvertUtf8toUtf16((*it)->text, text);

	int text_width = (int)((conRst.cnt1 * 0.8 + conRst.cnt2 * 1.2 + conRst.cnt4 * 1.2) * h);
	LOG("Text is %s", text);
	LOG("Text width is %d", text_width);

	auto& rect = (*it)->rect;

	if (creatNew) {
		memset(&rect, 0, sizeof(rect));
		std::set<int> invalid_bottom, invalid_top;
		for (auto it2 = draw.infoList.begin(); it2 != draw.infoList.end(); ++it2) {
			if (it == it2 || (((*it)->format & DT_RIGHT) != ((*it2)->format & DT_RIGHT))) continue;
			invalid_top.insert((*it2)->rect.top);
			invalid_bottom.insert((*it2)->rect.bottom);
		}

		if ((*it)->format & DT_BOTTOM) {
			for (int bottom = draw.height - draw.vbound - draw.vfix; ; bottom -= h + draw.linespace) {
				if (invalid_bottom.find(bottom) == invalid_bottom.end()) {
					rect.bottom = bottom;
					break;
				}
			}
			rect.top = rect.bottom - h - draw.linespace;
		}
		else {
			for (int top = draw.vbound + draw.vfix; ; top += h + draw.linespace) {
				if (invalid_top.find(top) == invalid_top.end()) {
					rect.top = top;
					break;
				}
			}

			rect.bottom = rect.top + h + draw.linespace;
		}
	} //if (creatNew)

	if ((*it)->format & DT_RIGHT) {
		rect.right = draw.width - draw.hbound;
		rect.left = rect.right - text_width;
	}
	else {
		rect.left = draw.hbound;
		rect.right = rect.left + text_width;
	}

	LOG("top = %d, bottom = %d, left = %d, right = %d", rect.top, rect.bottom, rect.left, rect.right);

	*draw.showing = draw.infoList.size() > 0;
}

void Draw::DrawInfos() {
	if (!draw.d3d) return;

	draw.d3d->BeginScene();

	RECT rect_shadow;
	for (const auto& info : draw.infoList) {
		rect_shadow = info->rect;
		if (info->format & DT_RIGHT) rect_shadow.right += draw.shadow;
		else rect_shadow.left += draw.shadow;
		if (info->format & DT_BOTTOM) rect_shadow.bottom += draw.shadow;
		else rect_shadow.top += draw.shadow;

		unsigned color_shadow = (0xFFFFFF & SHADOW_COLOR) | (0xFF << 24);

		draw.d3d->DrawString(info->text, -1, &rect_shadow, info->format, color_shadow);
	}

	for (const auto& info : draw.infoList) {
		draw.d3d->DrawString(info->text, -1, &info->rect, info->format, info->color);
	}

	draw.d3d->EndScene();
}

void Draw::RemoveInfo(InfoType type) {
	switch (type)
	{
	case InfoType::All:
		draw.infoList.clear();
		break;
	case InfoType::Dead:
		draw.infoList.remove_if([](const PtrInfo& t) { return t->deadTime < Clock::Now(); });
		break;
	default:
		draw.infoList.remove_if([&type](const PtrInfo& t) { return t->type == type; });
		break;
	}
	*draw.showing = draw.infoList.size();
}

const unsigned& Draw::Showing() {
	return *draw.showing;
}
