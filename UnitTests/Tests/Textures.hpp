#pragma once

#include <random>

#define TEST_NO_MAIN
#include <acutest.h>

#include <B+/GL/Textures/TextureD.hpp>
#include <B+/GL/Textures/TextureCube.h>
#include "../SimpleApp.hpp"

using namespace Bplus::GL::Textures;


#pragma region TextureCreation() test

template<typename Texture_t, typename Size_t>
void RunTextureTypeCreationTest(std::string testName,
                                Size_t size, Format format,
                                uint_mipLevel_t nMips)
{
    auto swizzle = std::array<SwizzleSources, 4>{ SwizzleSources::Red, SwizzleSources::Green,
                                                  SwizzleSources::Blue, SwizzleSources::Alpha };

    std::optional<DepthStencilSources> depthStencil = std::nullopt;
    if (format.IsDepthAndStencil())
        depthStencil = DepthStencilSources::Depth;

    std::string test1Name = testName + " (normal constructor)";
    TEST_CASE(test1Name.c_str());
    //Note that Depth/Stencil formats aren't supported for 3D textures.
    if (std::is_same_v<Texture_t, Bplus::GL::Textures::Texture3D> &&
        format.IsDepthStencil())
    {
        TEST_EXCEPTION_(Texture_t tex1(size BP_COMMA format BP_COMMA nMips),
                        std::exception,
                        "Expected an error when creating a 3D depth/stencil texture");
    }
    else
    {
        Texture_t tex1(size, format, nMips, Texture_t::Sampler_t(), swizzle, depthStencil);
        std::string test2Name = testName + " (move constructor)";
        TEST_CASE(test2Name.c_str());
        Texture_t tex2(std::move(tex1));
    }
}


enum class TextureTypesFlags : uint8_t {
    OneD = 1, TwoD = 2, ThreeD = 4, Cube = 8,
    AllTwoD = TwoD | Cube,
    All = OneD | TwoD | ThreeD | Cube
};
#define TEX_TYPE_FLAGS_HAS(value, test) (((uint8_t)(value) & (uint8_t)(test)) != 0)

void RunTextureCreationTests(const char* testName,
                             glm::uvec3 fullSize, Format format,
                             TextureTypesFlags types = TextureTypesFlags::All,
                             uint_mipLevel_t nMips = 0)
{
    if (TEX_TYPE_FLAGS_HAS(types, TextureTypesFlags::OneD))
        RunTextureTypeCreationTest<Texture1D, glm::uvec1>(std::string{ testName } + " (Texture1D)",
                                                          glm::uvec1{ fullSize.x }, format, nMips);
    if (TEX_TYPE_FLAGS_HAS(types, TextureTypesFlags::TwoD))
        RunTextureTypeCreationTest<Texture2D, glm::uvec2>(std::string{ testName } + " (Texture2D)",
                                                          glm::uvec2{ fullSize.x, fullSize.y }, format, nMips);
    if (TEX_TYPE_FLAGS_HAS(types, TextureTypesFlags::ThreeD))
        RunTextureTypeCreationTest<Texture3D, glm::uvec3>(std::string{ testName } + " (Texture3D)",
                                                          glm::uvec3{ fullSize.x, fullSize.y, fullSize.z },
                                                          format, nMips);
    if (TEX_TYPE_FLAGS_HAS(types, TextureTypesFlags::Cube))
        RunTextureTypeCreationTest<TextureCube, glm::u32>(std::string{ testName } +" (TextureCube)",
                                                          fullSize.x, format, nMips);
}

