#include "Data.h"

using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Textures;

uint8_t Bplus::GL::Textures::GetNChannels(ComponentData data)
{
    switch (data)
    {
        case ComponentData::Red:
        case ComponentData::Green:
        case ComponentData::Blue:
            return 1;

        case ComponentData::RG:
            return 2;

        case ComponentData::RGB:
        case ComponentData::BGR:
            return 3;

        case ComponentData::RGBA:
        case ComponentData::BGRA:
            return 4;

        default:
            std::string errMsg = "Unknown :  Bplus::GL::Textures::ComponentData.";
            errMsg += data._to_string();
            BPAssert(false, errMsg.c_str());
            return 0;
    }
}
bool Bplus::GL::Textures::UsesChannel(ComponentData components, ColorChannels channel)
{
    switch (components)
    {
        case ComponentData::Red: return channel == +ColorChannels::Red;
        case ComponentData::Green: return channel == +ColorChannels::Green;
        case ComponentData::Blue: return channel == +ColorChannels::Blue;

        case ComponentData::RG: return channel == +ColorChannels::Red ||
                                       channel == +ColorChannels::Green;

        case ComponentData::RGB:
        case ComponentData::BGR:
            return channel != +ColorChannels::Alpha;

        case ComponentData::RGBA:
        case ComponentData::BGRA:
            return true;

        default:
            std::string errMsg = "Unknown :  Bplus::GL::Textures::ComponentData.";
            errMsg += components._to_string();
            BPAssert(false, errMsg.c_str());
            return 0;
    }
}