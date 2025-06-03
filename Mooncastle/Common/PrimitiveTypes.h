#pragma once

#include <stdint.h>

//Signed integer types
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

//Unsigned integer types
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

//Invalid integer types
constexpr u64 u64_invalid_id{ 0xffff'ffff'ffff'ffffui64 };
constexpr u32 u32_invalid_id{ 0xffff'ffffui32 };
constexpr u16 u16_invalid_id{ 0xffffui16 };
constexpr u8 u8_invalid_id{ 0xffui8 };

//Floating point types
using f32 = float;