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

//Runs a test for getting/setting texture data with some kind of precise format.
template<typename T>
void TestTextureGetSetExact(Format texFormat, ComponentData dataFormat, T testVal)
{
    std::string testCaseName = "{";
    testCaseName += ToString(texFormat);
    testCaseName += ": ";
    testCaseName += dataFormat._to_string();
    testCaseName += "}";
    TEST_CASE(testCaseName.c_str());

    Texture2D tex(glm::uvec2{ 1, 1 }, texFormat);
    tex.SetData(&testVal, dataFormat);
    
    T outputTestVal;
    tex.GetData(&outputTestVal, dataFormat);

    std::string errMsg = "Expected ";
    errMsg += std::to_string(testVal);
    errMsg += " but got ";
    errMsg += std::to_string(outputTestVal);
    TEST_CHECK_(outputTestVal == testVal, errMsg.c_str());
}
template<typename T>
void TestTextureGetSetExactMulti(Format texFormat, ComponentData dataFormat,
                                 const T* testData, size_t testDataCount)
{
    std::string testCaseName = "{";
    testCaseName += ToString(texFormat);
    testCaseName += ": ";
    testCaseName += dataFormat._to_string();
    testCaseName += "}";
    TEST_CASE(testCaseName.c_str());

    Texture2D tex(glm::uvec2{ 1, 1 }, texFormat);
    tex.SetData(testData, dataFormat);
    
    std::vector<T> outputTestVal;
    outputTestVal.resize(testDataCount);
    tex.GetData(outputTestVal.data(), dataFormat);

    for (size_t i = 0; i < outputTestVal.size(); ++i)
        TEST_CHECK(outputTestVal[i] == testData[i]);
}
void TextureSimpleGetSetData()
{
    Simple::Run(
        //Update:
        [&](float deltaT)
        {
            //Test get/set of exact single-channel values:
            TestTextureGetSetExact(
                Format{ SimpleFormat{FormatTypes::NormalizedUInt,
                                     FormatComponents::R,
                                     BitDepths::B8} },
                +ComponentData::Red,
                (glm::u8)203);
            TestTextureGetSetExact(
                Format{ SimpleFormat{FormatTypes::NormalizedUInt,
                                     FormatComponents::RG,
                                     BitDepths::B8} },
                +ComponentData::Red,
                (glm::u8)203);
            TestTextureGetSetExact(
                Format{ SimpleFormat{FormatTypes::NormalizedUInt,
                                     FormatComponents::RG,
                                     BitDepths::B8} },
                +ComponentData::Green,
                (glm::u8)203);
            TestTextureGetSetExact(
                Format{ SimpleFormat{FormatTypes::NormalizedUInt,
                                     FormatComponents::RGBA,
                                     BitDepths::B8} },
                +ComponentData::Blue,
                (glm::u8)203);
            TestTextureGetSetExact(
                Format{ SimpleFormat{FormatTypes::NormalizedUInt,
                                    FormatComponents::RGB,
                                    BitDepths::B5} },
                +ComponentData::Green,
                (glm::u8)16);
            TestTextureGetSetExact(
                Format{ SimpleFormat{FormatTypes::NormalizedInt,
                                     FormatComponents::RG,
                                     BitDepths::B8} },
                +ComponentData::Red,
                (glm::i8)67);
            TestTextureGetSetExact(
                Format{ SimpleFormat{FormatTypes::NormalizedInt,
                                     FormatComponents::RG,
                                     BitDepths::B8} },
                +ComponentData::Red,
                (glm::i8)(-67));
            TestTextureGetSetExact(
                Format{ SimpleFormat{FormatTypes::Float,
                                     FormatComponents::RGB,
                                     BitDepths::B32} },
                +ComponentData::Red,
                (glm::f32)123.456f);
            TestTextureGetSetExact(
                Format{ SpecialFormats::RGB10_A2 },
                +ComponentData::Green,
                (glm::u8)32);

            //Test get/set of exact multi-channel values.
            glm::u8vec2 v2_u8{ 201, 203 };
            TestTextureGetSetExactMulti(
                Format{SimpleFormat{FormatTypes::UInt,
                                    FormatComponents::RGB,
                                    BitDepths::B16}},
                +ComponentData::RG,
                glm::value_ptr(v2_u8), 2);

            Simple::App->Quit(true);
        },
        //Render:
        [&](float deltaT)
        {
        },
        //Quit:
        [&]()
        {
        });
}

//TODO: TextureSubRectData()