void TextureCreation()
{
    Simple::RunTest([&]()
    {
        RunTextureCreationTests("Simple RGBA 8", { 1, 1, 1 },
                                SimpleFormat{ +FormatTypes::NormalizedUInt,
                                              +SimpleFormatComponents::RGBA,
                                              +SimpleFormatBitDepths::B8 });
        RunTextureCreationTests("Simple RG F32", { 2, 2, 2 },
                                SimpleFormat{ +FormatTypes::Float,
                                              +SimpleFormatComponents::RG,
                                              +SimpleFormatBitDepths::B32 });
        RunTextureCreationTests("Simple R I16", { 3, 7, 13 },
                                SimpleFormat{ FormatTypes::Int,
                                              SimpleFormatComponents::R,
                                              SimpleFormatBitDepths::B16 });
        RunTextureCreationTests("Special: RGB10 A2 UInt", { 31, 33, 29 },
                                +SpecialFormats::RGB10_A2_UInt);

        RunTextureCreationTests("Special: RGB9 e5", { 41, 39, 101 },
                                +SpecialFormats::RGB_SharedExpFloats);
        RunTextureCreationTests("Special: sRGB_LinA", { 41, 39, 101 },
                                +SpecialFormats::sRGB_LinearAlpha); 

        RunTextureCreationTests("Compressed: Greyscale signed", { 16, 16, 16 },
                                +CompressedFormats::Greyscale_NormalizedInt,
                                TextureTypesFlags::AllTwoD);
        RunTextureCreationTests("Compressed: RG unsigned", { 16, 16, 16 },
                                +CompressedFormats::RG_NormalizedUInt,
                                TextureTypesFlags::AllTwoD);
        RunTextureCreationTests("Compressed: RGB unsigned float", { 16, 16, 16 },
                                +CompressedFormats::RGB_UFloat,
                                TextureTypesFlags::AllTwoD);
        RunTextureCreationTests("Compressed: RGBA sRGB", { 16, 16, 16 },
                                +CompressedFormats::RGBA_sRGB_NormalizedUInt,
                                TextureTypesFlags::AllTwoD);

        RunTextureCreationTests("Depth: 24U", { 1920, 1080, 1 },
                                +DepthStencilFormats::Depth_24U);
        RunTextureCreationTests("Stencil: 8U", { 1920, 1080, 1 },
                                +DepthStencilFormats::Stencil_8,
                                TextureTypesFlags::AllTwoD);
        RunTextureCreationTests("Depth/Stencil: 32F, 8U", { 1921, 1079, 1 },
                                +DepthStencilFormats::Depth32F_Stencil8,
                                TextureTypesFlags::AllTwoD);
    });
}

#pragma endregion

#pragma region TextureSimpleGetSetData() test

//TODO: Figure out.
template<typename T>
void TestTextureGetSetColors(Format texFormat, PixelIOChannels components,
                             glm::uvec3 size, const T* data)
{
    BPAssert(!texFormat.IsDepthStencil(),
             "TextTextureGetSetColor() isn't for depth/stencil textures");

    //Test a Texture2D with the given format, size, and data.
    Texture2D tex(glm::uvec2{ size.x, size.y * size.z }, texFormat);
    tex.Set_Color<T>(data, components);
    for (glm::u32 y = 0; y < tex.GetSize().y; ++y)
        for (glm::u32 x = 0; x < tex.GetSize().x; ++x)
        {
            //Get the data for this pixel.
            uint8_t nComponents = texFormat.GetNChannels();
            const T* startValue = &data[x + (tex.GetSize().x * y)];

            //Generate a test case name.
            std::string testCaseName = "{";
            testCaseName += ToString(texFormat) + " (" +
                                std::to_string(tex.GetSize().x) +
                                "x" + std::to_string(tex.GetSize().y) +
                                ") at " + std::to_string(x) + "," + std::to_string(y) +
                                ": ";
            for (uint8_t componentI = 0; componentI < nComponents; componentI++)
            {
                if (componentI > 0)
                    testCaseName += ",";
                testCaseName += std::to_string(startValue[componentI]);
            }

            //Read data from the texture, then test it against the expected data.
            std::array<T, 4> actualData;
            auto readRegion = Bplus::Math::Box2Du::MakeMinSize(glm::uvec2{ x, y },
                                                               glm::uvec2{ 1, 1 });
            tex.Get_Color<T>(&actualData[0], components, GetData2DParams(readRegion));
            
        }
}

