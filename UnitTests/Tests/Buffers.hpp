#pragma once

#define TEST_NO_MAIN
#include <acutest.h>

#include <B+/Renderer/Buffer.h>
#include "../SimpleApp.h"


void BuffersBasic()
{
    std::unique_ptr<Bplus::GL::Buffer> buffer;
    Simple::Run(
        //Update:
        [&](float deltaT)
        {
            buffer = std::make_unique<Bplus::GL::Buffer>();
        },
        //Render:
        [&](float deltaT)
        {

        },
        //Quit:
        [&]()
        {
            buffer.reset();
        });
}