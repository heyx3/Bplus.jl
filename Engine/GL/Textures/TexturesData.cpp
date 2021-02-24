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
            std::string errMsg = "Unknown :  Bplus::GL::Textures::PixelIOChannels.";
            errMsg += data._to_string();
            BP_ASSERT(false, errMsg.c_str());
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
            std::string errMsg = "Unknown :  Bplus::GL::Textures::PixelIOChannels.";
            errMsg += components._to_string();
            BP_ASSERT(false, errMsg.c_str());
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
            std::string errMsg = "Unknown :  Bplus::GL::Textures::PixelIOChannels.";
            errMsg += components._to_string();
            BP_ASSERT(false, errMsg.c_str());
            return 0;
    }
}