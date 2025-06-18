#pragma comment(lib, "mooncastle.lib")

#define TEST_ENTITY_COMPONENTS 0
#define TEST_WINDOW 1

#if TEST_ENTITY_COMPONENTS
#include "TestEntityComponents.h"
#elif TEST_WINDOW
#include "TestWindow.h"
#else
#error "Entity components test is not enabled. Please set TEST_ENTITY_COMPONENTS to 1 to enable it."
#endif

#ifdef _WIN64
#include <Windows.h>

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
#if _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

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