template<typename T, glm::size_t L>
void TestTextureGetSetSingle(Format texFormat, PixelIOChannels dataComponentFormat,
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
    
    //Swap framebuffers so that graphics debuggers can take a snapshot.
    SDL_GL_SwapWindow(Simple::App->MainWindow);

    std::array<T, L> outputTestVal;
    tex.Get_Color(outputTestVal.data(), dataComponentFormat);

    //Swap framebuffers so that graphics debuggers can take a snapshot.
    SDL_GL_SwapWindow(Simple::App->MainWindow);

    //Test each channel that was actually set.
    auto TestChannel = [&](ColorChannels channel, const char* channelName) {
        if (UsesChannel(dataComponentFormat, channel))
        {
            auto channelI = GetChannelIndex(dataComponentFormat, channel);
            TEST_CHECK_(outputTestVal[channelI] == testDataComponents[channelI],
                        channelName);
        }
    };
    TestChannel(ColorChannels::Red, "Red");
    TestChannel(ColorChannels::Green, "Red");
    TestChannel(ColorChannels::Blue, "Blue");
    TestChannel(ColorChannels::Alpha, "Alpha");
}
template<typename T, glm::size_t L>
void TestTextureGetSetSingleAllChannels(SimpleFormat texFormat,
                                        std::array<T, L> testData)
{
    PixelIOChannels dataComponentFormat;
    switch (texFormat.Components)
    {
        case SimpleFormatComponents::R:    dataComponentFormat = PixelIOChannels::Red ; break;
        case SimpleFormatComponents::RG:   dataComponentFormat = PixelIOChannels::RG  ; break;
        case SimpleFormatComponents::RGB:  dataComponentFormat = PixelIOChannels::RGB ; break;
        case SimpleFormatComponents::RGBA: dataComponentFormat = PixelIOChannels::RGBA; break;
        default: BPAssert(false, "Unexpected Bplus::GL::Textures::SimpleFormatComponents"); break;
    }
    
    glm::bvec4 usedChannels = { UsesChannel(dataComponentFormat, ColorChannels::Red),
                                UsesChannel(dataComponentFormat, ColorChannels::Green),
                                UsesChannel(dataComponentFormat, ColorChannels::Blue),
                                UsesChannel(dataComponentFormat, ColorChannels::Alpha) };
    if (usedChannels.r)
    {
        TestTextureGetSetSingle(texFormat, PixelIOChannels::Red, testData);
        if (usedChannels.g)
        {
            TestTextureGetSetSingle(texFormat, PixelIOChannels::RG, testData);
            if (usedChannels.b)
            {
                TestTextureGetSetSingle(texFormat, PixelIOChannels::RGB, testData);
                if (usedChannels.a)
                    TestTextureGetSetSingle(texFormat, PixelIOChannels::RGBA, testData);
            }
        }
    }
    if (usedChannels.g)
        TestTextureGetSetSingle(texFormat, PixelIOChannels::Green, testData);
    if (usedChannels.b)
        TestTextureGetSetSingle(texFormat, PixelIOChannels::Blue, testData);
}
void TextureSimpleGetSetData()
{
    Simple::RunTest([&]()
    {
        TestTextureGetSetSingleAllChannels(
            { FormatTypes::NormalizedUInt,
              SimpleFormatComponents::R,
              SimpleFormatBitDepths::B8 },
            std::array{ (glm::u8)203 });
        TestTextureGetSetSingleAllChannels(
            { FormatTypes::NormalizedUInt,
              SimpleFormatComponents::RG,
              SimpleFormatBitDepths::B8 },
            std::array<glm::u8, 2>{ 203, 204 });
        TestTextureGetSetSingleAllChannels(
            { FormatTypes::NormalizedUInt,
              SimpleFormatComponents::RGBA,
              SimpleFormatBitDepths::B8 },
            std::array<glm::u8, 4>{ 1, 128, 35, 206 });

        TestTextureGetSetSingleAllChannels(
            { FormatTypes::NormalizedInt,
              SimpleFormatComponents::RG,
              SimpleFormatBitDepths::B8 },
            std::array<glm::i8, 2>{ 67, 127 });
        TestTextureGetSetSingleAllChannels(
            { FormatTypes::NormalizedInt,
              SimpleFormatComponents::RG,
              SimpleFormatBitDepths::B8 },
            std::array<glm::i8, 2>{ -67, -127 });

        TestTextureGetSetSingleAllChannels(
            { FormatTypes::Float,
              SimpleFormatComponents::RGB,
              SimpleFormatBitDepths::B32 },
            std::array<glm::f32, 3>{ 123.456f, -123.456f, 0 });
        TestTextureGetSetSingleAllChannels(
            { FormatTypes::Float,
              SimpleFormatComponents::RGBA,
              SimpleFormatBitDepths::B16 },
            std::array<glm::f32, 4>{ 123, -123, 0, 1.5f });
        
        TestTextureGetSetSingleAllChannels(
            { FormatTypes::UInt,
              SimpleFormatComponents::RGB,
              SimpleFormatBitDepths::B16 },
            std::array<glm::u16, 3>{ 64001, 0, 20000 });
        TestTextureGetSetSingleAllChannels(
            { FormatTypes::UInt,
              SimpleFormatComponents::RGB,
              SimpleFormatBitDepths::B32 },
            std::array<glm::u32, 3>{ 2647324001, 0, 567890123 });
        TestTextureGetSetSingleAllChannels(
            { FormatTypes::UInt,
              SimpleFormatComponents::R,
              SimpleFormatBitDepths::B32 },
            std::array<glm::u32, 1>{ 2097152 });

        TestTextureGetSetSingleAllChannels(
            { FormatTypes::Int,
              SimpleFormatComponents::RGB,
              SimpleFormatBitDepths::B16 },
            std::array<glm::i16, 3>{ 14503, -999, -20000 });

        //TODO: Special formats
        //TODO: Compressed formats
        //TODO: Depth get/set
    });
}

#pragma endregion

//TODO: TextureSubRectData()
//TODO: cubemap tests