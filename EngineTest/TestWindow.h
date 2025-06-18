#pragma once
#include "Test.h"
#include "..\Platform\PlatformTypes.h"
#include "..\Platform\Platform.h"

using namespace mooncastle;

platform::window windows[4];

LRESULT winProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_DESTROY:
	{
		bool allClosed{ true };

		for (u32 i{ 0 }; i < _countof(windows); ++i)
		{
			if (!windows[i].isClosed())
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

class engineTest : public test
{
public:
    bool initialize() override
    {
        platform::windowInitInfo info[]
        {
            {&winProc, nullptr, L"Test window 1", 100, 100, 400, 800},
            {&winProc, nullptr, L"Test window 2", 150, 150, 800, 400},
            {&winProc, nullptr, L"Test window 3", 200, 200, 400, 400},
            {&winProc, nullptr, L"Test window 4", 250, 250, 800, 600},
        };

        static_assert(_countof(info) == _countof(windows));

        for (u32 i{ 0 }; i < _countof(windows); ++i) 
        {
            windows[i] = platform::createWindow(&info[i]);
        }
        return true;
    }

    void run() override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    void shutdown() override
    {
        for (u32 i{ 0 }; i < _countof(windows); ++i)
        {
            platform::removeWindow(windows[i].getId());
        }
    }
};