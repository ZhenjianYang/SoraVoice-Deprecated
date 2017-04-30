#pragma once

#include <type_traits>

struct ConvertResult {
	int cnt1;
	int cnt2;
	int cnt4;
};
static const ConvertResult ResultZero {};

template<typename CharType, typename = std::enable_if_t<(sizeof(CharType) >= 2)>>
static ConvertResult ConvertUtf8toUtf16(CharType* dst_buff, int size_dst, const char* src_buff) {
	constexpr unsigned char f2  = 0b11000000;
	constexpr unsigned char f5  = 0b11111000;
	
	constexpr unsigned char lmin = 0b10000000;
	constexpr unsigned char lmax = 0b10111111;
	constexpr unsigned char lmk  = 0b00111111;
		
	ConvertResult rst {};
	const unsigned char* src = (decltype(src))src_buff;
	int i = 0;
	while(i < size_dst - 1) {
		if(*src == 0) break;
		else if(*src <= 0x7F) {
			rst.cnt1++;
			dst_buff[i++] = *(src++);
		}
		else {
			unsigned unicode = 0;
			auto first = *(src++);
			unsigned f, len;
			for(len = 1, f = f2; f <= f5; len++, src++, (f >>= 1) |= 0b10000000){
				if((first & f) != f) break;
				if(*src >= lmin && *src <= lmax) {
					(unicode <<= 6) |= *src & lmk;
				}
				else {
					len = 0; 
					break; 
				}
			}
			if(len > 4 || len <= 1) {
				dst_buff[0] = 0;
				return ResultZero;
			}
			unicode |= (first & ~f) << (6 * (len - 1));
			if(unicode > 0x10FFFF) {
				dst_buff[0] = 0;
				return ResultZero;
			}
			if(unicode <= 0xFFFF) {
				if(unicode >= 0xD800 && unicode <= 0xDFFF) {
					dst_buff[0] = 0;
					return ResultZero;
				}
				dst_buff[i++] = (CharType)unicode;
				rst.cnt2++;
			}
			else {
				unsigned tmp = unicode - 0x10000;
				dst_buff[i++] = (CharType)((tmp >> 10)   | 0xD800);
				dst_buff[i++] = (CharType)((tmp & 0x3FF) | 0xDC00);
				rst.cnt4++;
			}
		}
	}
	dst_buff[i] = 0;
	return rst;
}

template<typename CharArray,typename = std::enable_if_t<
			std::is_array<CharArray>::value && (sizeof(CharArray) / std::extent<CharArray>::value >= 2)>>
static ConvertResult ConvertUtf8toUtf16(CharArray &dst_array, const char* src_buff) {
	constexpr int len = std::extent<CharArray>::value;
	return ConvertUtf8toUtf16(dst_array, len, src_buff);
}
