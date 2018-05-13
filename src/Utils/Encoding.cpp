#include "Encoding.h"

#include <locale>
#include <codecvt>

using namespace std;

std::string Encoding::Utf16ToUtf8(const wchar_t* utf16str)
{
	wstring_convert<codecvt_utf8<wchar_t>> wconv;
	return wconv.to_bytes(utf16str);
}

std::wstring Encoding::Utf8ToUtf16(const char* utf8str)
{
	wstring_convert<codecvt_utf8<wchar_t>> wconv;
	return wconv.from_bytes(utf8str);
}

std::wstring Encoding::StrToUtf16(const char* str, const char* codename)
{
	using conv = std::codecvt_byname<wchar_t, char, std::mbstate_t>;
	wstring_convert<conv> con(new conv(codename));
	return con.from_bytes(str);
}

std::string Encoding::Utf16ToStr(const wchar_t* utf16str, const char* codename)
{
	using conv = std::codecvt_byname<wchar_t, char, std::mbstate_t>;
	wstring_convert<conv> con(new conv(codename));
	return con.to_bytes(utf16str);
}

std::string Encoding::StrToUtf8(const char* str, const char* codename)
{
	return Utf16ToUtf8(StrToUtf16(str, codename));
}

std::string Encoding::Utf8ToStr(const char* utf8str, const char* codename)
{
	return Utf16ToStr(Utf8ToUtf16(utf8str), codename);
}
