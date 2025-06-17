#ifdef _WIN64

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <crtdbg.h>

#ifndef USE_WITH_EDITOR

extern bool engineInitialize();
extern void engineUpdate();
extern void engineShutdown();

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
#if _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
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