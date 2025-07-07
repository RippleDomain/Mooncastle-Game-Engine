#include "TestRenderer.h"
#include "ShaderCompilation.h"
#include "..\Platform\PlatformTypes.h"
#include "..\Platform\Platform.h"
#include "..\Graphics\Renderer.h"

#if TEST_RENDERER

using namespace mooncastle;

graphics::renderSurface surfaces[4];

timeIt timer{};

bool isRestarting{ false };

void removeRenderSurface(graphics::renderSurface& surface);
bool testInitialize();
void testShutdown();

LRESULT winProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_DESTROY:
	{
		bool allClosed{ true };
		
		for (u32 i{ 0 }; i < _countof(surfaces); ++i)
		{
			if (surfaces[i].window.isValid()) 
			{
				if (surfaces[i].window.isClosed())
				{
					removeRenderSurface(surfaces[i]);
				}
				else
				{
					allClosed = false;
				}
			}
		}
		if (allClosed && !isRestarting)
		{
			PostQuitMessage(0);
			return 0;
		}
	}
	break;

	case WM_SYSCHAR:
		if (wparam == VK_RETURN && (HIWORD(lparam) & KF_ALTDOWN))
		{
			platform::window win{ platform::windowId{(id::idType)GetWindowLongPtr(hwnd, GWLP_USERDATA)} };
			win.setFullScreen(!win.isFullScreen());
			return 0;
		}
		break;
	case WM_KEYDOWN:
		if (wparam == VK_ESCAPE)
		{
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			return 0;
		}
		else if (wparam == VK_F11)
		{
			isRestarting = true;
			testShutdown();
			testInitialize();
		}

	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void createRenderSurface(graphics::renderSurface& surface, platform::windowInitInfo info)
{
	surface.window = platform::createWindow(&info);
	surface.surface = graphics::createSurface(surface.window);
}

void removeRenderSurface(graphics::renderSurface& surface)
{
	graphics::renderSurface temp{ surface };
	surface = {};

	if (temp.surface.isValid())
	{
		graphics::removeSurface(temp.surface.getId());
	}

	if (temp.window.isValid())
	{
		platform::removeWindow(temp.window.getId());
	}
}

bool testInitialize()
{
	while (!compileShaders())
	{
		//Pop up a message box allowing the user to retry compilation.
		if (MessageBox(nullptr, L"Failed to compile engine shaders.", L"Shader Compilation Error", MB_RETRYCANCEL) != IDRETRY)
		{
			return false;
		}
	}

	if (!graphics::initialize(graphics::graphicsPlatform::direct3D12)) return false;

	platform::windowInitInfo info[]
	{
		{&winProc, nullptr, L"Render Window 1", 500, 100, 400, 800},
		{&winProc, nullptr, L"Render Window 2", 550, 150, 800, 400},
		{&winProc, nullptr, L"Render Window 3", 600, 200, 400, 400},
		{&winProc, nullptr, L"Render Window 4", 650, 250, 800, 600},
	};

	static_assert(_countof(info) == _countof(surfaces));

	for (u32 i{ 0 }; i < _countof(surfaces); ++i)
	{
		createRenderSurface(surfaces[i], info[i]);
	}

	isRestarting = false;

	return true;
}

void testShutdown()
{
	for (u32 i{ 0 }; i < _countof(surfaces); ++i)
	{
		removeRenderSurface(surfaces[i]);
	}

	graphics::shutdown();
}

bool engineTest::initialize()
{
	return testInitialize();
}

void engineTest::run()
{
	timer.begin();

	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	for (u32 i{ 0 }; i < _countof(surfaces); ++i)
	{
		if (surfaces[i].surface.isValid())
		{
			surfaces[i].surface.render();
		}
	}

	timer.end();
}

void engineTest::shutdown()
{
	return testShutdown();
}

#endif