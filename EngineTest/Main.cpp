#include "Test.h"

#pragma comment(lib, "mooncastle.lib")

#if TEST_ENTITY_COMPONENTS
#include "TestEntityComponents.h"
#elif TEST_WINDOW
#include "TestWindow.h"
#elif TEST_RENDERER
#include "TestRenderer.h"
#else
#error "Entity components test is not enabled. Please set TEST_ENTITY_COMPONENTS to 1 to enable it."
#endif

#ifdef _WIN64
#include <Windows.h>
#include "filesystem"

std::filesystem::path setCurrentDirToExecutablePath()
{
    wchar_t path[MAX_PATH];
    const uint32_t pathLength{ GetModuleFileName(0, &path[0], MAX_PATH) };

    if (!pathLength || GetLastError() == ERROR_INSUFFICIENT_BUFFER) return {};

    std::filesystem::path p{ path };
    std::filesystem::current_path(p.parent_path());

    return std::filesystem::current_path();
}

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
#if _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    setCurrentDirToExecutablePath();

    engineTest test{ };

    if (test.initialize())
    {
        MSG msg{};
        bool isRunning{ true };

        while (isRunning)
        {
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);

                isRunning &= (msg.message != WM_QUIT);
            }

            test.run();
        }
    }
    test.shutdown();
    return 0;
}

#else

int main() 
{
#if _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	engineTest test{ };
	if (test.initialize()) 
	{
		test.run();
	}
	test.shutdown();
}

#endif