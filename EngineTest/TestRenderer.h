#pragma once

#include "Test.h"

class engineTest : public test
{
public:
    bool initialize() override;
    void run() override;
    void shutdown() override;
};