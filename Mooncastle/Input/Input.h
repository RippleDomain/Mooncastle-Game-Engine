#pragma once

#include "EngineAPI/Input.h"
#include "CommonHeaders.h"

namespace mooncastle::input
{
	void bind(inputSource source);
	void unbind(inputSource::type type, inputCode::code code);
	void unbind(u64 binding);
	void set(inputSource::type type, inputCode::code code, math::v3 value);
}