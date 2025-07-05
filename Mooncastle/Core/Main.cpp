#include "CommonHeaders.h"

#ifdef _WIN64

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <crtdbg.h>
#include "filesystem"

namespace
{
    std::filesystem::path setCurrentDirToExecutablePath()
    {
        wchar_t path[MAX_PATH];
        const uint32_t pathLength{ GetModuleFileName(0, &path[0], MAX_PATH) };

        if (!pathLength || GetLastError() == ERROR_INSUFFICIENT_BUFFER) return {};

        std::filesystem::path p{ path };
        std::filesystem::current_path(p.parent_path());

        return std::filesystem::current_path();
    }
}

#ifndef USE_WITH_EDITOR

extern bool engineInitialize();
extern void engineUpdate();
extern void engineShutdown();

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
#if _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    setCurrentDirToExecutablePath();

    if (engineInitialize())
    {
        MSG msg{};
        bool is_running{ true };

        while (is_running)
        {
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);

                is_running &= (msg.message != WM_QUIT);
            }

            engineUpdate();
        }
    }
    engineShutdown();
    return 0;
}

#endif
#endif