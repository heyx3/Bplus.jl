#pragma once

#include <random>

#define TEST_NO_MAIN
#include <acutest.h>

#include <B+/Renderer/Textures/TextureD.hpp>
#include <B+/Renderer/Textures/TextureCube.h>
#include "../SimpleApp.hpp"

using namespace Bplus::GL::Textures;


#pragma region TextureCreation() test

template<typename Texture_t, typename Size_t>
void RunTextureTypeCreationTest(std::string testName,
                                Size_t size, Format format,
                                uint_mipLevel_t nMips)
{
    TEST_CASE(testName.c_str());

    Texture_t tex1{ size, format, nMips };
    Texture_t tex2{ std::move(tex1) };
}

void RunTextureCreationTest(const char* testName,
                            glm::uvec3 fullSize, Format format,
                            uint_mipLevel_t nMips = 0)
{
    RunTextureTypeCreationTest<Texture1D, glm::uvec1>(std::string{ testName } + " (Texture1D)",
                                                      glm::uvec1{ fullSize.x }, format, nMips);
    RunTextureTypeCreationTest<Texture2D, glm::uvec2>(std::string{ testName } + " (Texture2D)",
                                                      glm::uvec2{ fullSize.x, fullSize.y }, format, nMips);
    RunTextureTypeCreationTest<Texture3D, glm::uvec3>(std::string{ testName } + " (Texture3D)",
                                                      glm::uvec3{ fullSize.x, fullSize.y, fullSize.z },
                                                      format, nMips);
    RunTextureTypeCreationTest<TextureCube, glm::uvec2>(std::string{ testName } +" (TextureCube)",
                                                        glm::uvec2{ fullSize.x, fullSize.y },
                                                        format, nMips);
}

void TextureCreation()
{
    Simple::RunTest([&]()
    {
        RunTextureCreationTest("Simple RGBA 8", { 1, 1, 1 },
                               SimpleFormat{ +FormatTypes::NormalizedUInt,
                                             +FormatComponents::RGBA,
                                             +BitDepths::B8 });
        RunTextureCreationTest("Simple RG F32", { 2, 2, 2 },
                               SimpleFormat{ +FormatTypes::Float,
                                             +FormatComponents::RG,
                                             +BitDepths::B32 });
        RunTextureCreationTest("Simple R I16", { 3, 7, 13 },
                               SimpleFormat{ +FormatTypes::Int,
                                             +FormatComponents::R,
                                             +BitDepths::B16 });
        RunTextureCreationTest("Special: RGB10 A2 UInt", { 31, 33, 29 },
                               +SpecialFormats::RGB10_A2_UInt);

        RunTextureCreationTest("Special: RGB9 e5", { 41, 39, 101 },
                               +SpecialFormats::RGB_SharedExpFloats);
        RunTextureCreationTest("Special: sRGB_LinA", { 41, 39, 101 },
                               +SpecialFormats::sRGB_LinearAlpha);

        RunTextureCreationTest("Compressed: Greyscale signed", { 31, 32, 7 },
                               +CompressedFormats::Greyscale_NormalizedInt);
        RunTextureCreationTest("Compressed: RG unsigned", { 1, 3, 5 },
                               +CompressedFormats::RG_NormalizedUInt);
        RunTextureCreationTest("Compressed: RGB unsigned float", { 5, 4, 3 },
                               +CompressedFormats::RGB_UFloat);
        RunTextureCreationTest("Compressed: RGBA sRGB", { 32, 129, 256 },
                               +CompressedFormats::RGBA_sRGB_NormalizedUInt);

        RunTextureCreationTest("Depth: 24U", { 1920, 1080, 1 },
                               +DepthStencilFormats::Depth_24U);
        RunTextureCreationTest("Stencil: 8U", { 1920, 1080, 1 },
                               +DepthStencilFormats::Stencil_8);
        RunTextureCreationTest("Depth/Stencil: 32F, 8U", { 1921, 1079, 1 },
                               +DepthStencilFormats::Depth32F_Stencil8);
    });
}

#pragma endregion

#pragma region TextureSimpleGetSetData() test

//Runs a test for getting/setting texture data with some kind of precise format.
template<typename T>
void RunTextureSimpleGetSetTest(SimpleFormat texFormat, glm::vec<4, T> testValues)
{
    std::string testCasePrefix = "{";
    testCasePrefix += ToString(texFormat);
    testCasePrefix += ": ";

    //Run tests for getting and setting different channels in the texture, individually.
    for (uint8_t i = 0; i < std::min((uint8_t)3, texFormat.Components._to_integral()); ++i)
    {
        //Pick some components to try.
        //TODO: Pick more subsets like RG, RGBA, etc.
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

        //Log the test case's name.
        auto testCaseName = testCasePrefix + components._to_string();
        testCaseName += "}";
        TEST_CASE(testCaseName.c_str());

        //Set the texture's color in the above components.
        Texture2D tex(glm::uvec2{ 1, 1 }, texFormat);
        tex.Set_Color(&testValues[0], components); //TODO: Does this overwrite the other components to 0 and/or 1, or does it leave them alone? We need to specify.
        //Get the texture's color back into a separate variable.
        glm::vec<4, T> outputTestVal;
        tex.Get_Color(&outputTestVal[0], components);

        //Test that the two colors are equal in the components that were set.
        for (uint8_t channelI = 0; channelI < 4; ++i)
            if (UsesChannel(components, Bplus::GL::Textures::ColorChannels::_from_index(channelI)))
            {
                std::string errMsg = "Expected ";
                errMsg += std::to_string(testValues[channelI]);
                errMsg += " but got ";
                errMsg += std::to_string(outputTestVal[channelI]);
                TEST_CHECK_(outputTestVal == testValues, errMsg.c_str());
            }
    }
}

