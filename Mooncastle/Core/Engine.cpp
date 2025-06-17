#if !defined(SHIPPING)

#include "..\Content\ContentLoader.h"
#include "..\Components\Script.h"

#include <thread>

bool engineInitialize() 
{
	bool result{ mooncastle::content::loadGame() };
	return result;
}

void engineUpdate()
{
	mooncastle::script::update(10.0);
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void engineShutdown()
{
	mooncastle::content::unloadGame();
}

#endif