#pragma once

#include <cstdint>

using u64 = std::uint64_t;
using u32 = std::uint32_t;
using u16 = std::uint16_t;
using u8 = std::uint8_t;

using s64 = std::int64_t;
using s32 = std::int32_t;
using s16 = std::int16_t;
using s8 = std::int8_t;

static_assert(sizeof(u64) == 8, "size of u64 is not 8!");
static_assert(sizeof(u32) == 4, "size of u32 is not 4!");
static_assert(sizeof(u16) == 2, "size of u16 is not 2!");
static_assert(sizeof(u8) == 1, "size of u8 is not 1!");

static_assert(sizeof(s64) == 8, "size of s64 is not 8!");
static_assert(sizeof(s32) == 4, "size of s32 is not 4!");
static_assert(sizeof(s16) == 2, "size of s16 is not 2!");
static_assert(sizeof(s8) == 1, "size of s8 is not 1!");

