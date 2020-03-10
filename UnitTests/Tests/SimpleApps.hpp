#pragma once

#define TEST_NO_MAIN
#include <acutest.h>

#include <glm/gtx/string_cast.hpp>

#include "../SimpleApp.h"


void SimpleApps()
{
    glm::u8vec4 backCol1{ 45, 80, 206, 255 },
                backCol2{ 254, 2, 145, 150 };
    float colorT = 0.0f;

    Simple::Run(
        //Update:
        [&](float deltaT)
        {
            colorT += deltaT * 0.75f;
            colorT -= (int)colorT;

            ImGui::Text("I'm label 3.");

            int nKeys;
            const auto* keyPresses = SDL_GetKeyboardState(&nKeys);

            //Pressing Space constitutes a test failure.
            //Pressing Enter constitutes a test success.
            bool testPassed = TEST_CHECK_(!keyPresses[SDL_SCANCODE_SPACE],
                                          "The user pressed Space, \
indicating that not all ImGUI labels were visible.");
            if (!testPassed ||
                keyPresses[SDL_SCANCODE_KP_ENTER] ||
                keyPresses[SDL_SCANCODE_RETURN] ||
                keyPresses[SDL_SCANCODE_RETURN2])
            {
                Simple::App->Quit(true);
            }
        },
        //Render:
        [&](float deltaT)
        {
            ImGui::Text("I'm label 1.");

            auto backCol = glm::mix((glm::fvec4)backCol1,
                                    (glm::fvec4)backCol2,
                                    colorT);
            backCol /= 255;

            auto* context = Bplus::GL::Context::GetCurrentContext();
            context->Clear(backCol);

            ImGui::Text("I'm label 2.");

            ImGui::Text("If you see all four labels including this one,\nPress Enter. \
Else, press Space.");
        },
        //Quit:
        [&]()
        {

        });
}