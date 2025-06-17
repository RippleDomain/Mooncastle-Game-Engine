#pragma comment(lib, "mooncastle.lib")

#define TEST_ENTITY_COMPONENTS 1

#if TEST_ENTITY_COMPONENTS
#include "TestEntityComponents.h"
#else
#error "Entity components test is not enabled. Please set TEST_ENTITY_COMPONENTS to 1 to enable it."
#endif

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
	test.shutDown();
}