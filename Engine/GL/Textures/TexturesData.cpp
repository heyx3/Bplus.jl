#include "TexturesData.h"

using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Textures;

uint8_t Bplus::GL::Textures::GetNChannels(PixelIOChannels data)
{
    switch (data)
    {
        case PixelIOChannels::Red:
        case PixelIOChannels::Green:
        case PixelIOChannels::Blue:
            return 1;

        case PixelIOChannels::RG:
            return 2;

        case PixelIOChannels::RGB:
        case PixelIOChannels::BGR:
            return 3;

        case PixelIOChannels::RGBA:
        case PixelIOChannels::BGRA:
            return 4;

        default:
            BP_ASSERT_STR(false,
                          "Unknown Bplus::GL::Textures::PixelIOChannels : " + data._to_string());
            return 0;
    }
}
bool Bplus::GL::Textures::UsesChannel(PixelIOChannels components, ColorChannels channel)
{
    switch (components)
    {
        case PixelIOChannels::Red: return channel == +ColorChannels::Red;
        case PixelIOChannels::Green: return channel == +ColorChannels::Green;
        case PixelIOChannels::Blue: return channel == +ColorChannels::Blue;

        case PixelIOChannels::RG: return channel == +ColorChannels::Red ||
                                       channel == +ColorChannels::Green;

        case PixelIOChannels::RGB:
        case PixelIOChannels::BGR:
            return channel != +ColorChannels::Alpha;

        case PixelIOChannels::RGBA:
        case PixelIOChannels::BGRA:
            return true;

        default:
            BP_ASSERT_STR(false,
                          "Unknown Bplus::GL::Textures::PixelIOChannels : " + components._to_string());
            return 0;
    }
}
uint8_t Bplus::GL::Textures::GetChannelIndex(PixelIOChannels components,
                                             ColorChannels channel)
{
    BP_ASSERT(UsesChannel(components, channel),
             "Component format doesn't use the channel");

    switch (components)
    {
        //If there's only one component, the index is always zero.
        case PixelIOChannels::Red:
        case PixelIOChannels::Green:
        case PixelIOChannels::Blue:
            return 0;

        case PixelIOChannels::RG:
            return (channel == +ColorChannels::Red) ? 0 : 1;

        case PixelIOChannels::RGB:
            return (channel == +ColorChannels::Red) ? 0 :
                   ((channel == +ColorChannels::Green) ? 1 : 2);
        case PixelIOChannels::BGR:
            return (channel == +ColorChannels::Blue) ? 0 :
                   ((channel == +ColorChannels::Green) ? 1 : 2);

        case PixelIOChannels::RGBA:
            return (channel == +ColorChannels::Red) ? 0 :
                   ((channel == +ColorChannels::Green) ? 1 :
                   ((channel == +ColorChannels::Blue) ? 2 : 3));
        case PixelIOChannels::BGRA:
            return (channel == +ColorChannels::Blue) ? 0 :
                   ((channel == +ColorChannels::Green) ? 1 :
                   ((channel == +ColorChannels::Red) ? 2 : 3));

        default:
            BP_ASSERT_STR(false,
                "Unknown Bplus::GL::Textures::PixelIOChannels : " + components._to_string());
            return 0;
    }
}

GLenum Bplus::GL::Textures::GetIntegerVersion(PixelIOChannels components)
{
    switch (components)
    {
        case PixelIOChannels::Red: return GL_RED_INTEGER;
        case PixelIOChannels::Green: return GL_GREEN_INTEGER;
        case PixelIOChannels::Blue: return GL_BLUE_INTEGER;

        case PixelIOChannels::RG: return GL_RG_INTEGER;
        case PixelIOChannels::RGB: return GL_RGB_INTEGER;
        case PixelIOChannels::BGR: return GL_BGR_INTEGER;
        case PixelIOChannels::RGBA: return GL_RGBA_INTEGER;
        case PixelIOChannels::BGRA: return GL_BGRA_INTEGER;

        default:
            BP_ASSERT_STR(false,
                "Unknown Bplus::GL::Textures::PixelIOChannels : " + components._to_string());
            return 0;
    }
}
