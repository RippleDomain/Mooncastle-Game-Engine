#include "TestRenderer.h"
#include "..\Platform\PlatformTypes.h"
#include "..\Platform\Platform.h"
#include "..\Graphics\Renderer.h"

using namespace mooncastle;

graphics::renderSurface surfaces[4];

LRESULT winProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_DESTROY:
	{
		bool allClosed{ true };

		for (u32 i{ 0 }; i < _countof(surfaces); ++i)
		{
			if (!surfaces[i].window.isClosed())
			{
				allClosed = false;
			}
		}
		if (allClosed)
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
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void createRenderSurface(graphics::renderSurface& surface, platform::windowInitInfo info)
{
	surface.window = platform::createWindow(&info);
}

void removeRenderSurface(graphics::renderSurface& surface)
{
	platform::removeWindow(surface.window.getId());
}

bool engineTest::initialize()
{
	bool result{ graphics::initialize(graphics::graphicsPlatform::direct3D12) };
	if (!result) return result;

	platform::windowInitInfo info[]
	{
		{&winProc, nullptr, L"Test window 1", 100, 100, 400, 800},
		{&winProc, nullptr, L"Test window 2", 150, 150, 800, 400},
		{&winProc, nullptr, L"Test window 3", 200, 200, 400, 400},
		{&winProc, nullptr, L"Test window 4", 250, 250, 800, 600},
	};

	static_assert(_countof(info) == _countof(surfaces));

	for (u32 i{ 0 }; i < _countof(surfaces); ++i)
	{
		createRenderSurface(surfaces[i], info[i]);
	}

	return result;
}

void engineTest::run()
{
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void engineTest::shutdown()
{
	for (u32 i{ 0 }; i < _countof(surfaces); ++i)
	{
		removeRenderSurface(surfaces[i]);
	}

	graphics::shutdown();
}