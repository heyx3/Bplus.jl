#pragma once

#include <random>

#define TEST_NO_MAIN
#include <acutest.h>

#include <B+/Renderer/Textures/TextureD.hpp>
#include <B+/Renderer/Textures/TextureCube.h>
#include "../SimpleApp.h"

using namespace Bplus::GL::Textures;


void TestTextureCreation(const char* testName,
                         glm::uvec2 size, Format simpleFormat,
                         uint_mipLevel_t nMips = 0)
{
    TEST_CASE(testName);

    Texture2D tex{ size, simpleFormat, nMips };
    Texture2D tex2(std::move(tex));
}
void TextureCreation()
{
    Simple::Run(
        //Update:
        [&](float deltaT)
        {
            TestTextureCreation("Simple RGBA 8", { 1, 1 },
                                SimpleFormat{ +FormatTypes::NormalizedUInt,
                                              +FormatComponents::RGBA,
                                              +BitDepths::B8 });
            TestTextureCreation("Simple RG F32", { 2, 2 },
                                SimpleFormat{ +FormatTypes::Float,
                                              +FormatComponents::RG,
                                              +BitDepths::B32 });
            TestTextureCreation("Simple R I16", { 3, 7 },
                                SimpleFormat{ +FormatTypes::Int,
                                              +FormatComponents::R,
                                              +BitDepths::B16 });
            TestTextureCreation("Special: RGB10 A2 UInt", { 31, 33 },
                                +SpecialFormats::RGB10_A2_UInt);

            TestTextureCreation("Special: RGB9 e5", { 41, 39 },
                                +SpecialFormats::RGB_SharedExpFloats);
            TestTextureCreation("Special: sRGB_LinA", { 41, 39 },
                                +SpecialFormats::sRGB_LinearAlpha);

            TestTextureCreation("Compressed: Greyscale signed", { 31, 32 },
                                +CompressedFormats::Greyscale_NormalizedInt);
            TestTextureCreation("Compressed: RG unsigned", { 1, 3 },
                                +CompressedFormats::RG_NormalizedUInt);
            TestTextureCreation("Compressed: RGB unsigned float", { 5, 4 },
                                +CompressedFormats::RGB_UFloat);
            TestTextureCreation("Compressed: RGBA sRGB", { 32, 129 },
                                +CompressedFormats::RGBA_sRGB_NormalizedUInt);

            TestTextureCreation("Depth: 24U", { 1920, 1080 },
                                +DepthStencilFormats::Depth_24U);
            TestTextureCreation("Stencil: 8U", { 1920, 1080 },
                                +DepthStencilFormats::Stencil_8);
            TestTextureCreation("Depth/Stencil: 32F, 8U", { 1921, 1079 },
                                +DepthStencilFormats::Depth32F_Stencil8);

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

//Runs a test for getting/setting texture data with some kind of precise format.
template<typename T>
void TestTextureGetSetSingle(SimpleFormat texFormat, T testVal)
{
    std::string testCasePrefix = "{";
    testCasePrefix += ToString(texFormat);
    testCasePrefix += ": ";
    for (uint8_t i = 0; i < std::min((uint8_t)3, texFormat.Components._to_integral()); ++i)
    {
        ComponentData components;
        switch (i)
        {
            case 0: components = ComponentData::Red; break;
            case 1: components = ComponentData::Green; break;
            case 2: components = ComponentData::Blue; break;
            default:
                BPAssert(false, "Unexpected component");
                return;
        }

        auto testCaseName = testCasePrefix + components._to_string();
        testCaseName += "}";
        TEST_CASE(testCaseName.c_str());

        Texture2D tex(glm::uvec2{ 1, 1 }, texFormat);
        tex.SetData(&testVal, components);

        T outputTestVal;
        tex.GetData(&outputTestVal, components);

        std::string errMsg = "Expected ";
        errMsg += std::to_string(testVal);
        errMsg += " but got ";
        errMsg += std::to_string(outputTestVal);
        TEST_CHECK_(outputTestVal == testVal, errMsg.c_str());
    }
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
            TestTextureGetSetSingle(
                SimpleFormat{FormatTypes::NormalizedUInt,
                             FormatComponents::R,
                             BitDepths::B8},
                (glm::u8)203);
            TestTextureGetSetSingle(
                SimpleFormat{FormatTypes::NormalizedUInt,
                                     FormatComponents::RG,
                                     BitDepths::B8},
                (glm::u8)203);
            TestTextureGetSetSingle(
                SimpleFormat{FormatTypes::NormalizedUInt,
                             FormatComponents::RG,
                             BitDepths::B8},
                (glm::u8)203);
            TestTextureGetSetSingle(
                SimpleFormat{FormatTypes::NormalizedUInt,
                             FormatComponents::RGBA,
                             BitDepths::B8},
                (glm::u8)203);
            TestTextureGetSetSingle(
                SimpleFormat{FormatTypes::NormalizedUInt,
                             FormatComponents::RGB,
                             BitDepths::B5},
                (glm::u8)16);
            TestTextureGetSetSingle(
                SimpleFormat{FormatTypes::NormalizedInt,
                             FormatComponents::RG,
                             BitDepths::B8},
                (glm::i8)67);
            TestTextureGetSetSingle(
                SimpleFormat{FormatTypes::NormalizedInt,
                             FormatComponents::RG,
                             BitDepths::B8},
                (glm::i8)(-67));
            TestTextureGetSetSingle(
                SimpleFormat{FormatTypes::Float,
                             FormatComponents::RGB,
                             BitDepths::B32},
                (glm::f32)123.456f);

            /*TestTextureGetSetExact(
                { SpecialFormats::RGB10_A2 },
                +ComponentData::Green,
                (glm::u8)32);*/

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
//TODO: cubemap tests