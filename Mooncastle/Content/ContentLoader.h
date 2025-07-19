#pragma once

#include "CommonHeaders.h"

#if !defined(SHIPPING) && defined(_WIN64)

namespace mooncastle::content
{
	bool loadGame();
	void unloadGame();
	bool loadEngineShaders(std::unique_ptr<u8[]>& shaders, u64& size);
}

#endif