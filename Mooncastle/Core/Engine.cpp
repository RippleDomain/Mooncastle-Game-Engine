#if !defined(SHIPPING)

#include "Content/ContentLoader.h"
#include "Components/Script.h"
#include "Platform/PlatformTypes.h"
#include "Platform/Platform.h"
#include "Graphics/Renderer.h"

#include <thread>

using namespace mooncastle;

namespace
{
	graphics::renderSurface gameWindow{};

	LRESULT winProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		switch (msg)
		{
		case WM_DESTROY:
		{
			if (gameWindow.window.isClosed())
			{
				PostQuitMessage(0);
				return 0;
			}
		}
		break;

		case WM_SYSCHAR:
			if (wparam == VK_RETURN && (HIWORD(lparam) & KF_ALTDOWN))
			{
				gameWindow.window.setFullScreen(!gameWindow.window.isFullScreen());
				return 0;
			}
			break;
		}

		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
}

bool engineInitialize() 
{
	if (mooncastle::content::loadGame() == false) return false;

	platform::windowInitInfo info
	{
		&winProc, nullptr, L"Mooncastle Game" //TODO: Get the game name.
	};

	gameWindow.window = platform::createWindow(&info);
	if (!gameWindow.window.isValid()) return false;

	return true;
}

void engineUpdate()
{
	mooncastle::script::update(10.0);
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void engineShutdown()
{
	platform::removeWindow(gameWindow.window.getId());
	mooncastle::content::unloadGame();
}

#endif