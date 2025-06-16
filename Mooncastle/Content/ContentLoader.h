#pragma once

#include "CommonHeaders.h"

#if !defined(SHIPPING)

namespace mooncastle::content
{
	bool loadGame();
	void unloadGame();
}

#endif