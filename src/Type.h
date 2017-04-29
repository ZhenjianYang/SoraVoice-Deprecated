#pragma once

using u64 = unsigned long long;
using u32 = unsigned;
using u16 = unsigned short;
using u8 = unsigned char;

using s64 = long long;
using s32 = int;
using s16 = short;
using s8 = char;

static_assert(sizeof(u64) == 8, "size of u64 is not 8!");
static_assert(sizeof(u32) == 4, "size of u32 is not 4!");
static_assert(sizeof(u16) == 2, "size of u16 is not 2!");
static_assert(sizeof(u8) == 1, "size of u8 is not 1!");

static_assert(sizeof(s64) == 8, "size of s64 is not 8!");
static_assert(sizeof(s32) == 4, "size of s32 is not 4!");
static_assert(sizeof(s16) == 2, "size of s16 is not 2!");
static_assert(sizeof(s8) == 1, "size of s8 is not 1!");

