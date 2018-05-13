#pragma once

#include <string>

namespace Encoding {
	std::string Utf16ToUtf8(const wchar_t* utf16str);
	std::wstring Utf8ToUtf16(const char* utf8str);

	std::wstring StrToUtf16(const char* str, const char* codename);
	std::string Utf16ToStr(const wchar_t* utf16str, const char* codename);

	std::string StrToUtf8(const char* str, const char* codename);
	std::string Utf8ToStr(const char* utf8str, const char* codename);

	inline static std::string Utf16ToUtf8(const std::wstring& utf16str) { return Utf16ToUtf8(utf16str.c_str()); }
	inline static std::wstring Utf8ToUtf16(const std::string& utf8str) { return Utf8ToUtf16(utf8str.c_str()); }

	inline static std::wstring StrToUtf16(const std::string& str, const char* codename) { return StrToUtf16(str.c_str(), codename); }
	inline static std::string Utf16ToStr(const std::wstring& utf16str, const char* codename) { return Utf16ToStr(utf16str.c_str(), codename); }

	inline static std::string StrToUtf8(const std::string& str, const char* codename) { return StrToUtf8(str.c_str(), codename); }
	inline static std::string Utf8ToStr(const std::string& utf8str, const char* codename) { return Utf8ToStr(utf8str.c_str(), codename); }
}
