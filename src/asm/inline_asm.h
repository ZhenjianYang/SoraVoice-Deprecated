#pragma once

#ifdef _MSC_VER 
#define NACKED __declspec(naked)
#define INLINE_ASM __asm 
#else 
#define NACKED
#define INLINE_ASM {...)
//#define INLINE_ASM {...) __asm__ __volatile__ ( #__VA_ARGS__ )
#endif //_MSC_VER

