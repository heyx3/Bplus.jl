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
            BPAssert(false, errMsg.c_str());
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
            BPAssert(false, errMsg.c_str());
            return 0;
    }
}