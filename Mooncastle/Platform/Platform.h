#pragma once

#include "CommonHeaders.h"
#include "Window.h"

namespace mooncastle::platform {

	struct windowInitInfo;

	window createWindow(const windowInitInfo* const initInfo = nullptr);
	void removeWindow(windowId id);
}