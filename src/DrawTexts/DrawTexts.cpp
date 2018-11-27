#include "DrawTexts.h"

#include <SVData.h>
#include <Utils/Encoding.h>
#include <Utils/ApiPack.h>

#include <stdint.h>
#include <memory>
#include <fstream>

#include <ft2build.h>
#include FT_FREETYPE_H

using ColorType = uint16_t;
constexpr char TTF_FILENAME[] = "voice/font.ttf";
constexpr int MAX_TTFFILE_SIZE = 100 * 1024 * 1024;
constexpr char CODENAME_MS932[] = ".932";

int FontSizeTable[] = {
	0x08, 0x0c, 0x10, 0x14,
	0x18, 0x20, 0x12, 0x1a,
	0x1e, 0x24, 0x28, 0x2c,
	0x30, 0x32, 0x36, 0x3c,
	0x40, 0x48, 0x50, 0x60,
	0x80, 0x90, 0xa0, 0xc0,
};

ColorType FontColorTable[] = {
	0x0fff, 0x0fc7, 0x0f52, 0x08cf, 0x0fb4, 0x08fa, 0x0888, 0x0fee, 0x0853, 0x0333,
	0x0ca8, 0x0fdb, 0x0ace, 0x0cff, 0x056b, 0x0632, 0x0135, 0x0357, 0x0bbb,
};

using namespace std;

static unique_ptr<char[]> DT_ttf_buff;
static unsigned DT_ttf_size = 0;

static FT_Library DT_library = nullptr;
static FT_Face DT_face = nullptr;

namespace FT_APIS {
#define ADD_FT_API(name) static decltype(::name) *name;\
				static constexpr char STR_##name[] = #name;
	
	ADD_FT_API(FT_Init_FreeType);
	ADD_FT_API(FT_New_Memory_Face);
	ADD_FT_API(FT_Done_Face);
	ADD_FT_API(FT_Done_FreeType);
	ADD_FT_API(FT_Set_Pixel_Sizes);
	ADD_FT_API(FT_Set_Transform);
	ADD_FT_API(FT_Load_Char);
} ;

inline static unique_ptr<char[]> getTtfData(const char* ttf, unsigned& out_ttf_size);
inline static bool setFTApis();

bool DrawTexts::Init()
{
	if (!setFTApis()) return false;

	DT_ttf_buff = getTtfData(TTF_FILENAME, DT_ttf_size);
	if (!DT_ttf_buff) return false;

	FT_Error error;

	error = FT_APIS::FT_Init_FreeType(&DT_library);
	error = FT_APIS::FT_New_Memory_Face(DT_library, (FT_Byte*)DT_ttf_buff.get(), DT_ttf_size, 0, &DT_face);
	if (error) { End(); return false; }

	return false;
}

bool DrawTexts::End()
{
	FT_APIS::FT_Done_Face(DT_face);
	FT_APIS::FT_Done_FreeType(DT_library);

	DT_face = nullptr;
	DT_library = nullptr;
	return false;
}

void * DrawTexts::DrawTexts(const char * text, void * buffer, unsigned stride, unsigned color_index)
{
	if (!DT_face || !text || !text[0]) return buffer;
	wstring wstr = Encoding::Utf8ToUtf16(text);
	if(wstr.length() == 0) wstr = Encoding::StrToUtf16(text, CODENAME_MS932);
	if(wstr.length() == 0) return buffer;

	const ColorType color = FontColorTable[color_index];
	const unsigned fontSize = FontSizeTable[*(unsigned*)SV.addrs.p_fontsize];
	const int pixels_line = stride / 2;

	FT_Error error;
	error = FT_APIS::FT_Set_Pixel_Sizes(DT_face, 0, fontSize);

	ColorType* buff = (ColorType*)buffer;
	FT_GlyphSlot  slot = DT_face->glyph;
	FT_Vector     pen;
	FT_Matrix     matrix;

	matrix.xx = (FT_Fixed)(cos(0) * 0x10000L);
	matrix.xy = (FT_Fixed)(-sin(0) * 0x10000L);
	matrix.yx = (FT_Fixed)(sin(0) * 0x10000L);
	matrix.yy = (FT_Fixed)(cos(0) * 0x10000L);
	pen.x = 0, pen.y = 0;

	for (auto ch : wstr) {
		FT_APIS::FT_Set_Transform(DT_face, &matrix, &pen);
		error = FT_APIS::FT_Load_Char(DT_face, ch, FT_LOAD_RENDER);
		if (error) continue;

		auto bitmap_buffer = slot->bitmap.buffer;
		int width = slot->bitmap.width;
		int height = slot->bitmap.rows;

		for (int y = 0, py = fontSize - slot->bitmap_top - fontSize / 4; y < height; y++, py++) {
			if (py < 0) continue;
			for (int x = 0, px = slot->bitmap_left; x < width && px < pixels_line; x++, px++) {
				if (px < 0) continue;
				buff[py*pixels_line + px] = (((ColorType)bitmap_buffer[y*width + x] & 0xF0) << 8) | color;
			}
		}

		pen.x += slot->advance.x;
		pen.y += slot->advance.y;
	}
	return buff + pen.x/ 64;
}

unique_ptr<char[]> getTtfData(const char * ttf, unsigned& out_ttf_size) {
	ifstream ifs(ttf, ios::in | ios::binary);
	if (!ifs) return nullptr;

	ifs.seekg(0, ios::end);
	std::size_t size = (std::size_t)ifs.tellg();
	ifs.seekg(0, ios::beg);

	if (size > MAX_TTFFILE_SIZE) return nullptr;

	unique_ptr<char[]> data = std::make_unique<char[]>(size);
	ifs.read(data.get(), size);
	ifs.close();

	out_ttf_size = size;
	return data;
}

bool setFTApis() {
	return false;
}
