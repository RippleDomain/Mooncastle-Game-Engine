#pragma once

#include <thread>
#include <chrono>
#include <string>

#define TEST_ENTITY_COMPONENTS 0
#define TEST_WINDOW 0
#define TEST_RENDERER 1

class test
{
public:
	virtual bool initialize() = 0;
	virtual void run() = 0;
	virtual void shutdown() = 0;
};

#if _WIN64

#include <Windows.h>

class timeIt
{
public:
    using clock = std::chrono::high_resolution_clock;
    using timeStamp = std::chrono::steady_clock::time_point;

    void begin()
    {
        start = clock::now();
    }

    void end()
    {
        auto dt = clock::now() - start;
        msAverage += ((float)std::chrono::duration_cast<std::chrono::milliseconds>(dt).count() - msAverage) / (float)counter;
        ++counter;

        if (std::chrono::duration_cast<std::chrono::seconds>(clock::now() - seconds).count() >= 1)
        {
            OutputDebugStringA("Average frame (ms): ");
            OutputDebugStringA(std::to_string(msAverage).c_str());
            OutputDebugStringA((" " + std::to_string(counter)).c_str());
            OutputDebugStringA(" FPS");
            OutputDebugStringA("\n");

            msAverage = 0.f;
            counter = 1;
            seconds = clock::now();
        }
    }

private:
    float       msAverage{ 0.f };
    int         counter{ 1 };
    timeStamp   start;
    timeStamp   seconds{ clock::now() };
};

#endif