template<typename T, glm::size_t L>
void TestTextureGetSetSingle(Format texFormat, ComponentData dataComponentFormat,
                             std::array<T, L> testDataComponents)
{
    std::string testCaseName = "{";
    testCaseName += ToString(texFormat);
    testCaseName += ": ";
    testCaseName += dataComponentFormat._to_string();
    testCaseName += "}";
    TEST_CASE(testCaseName.c_str());

    Texture2D tex(glm::uvec2{ 1, 1 }, texFormat);
    tex.Set_Color(testDataComponents.data(), dataComponentFormat);
    
    std::array<T, L> outputTestVal;
    tex.Get_Color(outputTestVal.data(), dataComponentFormat);

    for (size_t i = 0; i < outputTestVal.size(); ++i)
        TEST_CHECK(outputTestVal[i] == testDataComponents[i]);
}
template<typename T, glm::size_t L>
void TestTextureGetSetSingleAllChannels(SimpleFormat texFormat,
                                        std::array<T, L> testData)
{
    ComponentData dataComponentFormat;
    switch (texFormat.Components)
    {
        case FormatComponents::R:    dataComponentFormat = ComponentData::Red ; break;
        case FormatComponents::RG:   dataComponentFormat = ComponentData::RG  ; break;
        case FormatComponents::RGB:  dataComponentFormat = ComponentData::RGB ; break;
        case FormatComponents::RGBA: dataComponentFormat = ComponentData::RGBA; break;
        default: BPAssert(false, "Unexpected Bplus::GL::Textures::FormatComponents"); break;
    }
    
    glm::bvec4 usedChannels = { UsesChannel(dataComponentFormat, ColorChannels::Red),
                                UsesChannel(dataComponentFormat, ColorChannels::Green),
                                UsesChannel(dataComponentFormat, ColorChannels::Blue),
                                UsesChannel(dataComponentFormat, ColorChannels::Alpha) };
    if (usedChannels.r)
    {
        TestTextureGetSetSingle(texFormat, ComponentData::Red, testData);
        if (usedChannels.g)
        {
            TestTextureGetSetSingle(texFormat, ComponentData::RG, testData);
            if (usedChannels.b)
            {
                TestTextureGetSetSingle(texFormat, ComponentData::RGB, testData);
                if (usedChannels.a)
                    TestTextureGetSetSingle(texFormat, ComponentData::RGBA, testData);
            }
        }
    }
    if (usedChannels.g)
        TestTextureGetSetSingle(texFormat, ComponentData::Green, testData);
    if (usedChannels.b)
        TestTextureGetSetSingle(texFormat, ComponentData::Blue, testData);
}
void TextureSimpleGetSetData()
{
    Simple::RunTest([&]()
    {
        TestTextureGetSetSingleAllChannels(
            SimpleFormat{ FormatTypes::NormalizedUInt,
                          FormatComponents::R,
                          BitDepths::B8 },
            std::array{ (glm::u8)203 });
        TestTextureGetSetSingleAllChannels(
            { FormatTypes::NormalizedUInt,
              FormatComponents::RG,
              BitDepths::B8 },
            std::array{ (glm::u8)203, 204 });
        TestTextureGetSetSingleAllChannels(
            { FormatTypes::NormalizedUInt,
              FormatComponents::RGBA,
              BitDepths::B8 },
            std::array{ (glm::u8)1, 128, 35, 206 });
        TestTextureGetSetSingleAllChannels(
            { FormatTypes::NormalizedUInt,
              FormatComponents::RGB,
              BitDepths::B5 },
            std::array{ (glm::u8)16, 0, 3 });
        TestTextureGetSetSingleAllChannels(
            { FormatTypes::NormalizedUInt,
              FormatComponents::RGB,
              BitDepths::B10 },
            std::array{ (glm::uint16_t)1023, 513, 0 });

        TestTextureGetSetSingleAllChannels(
            { FormatTypes::NormalizedInt,
              FormatComponents::RG,
              BitDepths::B8 },
            std::array{ (glm::u8)67, 127 });
        TestTextureGetSetSingleAllChannels(
            { FormatTypes::NormalizedInt,
              FormatComponents::RG,
              BitDepths::B8 },
            std::array{ (glm::i8)-67, -127 });

        TestTextureGetSetSingleAllChannels(
            { FormatTypes::Float,
              FormatComponents::RGB,
              BitDepths::B32 },
            std::array{ (glm::f32)123.456f, -123.456f, 0 });
        TestTextureGetSetSingleAllChannels(
            { FormatTypes::Float,
              FormatComponents::RGBA,
              BitDepths::B16 },
            std::array{ (glm::f32)123, -123, 0, 1.5f });
        
        TestTextureGetSetSingleAllChannels(
            { FormatTypes::UInt,
              FormatComponents::RGB,
              BitDepths::B16 },
            std::array{ (glm::uint16_t)64001, 0, 20000 });
        TestTextureGetSetSingleAllChannels(
            { FormatTypes::UInt,
              FormatComponents::RGB,
              BitDepths::B32 },
            std::array{ (glm::uint32_t)2647324001, 0, 567890123 });
        TestTextureGetSetSingleAllChannels(
            { FormatTypes::UInt,
              FormatComponents::R,
              BitDepths::B32 },
            std::array{ (glm::uint32_t)2097152 });

        TestTextureGetSetSingleAllChannels(
            { FormatTypes::Int,
              FormatComponents::RGB,
              BitDepths::B16 },
            std::array{ (glm::int16_t)14503, -999, -20000 });

        //TODO: Special formats
        //TODO: Compressed formats
        //TODO: Depth get/set
    });
}

#pragma endregion

//TODO: TextureSubRectData()
//TODO: cubemap tests