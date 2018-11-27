#include "Encoding.h"

#include <locale>
#include <codecvt>

using namespace std;

static constexpr char STR_EMPTY[] = "";
static constexpr wchar_t WSTR_EMPTY[] = L"";

std::string Encoding::Utf16ToUtf8(const wchar_t* utf16str)
{
	wstring_convert<codecvt_utf8<wchar_t>> wconv(STR_EMPTY, WSTR_EMPTY);
	return wconv.to_bytes(utf16str);
}

std::wstring Encoding::Utf8ToUtf16(const char* utf8str)
{
	wstring_convert<codecvt_utf8<wchar_t>> wconv(STR_EMPTY, WSTR_EMPTY);
	return wconv.from_bytes(utf8str);
}

std::wstring Encoding::StrToUtf16(const char* str, const char* codename)
{
	using conv = std::codecvt_byname<wchar_t, char, std::mbstate_t>;
	wstring_convert<conv> con(new conv(codename));
	auto str_end = str;
	while (*str_end) str_end++;

	auto wstr = con.from_bytes(str, str_end);
	if (wstr.length() == str_end - str) {
		for (auto ch : wstr) if (ch >= 0x80) return WSTR_EMPTY;
	}
	return wstr;
}

std::string Encoding::Utf16ToStr(const wchar_t* utf16str, const char* codename)
{
	using conv = std::codecvt_byname<wchar_t, char, std::mbstate_t>;
	wstring_convert<conv> con(new conv(codename));
	auto str_end = utf16str;
	while (*str_end) str_end++;

	auto str = con.to_bytes(utf16str, str_end);
	if (str.length() == str_end - utf16str) {
		for (auto ch : str) if (ch >= 0x80) return STR_EMPTY;
	}
	return str;
}

std::string Encoding::StrToUtf8(const char* str, const char* codename)
{
	return Utf16ToUtf8(StrToUtf16(str, codename));
}

std::string Encoding::Utf8ToStr(const char* utf8str, const char* codename)
{
	return Utf16ToStr(Utf8ToUtf16(utf8str), codename);
}
