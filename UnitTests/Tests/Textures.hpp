#pragma once

#include <random>

#define TEST_NO_MAIN
#include <acutest.h>

#include <B+/Renderer/Textures/Texture.h>
#include "../SimpleApp.h"

using namespace Bplus::GL::Textures;


void TextureCreation()
{
    std::unique_ptr<Texture2D> tex;
    Simple::Run(
        //Update:
        [&](float deltaT)
        {
            TEST_CASE("Simple RGBA 8");
            tex = std::make_unique<Texture2D>(glm::uvec2{ 1, 1 },
                                              SimpleFormat{+FormatTypes::NormalizedUInt,
                                                           +FormatComponents::RGBA,
                                                           +BitDepths::B8});
            tex.reset();

            TEST_CASE("Simple RG F32");
            tex = std::make_unique<Texture2D>(glm::uvec2{ 2, 2 },
                                              SimpleFormat{+FormatTypes::Float,
                                                           +FormatComponents::RG,
                                                           +BitDepths::B32});
            tex.reset();

            TEST_CASE("Simple R I16");
            tex = std::make_unique<Texture2D>(glm::uvec2{ 3, 7 },
                                              SimpleFormat{+FormatTypes::Int,
                                                           +FormatComponents::R,
                                                           +BitDepths::B16});
            tex.reset();

            TEST_CASE("Special: RGB10 A2 UInt");
            tex = std::make_unique<Texture2D>(glm::uvec2{ 31, 33 },
                                              +SpecialFormats::RGB10_A2_UInt);
            tex.reset();

            TEST_CASE("Special: RGB9 e5");
            tex = std::make_unique<Texture2D>(glm::uvec2{ 41, 39 },
                                              +SpecialFormats::RGB_SharedExpFloats);
            tex.reset();

            TEST_CASE("Special: sRGB_LinA");
            tex = std::make_unique<Texture2D>(glm::uvec2{ 41, 39 },
                                              +SpecialFormats::sRGB_LinearAlpha);
            tex.reset();

            TEST_CASE("Compressed: Greyscale signed");
            tex = std::make_unique<Texture2D>(glm::uvec2{ 31, 32 },
                                              +CompressedFormats::Greyscale_NormalizedInt);
            tex.reset();

            TEST_CASE("Compressed: RG unsigned");
            tex = std::make_unique<Texture2D>(glm::uvec2{ 1, 3 },
                                              +CompressedFormats::RG_NormalizedUInt);
            tex.reset();

            TEST_CASE("Compressed: RGB unsigned float");
            tex = std::make_unique<Texture2D>(glm::uvec2{ 5, 4 },
                                              +CompressedFormats::RGB_UFloat);
            tex.reset();

            TEST_CASE("Compressed: RGBA sRGB");
            tex = std::make_unique<Texture2D>(glm::uvec2{ 31, 32 },
                                              +CompressedFormats::RGBA_sRGB_NormalizedUInt);
            tex.reset();

            TEST_CASE("Depth: 24U");
            tex = std::make_unique<Texture2D>(glm::uvec2{ 1920, 1080 },
                                              +DepthStencilFormats::Depth_24U);
            tex.reset();

            TEST_CASE("Stenci: 8U");
            tex = std::make_unique<Texture2D>(glm::uvec2{ 1920, 1080 },
                                              +DepthStencilFormats::Stencil_8);
            tex.reset();

            TEST_CASE("Depth/Stencil: 32F, 8U");
            tex = std::make_unique<Texture2D>(glm::uvec2{ 1921, 1081 },
                                              +DepthStencilFormats::Depth32F_Stencil8);
            tex.reset();

            Simple::App->Quit(true);
        },
        //Render:
        [&](float deltaT)
        {
        },
        //Quit:
        [&]()
        {
            tex.reset();
        });
}

void TextureSimpleGetSetData()
{
    std::unique_ptr<Texture2D> tex;
    Simple::Run(
        //Update:
        [&](float deltaT)
        {
            tex = std::make_unique<Texture2D>(glm::uvec2{ 1, 1 },
                                              SimpleFormat{FormatTypes::NormalizedUInt,
                                                           FormatComponents::RGBA,
                                                           BitDepths::B8});

            //TODO: Implement.

            Simple::App->Quit(true);
        },
        //Render:
        [&](float deltaT)
        {
        },
        //Quit:
        [&]()
        {
            tex.reset();
        });
}