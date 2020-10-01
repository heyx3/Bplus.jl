#include "Format.h"

#include <glm/gtx/component_wise.hpp>
#include "../../Math/Math.h"

using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Textures;


//This file has a lot of complex switch statements.
//This macro helps a lot with ensuring we don't silently fall though any missed cases.
#define SWITCH_DEFAULT(enumName, enumVal) \
    default: { \
        std::string errMsg = "Unknown: " #enumName "::"; \
        errMsg += enumVal._to_string(); \
        BPAssert(false, errMsg.c_str()); \
    }

namespace
{
    bool _StoresChannel(SimpleFormatComponents components, AllChannels channel)
    {
        switch (components)
        {
            case SimpleFormatComponents::R:
                return channel == +AllChannels::Red;
            case SimpleFormatComponents::RG:
                return (channel == +AllChannels::Red) |
                       (channel == +AllChannels::Green);
            case SimpleFormatComponents::RGB:
                return (channel == +AllChannels::Red) |
                       (channel == +AllChannels::Green) |
                       (channel == +AllChannels::Blue);
            case SimpleFormatComponents::RGBA:
                return (channel == +AllChannels::Red) |
                       (channel == +AllChannels::Green) |
                       (channel == +AllChannels::Blue) |
                       (channel == +AllChannels::Alpha);

            SWITCH_DEFAULT(SimpleFormatComponents, components)
                return false;
        }
    }
}


uint_fast32_t Bplus::GL::Textures::GetBlockSize(CompressedFormats format)
{
    switch (format)
    {
        case CompressedFormats::Greyscale_NormalizedUInt:
        case CompressedFormats::Greyscale_NormalizedInt:
        case CompressedFormats::RG_NormalizedUInt:
        case CompressedFormats::RG_NormalizedInt:
        case CompressedFormats::RGB_Float:
        case CompressedFormats::RGB_UFloat:
        case CompressedFormats::RGBA_NormalizedUInt:
        case CompressedFormats::RGBA_sRGB_NormalizedUInt:
            return 4;

        SWITCH_DEFAULT(CompressedFormats, format)
            return 0;
    }
}

std::string Bplus::GL::Textures::ToString(const SimpleFormat& format)
{
    std::string str;

    str += format.Components._to_string();
    str += "_";

    std::string bitStr = format.ChannelBitSize._to_string();
    bitStr = bitStr.substr(1); //Remove the 'B' from in front.
    str += bitStr;

    switch (format.Type)
    {
        case FormatTypes::Float:          str += "F"; break;
        case FormatTypes::NormalizedUInt: str += "Un"; break;
        case FormatTypes::NormalizedInt:  str += "In"; break;
        case FormatTypes::UInt: str += "UInteger"; break;
        case FormatTypes::Int: str += "SInteger"; break;

        SWITCH_DEFAULT(FormatTypes, format.Type);
            break;
    }

    return str;
}
std::string Bplus::GL::Textures::ToString(const Format& format)
{
    if (format.IsSimple())
        return ToString(format.AsSimple());
    else if (format.IsSpecial())
        return format.AsSpecial()._to_string();
    else if (format.IsCompressed())
        return format.AsCompressed()._to_string();
    else if (format.IsDepthStencil())
        return format.AsDepthStencil()._to_string();
    else
    {
        BPAssert(false, "Unexpected format type");
        return "";
    }
}

bool Bplus::GL::Textures::IsDepthOnly(DepthStencilFormats format)
{
    switch (format)
    {
        case DepthStencilFormats::Depth_16U:
        case DepthStencilFormats::Depth_24U:
        case DepthStencilFormats::Depth_32U:
        case DepthStencilFormats::Depth_32F:
            return true;

        case DepthStencilFormats::Depth24U_Stencil8:
        case DepthStencilFormats::Depth32F_Stencil8:
        case DepthStencilFormats::Stencil_8:
            return false;
            
        SWITCH_DEFAULT(DepthStencilFormats, format)
            return false;
    }
}
bool Bplus::GL::Textures::IsStencilOnly(DepthStencilFormats format)
{
    switch (format)
    {
        case DepthStencilFormats::Stencil_8:
            return true;

        case DepthStencilFormats::Depth_16U:
        case DepthStencilFormats::Depth_24U:
        case DepthStencilFormats::Depth_32U:
        case DepthStencilFormats::Depth_32F:
        case DepthStencilFormats::Depth24U_Stencil8:
        case DepthStencilFormats::Depth32F_Stencil8:
            return false;
            
        SWITCH_DEFAULT(DepthStencilFormats, format)
            return false;
    }
}
bool Bplus::GL::Textures::IsDepthAndStencil(DepthStencilFormats format)
{
    switch (format)
    {
        case DepthStencilFormats::Depth24U_Stencil8:
        case DepthStencilFormats::Depth32F_Stencil8:
            return true;

        case DepthStencilFormats::Depth_16U:
        case DepthStencilFormats::Depth_24U:
        case DepthStencilFormats::Depth_32U:
        case DepthStencilFormats::Depth_32F:
        case DepthStencilFormats::Stencil_8:
            return false;

        SWITCH_DEFAULT(DepthStencilFormats, format)
            return false;
    }
}


bool Format::IsDepthAndStencil() const
{
    if (!IsDepthStencil())
        return false;
    else
        return Bplus::GL::Textures::IsDepthAndStencil(AsDepthStencil());
}
bool Format::IsDepthOnly() const
{
    if (!IsDepthStencil())
        return false;
    else
        return Bplus::GL::Textures::IsDepthOnly(AsDepthStencil());
}
bool Format::IsStencilOnly() const
{
    if (!IsDepthStencil())
        return false;
    else
        return Bplus::GL::Textures::IsStencilOnly(AsDepthStencil());
}

std::optional<FormatTypes> Format::GetComponentType() const
{
    if (IsSimple())
        return AsSimple().Type;
    else if (IsSpecial()) switch (AsSpecial())
    {
        case SpecialFormats::R3_G3_B2:
        case SpecialFormats::R5_G6_B5:
        case SpecialFormats::RGB10_A2:
        case SpecialFormats::RGB5_A1:
        case SpecialFormats::sRGB:
        case SpecialFormats::sRGB_LinearAlpha:
            return FormatTypes::NormalizedUInt;

        case SpecialFormats::RGB_TinyFloats:
        case SpecialFormats::RGB_SharedExpFloats:
            return FormatTypes::Float;

        case SpecialFormats::RGB10_A2_UInt:
            return FormatTypes::UInt;

        SWITCH_DEFAULT(SpecialFormats, AsSpecial())
            return std::nullopt;
    }
    else if (IsCompressed()) switch (AsCompressed())
    {
        case CompressedFormats::Greyscale_NormalizedUInt:
        case CompressedFormats::RG_NormalizedUInt:
        case CompressedFormats::RGBA_NormalizedUInt:
        case CompressedFormats::RGBA_sRGB_NormalizedUInt:
            return FormatTypes::NormalizedUInt;

        case CompressedFormats::Greyscale_NormalizedInt:
        case CompressedFormats::RG_NormalizedInt:
            return FormatTypes::NormalizedInt;

        case CompressedFormats::RGB_Float:
        case CompressedFormats::RGB_UFloat: //Pretend this is a float.
            return FormatTypes::Float;

        SWITCH_DEFAULT(CompressedFormats, AsCompressed())
            return std::nullopt;
    }
    else if (IsDepthStencil()) switch (AsDepthStencil())
    {
        case DepthStencilFormats::Depth_16U:
        case DepthStencilFormats::Depth_24U:
        case DepthStencilFormats::Depth_32U:
            return FormatTypes::NormalizedUInt;

        case DepthStencilFormats::Depth_32F:
            return FormatTypes::Float;

        case DepthStencilFormats::Stencil_8:
            return FormatTypes::UInt;

        case DepthStencilFormats::Depth24U_Stencil8:
        case DepthStencilFormats::Depth32F_Stencil8:
            return std::nullopt;

        SWITCH_DEFAULT(DepthStencilFormats, AsDepthStencil())
            return std::nullopt;
    }
    else
    {
        std::string errMsg = "Unknown format type, index ";
        errMsg += std::to_string(data.index());
        BPAssert(false, errMsg.c_str());
        return std::nullopt;
    }
}

bool Format::IsInteger() const
{
    if (IsSimple())
        switch (AsSimple().Type)
        {
            case FormatTypes::NormalizedUInt:
            case FormatTypes::NormalizedInt:
            case FormatTypes::Float:
                return false;

            case FormatTypes::UInt:
            case FormatTypes::Int:
                return true;

            SWITCH_DEFAULT(FormatTypes, AsSimple().Type);
                return false;
        }
    else if (IsSpecial())
        switch (AsSpecial())
        {
            case SpecialFormats::R3_G3_B2:
            case SpecialFormats::R5_G6_B5:
            case SpecialFormats::RGB10_A2:
            case SpecialFormats::RGB5_A1:
            case SpecialFormats::RGB_SharedExpFloats:
            case SpecialFormats::RGB_TinyFloats:
            case SpecialFormats::sRGB:
            case SpecialFormats::sRGB_LinearAlpha:
                return false;

            case SpecialFormats::RGB10_A2_UInt:
                return true;

            SWITCH_DEFAULT(SpecialFormats, AsSpecial());
                return false;
        }
    else if (IsCompressed())
        switch (AsCompressed())
        {
            case CompressedFormats::Greyscale_NormalizedUInt:
            case CompressedFormats::Greyscale_NormalizedInt:
            case CompressedFormats::RG_NormalizedUInt:
            case CompressedFormats::RG_NormalizedInt:
            case CompressedFormats::RGB_Float:
            case CompressedFormats::RGB_UFloat:
            case CompressedFormats::RGBA_NormalizedUInt:
            case CompressedFormats::RGBA_sRGB_NormalizedUInt:
                return false;

            SWITCH_DEFAULT(CompressedFormats, AsCompressed());
                return false;
        }
    else if (IsDepthStencil())
        switch (AsDepthStencil())
        {
            case DepthStencilFormats::Depth_16U:
            case DepthStencilFormats::Depth_24U:
            case DepthStencilFormats::Depth_32U:
            case DepthStencilFormats::Depth_32F:
                return false;
                
            case DepthStencilFormats::Stencil_8:
                return true;

            case DepthStencilFormats::Depth24U_Stencil8:
            case DepthStencilFormats::Depth32F_Stencil8:
                //This is a weird case, but the function header
                //    specifically promises to return "false" here.
                return false;

            SWITCH_DEFAULT(DepthStencilFormats, AsDepthStencil());
                return false;
        }
    else
    {
        std::string errMsg = "Unknown format type, index ";
        errMsg += std::to_string(data.index());
        BPAssert(false, errMsg.c_str());
        return false;
    }
}

bool Format::StoresChannel(AllChannels c) const
{
    if (IsSimple())
    {
        return _StoresChannel(AsSimple().Components, c);
    }
    else if (IsSpecial())
    {
        switch (AsSpecial())
        {
            case SpecialFormats::R3_G3_B2:
            case SpecialFormats::R5_G6_B5:
            case SpecialFormats::RGB_SharedExpFloats:
            case SpecialFormats::RGB_TinyFloats:
            case SpecialFormats::sRGB:
                return _StoresChannel(SimpleFormatComponents::RGB, c);

            case SpecialFormats::RGB10_A2:
            case SpecialFormats::RGB10_A2_UInt:
            case SpecialFormats::RGB5_A1:
            case SpecialFormats::sRGB_LinearAlpha:
                return _StoresChannel(SimpleFormatComponents::RGBA, c);

            SWITCH_DEFAULT(SpecialFormats, AsSpecial())
                return false;
        }
    }
    else if (IsCompressed())
    {
        SimpleFormatComponents components;
        switch (AsCompressed())
        {
            case CompressedFormats::Greyscale_NormalizedUInt:
            case CompressedFormats::Greyscale_NormalizedInt:
                components = SimpleFormatComponents::R;
                break;

            case CompressedFormats::RG_NormalizedUInt:
            case CompressedFormats::RG_NormalizedInt:
                components = SimpleFormatComponents::RG;
                break;

            case CompressedFormats::RGB_Float:
            case CompressedFormats::RGB_UFloat:
                components = SimpleFormatComponents::RGB;
                break;

            case CompressedFormats::RGBA_NormalizedUInt:
            case CompressedFormats::RGBA_sRGB_NormalizedUInt:
                components = SimpleFormatComponents::RGBA;
                break;

            SWITCH_DEFAULT(CompressedFormats, AsCompressed())
                return false;
        }

        return _StoresChannel(components, c);
    }
    else if (IsDepthStencil())
    {
        if (IsDepthOnly() | IsDepthAndStencil())
            return c == +AllChannels::Depth;

        BPAssert(IsDepthOnly() | IsDepthAndStencil(),
                 "Not Depth, Stencil, or hybrid!?");
        return c == +AllChannels::Stencil;
    }
    else
    {
        std::string errMsg = "Unknown format type, index ";
        errMsg += std::to_string(data.index());
        BPAssert(false, errMsg.c_str());
        return false;
    }
}
uint8_t Format::GetNChannels() const
{
    return (StoresChannel(AllChannels::Red) ? 1 : 0) +
           (StoresChannel(AllChannels::Blue) ? 1 : 0) +
           (StoresChannel(AllChannels::Green) ? 1 : 0) +
           (StoresChannel(AllChannels::Alpha) ? 1 : 0) +
           (StoresChannel(AllChannels::Depth) ? 1 : 0) +
           (StoresChannel(AllChannels::Stencil) ? 1 : 0);
}

uint_fast8_t Format::GetChannelBitSize(std::optional<AllChannels> channel) const
{
    #define COMPUTE_SEPARATE_OUTPUT(format, r, g, b, a, d, s) \
        { BPAssert(channel.has_value(), "Channel not given for an uneven format: " format); \
        switch (channel.value()) { \
            case AllChannels::Red: return r; \
            case AllChannels::Green: return g; \
            case AllChannels::Blue: return b; \
            case AllChannels::Alpha: return a; \
            case AllChannels::Depth: return d; \
            case AllChannels::Stencil: return s; \
            SWITCH_DEFAULT(AllChannels, channel.value()) return 0; \
        } }

    if (IsSimple())
    {
        auto simple = AsSimple();
        return (!channel.has_value() ||
                _StoresChannel(simple.Components, channel.value())) ?
                   AsSimple().ChannelBitSize._to_integral() :
                   0;
    }
    else if (IsSpecial()) switch (AsSpecial())
    {
        case SpecialFormats::R3_G3_B2:
            COMPUTE_SEPARATE_OUTPUT("R3_G3_B2", 3, 3, 2, 0, 0, 0)
        case SpecialFormats::R5_G6_B5:
            COMPUTE_SEPARATE_OUTPUT("R5_G6_B5", 5, 6, 5, 0, 0, 0)
        case SpecialFormats::RGB10_A2:
            COMPUTE_SEPARATE_OUTPUT("RGB10_A2", 10, 10, 10, 2, 0, 0)
        case SpecialFormats::RGB10_A2_UInt:
            COMPUTE_SEPARATE_OUTPUT("RGB10_A2_UInt", 10, 10, 10, 2, 0, 0)
        case SpecialFormats::RGB5_A1:
            COMPUTE_SEPARATE_OUTPUT("RGB5_A1", 5, 5, 5, 1, 0, 0)
        case SpecialFormats::RGB_TinyFloats:
            COMPUTE_SEPARATE_OUTPUT("RGB_TinyFloats", 11, 11, 10, 0, 0, 0)

        case SpecialFormats::sRGB:
            if (!channel.has_value() || _StoresChannel(SimpleFormatComponents::RGB, channel.value()))
                return 8;
            else
                return 0;
        case SpecialFormats::sRGB_LinearAlpha:
            if (!channel.has_value() || _StoresChannel(SimpleFormatComponents::RGBA, channel.value()))
                return 8;
            else
                return 0;

        case SpecialFormats::RGB_SharedExpFloats:
            //In this format, each component shares a 5-byte exponent,
            //    so there's no good answer here.
            if (!channel.has_value())
                return 10;
            else
                COMPUTE_SEPARATE_OUTPUT("RGB_SharedExpFloats", 11, 11, 10, 0, 0, 0)


        SWITCH_DEFAULT(SpecialFormats, AsSpecial())
            return 0;
    }
    else if (IsCompressed()) switch (AsCompressed())
    {
        case CompressedFormats::Greyscale_NormalizedUInt:
        case CompressedFormats::Greyscale_NormalizedInt:
            if (!channel.has_value() || _StoresChannel(SimpleFormatComponents::R, channel.value()))
                return 64 / (4 * 4);
            else
                return 0;

        case CompressedFormats::RG_NormalizedUInt:
        case CompressedFormats::RG_NormalizedInt:
            if (!channel.has_value() || _StoresChannel(SimpleFormatComponents::RG, channel.value()))
                return 2 * (64 / (4 * 4));
            else
                return 0;

        case CompressedFormats::RGB_Float:
        case CompressedFormats::RGB_UFloat:
            if (!channel.has_value() || _StoresChannel(SimpleFormatComponents::RGBA, channel.value()))
                return 8;
            else
                return 0;

        case CompressedFormats::RGBA_NormalizedUInt:
        case CompressedFormats::RGBA_sRGB_NormalizedUInt:
            if (!channel.has_value() || _StoresChannel(SimpleFormatComponents::RGBA, channel.value()))
                return 8;
            else
                return 0;

        SWITCH_DEFAULT(CompressedFormats, AsCompressed())
            return 0;
    }
    else if (IsDepthStencil())
    {
        //Exit early if asking for any channel other than depth or stencil.
        if (channel.has_value() &&
            channel.value() != +AllChannels::Depth && channel.value() != +AllChannels::Stencil)
        {
            return 0;
        }

        switch (AsDepthStencil())
        {
            #define DSF_CASE(enumVal, channelVal, bitVal) \
                case DepthStencilFormats::enumVal: \
                    if (channel.value_or(AllChannels::Red) == +AllChannels::channelVal) \
                        return bitVal; \
                    else \
                        return 0

            DSF_CASE(Depth_16U, Depth, 16);
            DSF_CASE(Depth_24U, Depth, 24);
            DSF_CASE(Depth_32U, Depth, 32);
            DSF_CASE(Depth_32F, Depth, 32);
            DSF_CASE(Stencil_8, Stencil, 8);

            #undef DSF_CASE

            case DepthStencilFormats::Depth24U_Stencil8:
                BPAssert(channel.has_value(), "Depth24U_Stencil8 is not uniform");
                switch (channel.value())
                {
                    case AllChannels::Depth: return 24;
                    case AllChannels::Stencil: return 8;

                    SWITCH_DEFAULT(AllChannels, channel.value())
                        return 0;
                }

            case DepthStencilFormats::Depth32F_Stencil8:
                BPAssert(channel.has_value(), "Depth32F_Stencil8 is not uniform");
                switch (channel.value())
                {
                    case AllChannels::Depth: return 32;
                    case AllChannels::Stencil: return 8;

                    SWITCH_DEFAULT(AllChannels, channel.value())
                        return 0;
                }

            SWITCH_DEFAULT(DepthStencilFormats, AsDepthStencil())
                return 0;
        }
    }
    else
    {
        std::string errMsg = "Unknown format type, index ";
        errMsg += std::to_string(data.index());
        BPAssert(false, errMsg.c_str());
        return 0;
    }

    #undef COMPUTE_SEPARATE_OUTPUT
}
uint_fast8_t Format::GetPixelBitSize() const
{
    return GetChannelBitSize(AllChannels::Red) +
           GetChannelBitSize(AllChannels::Green) +
           GetChannelBitSize(AllChannels::Blue) +
           GetChannelBitSize(AllChannels::Alpha) +
           GetChannelBitSize(AllChannels::Depth) +
           GetChannelBitSize(AllChannels::Stencil);
}
uint_fast32_t Format::GetByteSize(const glm::uvec3& textureSize) const
{
    glm::u32 nPixels = glm::compMul(textureSize);

    if (IsSimple())
    {
        auto simple = AsSimple();
        return nPixels *
               simple.ChannelBitSize._to_integral() *
               simple.Components._to_integral();
    }
    else if (IsSpecial())
    {
        switch (AsSpecial())
        {
            case SpecialFormats::R3_G3_B2: return nPixels;

            case SpecialFormats::R5_G6_B5: return nPixels * 2;
            case SpecialFormats::RGB5_A1: return nPixels * 2;

            case SpecialFormats::RGB10_A2: return nPixels * 4;
            case SpecialFormats::RGB10_A2_UInt: return nPixels * 4;
            case SpecialFormats::RGB_SharedExpFloats: return nPixels * 4;
            case SpecialFormats::RGB_TinyFloats: return nPixels * 4;

            case SpecialFormats::sRGB: return nPixels * 3;
            case SpecialFormats::sRGB_LinearAlpha: return nPixels * 4;

            SWITCH_DEFAULT(SpecialFormats, AsSpecial())
                return 0;
        }
    }
    else if (IsCompressed())
    {
        //The texture is stored in blocks, so pad the size out to fit whole blocks.
        auto blockSize = GetBlockSize(AsCompressed());
        glm::u32 nBlocks = glm::compMul(GetBlockCount(AsCompressed(), textureSize));

        switch (AsCompressed())
        {
            //Reference: https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_texture_compression_rgtc.txt

            case CompressedFormats::Greyscale_NormalizedUInt:
            case CompressedFormats::Greyscale_NormalizedInt:
                return nBlocks * 8; //64 bits per block

            case CompressedFormats::RG_NormalizedUInt:
            case CompressedFormats::RG_NormalizedInt:
                return nBlocks * 16; //128 bits per block

            case CompressedFormats::RGB_Float:
            case CompressedFormats::RGB_UFloat:
            case CompressedFormats::RGBA_NormalizedUInt:
            case CompressedFormats::RGBA_sRGB_NormalizedUInt:
                return nBlocks * 16; //128 bits per block

            SWITCH_DEFAULT(CompressedFormats, AsCompressed())
                return 0;
        }
    }
    else if (IsDepthStencil())
    {
        switch (AsDepthStencil())
        {
            case DepthStencilFormats::Depth_16U: return nPixels * 2;
            case DepthStencilFormats::Depth_24U: return nPixels * 3;
            case DepthStencilFormats::Depth_32U: return nPixels * 4;
            case DepthStencilFormats::Depth_32F: return nPixels * 4;

            case DepthStencilFormats::Stencil_8: return nPixels;

            case DepthStencilFormats::Depth24U_Stencil8: return nPixels * 4;
            case DepthStencilFormats::Depth32F_Stencil8: return nPixels * 8; //The stencil bits are padded out to 4 bytes

            SWITCH_DEFAULT(DepthStencilFormats, AsDepthStencil())
                return 0;
        }
    }
    else
    {
        std::string errMsg = "Unknown format type, index ";
        errMsg += std::to_string(data.index());
        BPAssert(false, errMsg.c_str());
        return false;
    }
}


GLenum Format::GetOglEnum() const
{
    if (IsSimple())
    {
        auto _data = AsSimple();

        #define SWITCH_COMPONENTS(r, rg, rgb, rgba) \
            switch (_data.Components) \
            { \
                case SimpleFormatComponents::R:    { r }    break; \
                case SimpleFormatComponents::RG:   { rg }   break; \
                case SimpleFormatComponents::RGB:  { rgb }  break; \
                case SimpleFormatComponents::RGBA: { rgba } break; \
                SWITCH_DEFAULT(SimpleFormatComponents, _data.Components) return GL_NONE; \
            }
        
        #define SWITCH_TYPES(flt, normUint, normInt, uint_, int_) \
            switch (_data.Type) \
            { \
                case FormatTypes::Float: { flt } break; \
                case FormatTypes::NormalizedUInt: { normUint } break; \
                case FormatTypes::NormalizedInt: { normInt } break; \
                case FormatTypes::UInt: { uint_ } break; \
                case FormatTypes::Int: { int_ } break; \
                SWITCH_DEFAULT(FormatTypes, _data.Type) return GL_NONE; \
            }
        #define R(v) return v;

        switch (_data.ChannelBitSize)
        {
            case SimpleFormatBitDepths::B2:
                if (_data.Type == +FormatTypes::NormalizedUInt &&
                    _data.Components == +SimpleFormatComponents::RGBA)
                {
                    return GL_RGBA2;
                }
                else
                {
                    return GL_NONE;
                }

            case SimpleFormatBitDepths::B4:
                if (_data.Type == +FormatTypes::NormalizedUInt)
                    SWITCH_COMPONENTS(R(GL_NONE), R(GL_NONE), R(GL_RGB4), R(GL_RGBA4))
                else
                    return GL_NONE;

            case SimpleFormatBitDepths::B5:
                if (_data.Components == +SimpleFormatComponents::RGB &&
                    _data.Type == +FormatTypes::NormalizedUInt)
                {
                    return GL_RGB5;
                }
                else
                {
                    return GL_NONE;
                }

            case SimpleFormatBitDepths::B8:
                SWITCH_TYPES(R(GL_NONE),
                             SWITCH_COMPONENTS(R(GL_R8), R(GL_RG8), R(GL_RGB8), R(GL_RGBA8)),
                             SWITCH_COMPONENTS(R(GL_R8_SNORM), R(GL_RG8_SNORM),
                                               R(GL_RGB8_SNORM), R(GL_RGBA8_SNORM)),
                             SWITCH_COMPONENTS(R(GL_R8UI), R(GL_RG8UI),
                                               R(GL_RGB8UI), R(GL_RGBA8UI)),
                             SWITCH_COMPONENTS(R(GL_R8I), R(GL_RG8I),
                                               R(GL_RGB8I), R(GL_RGBA8I)))

            case SimpleFormatBitDepths::B10:
                if (_data.Components == +SimpleFormatComponents::RGB &&
                    _data.Type == +FormatTypes::NormalizedUInt)
                {
                    return GL_RGB10;
                }
                else
                {
                    return GL_NONE;
                }

            case SimpleFormatBitDepths::B12:
                if (_data.Type == +FormatTypes::NormalizedUInt)
                    SWITCH_COMPONENTS(R(GL_NONE), R(GL_NONE), R(GL_RGB12), R(GL_RGBA12))
                else
                    return GL_NONE;

            case SimpleFormatBitDepths::B16:
                SWITCH_TYPES(SWITCH_COMPONENTS(R(GL_R16F), R(GL_RG16F),
                                               R(GL_RGB16F), R(GL_RGBA16F)),
                             SWITCH_COMPONENTS(R(GL_R16), R(GL_RG16),
                                               R(GL_RGB16F), R(GL_RGBA16F)),
                             SWITCH_COMPONENTS(R(GL_R16_SNORM), R(GL_RG16_SNORM),
                                               R(GL_RGB16_SNORM), R(GL_RGBA16_SNORM)),
                             SWITCH_COMPONENTS(R(GL_R16UI), R(GL_RG16UI),
                                               R(GL_RGB16UI), R(GL_RGBA16UI)),
                             SWITCH_COMPONENTS(R(GL_R16I), R(GL_RG16I),
                                               R(GL_RGB16I), R(GL_RGBA16I)))

            case SimpleFormatBitDepths::B32:
                SWITCH_TYPES(SWITCH_COMPONENTS(R(GL_R32F), R(GL_RG32F),
                                               R(GL_RGB32F), R(GL_RGBA32F)),
                             R(GL_NONE), R(GL_NONE),
                             SWITCH_COMPONENTS(R(GL_R32UI), R(GL_RG32UI),
                                               R(GL_RGB32UI), R(GL_RGBA32UI)),
                             SWITCH_COMPONENTS(R(GL_R32I), R(GL_RG32I),
                                               R(GL_RGB32I), R(GL_RGBA32I)))


            SWITCH_DEFAULT(SimpleFormatBitDepths, _data.ChannelBitSize)
                return GL_NONE;
        }

        #undef R
        #undef SWITCH_TYPES
        #undef SWITCH_COMPONENTS
    }
    else if (IsSpecial())
    {
        return (GLenum)AsSpecial();
    }
    else if (IsCompressed())
    {
        return (GLenum)AsCompressed();
    }
    else if (IsDepthStencil())
    {
        return (GLenum)AsDepthStencil();
    }
    else
    {
        std::string errMsg = "Unknown format type, index ";
        errMsg += std::to_string(data.index());
        BPAssert(false, errMsg.c_str());
        return GL_NONE;
    }
}
GLenum Format::GetNativeOglEnum(std::optional<Textures::Types> texType) const
{
    GLenum glType = GL_RENDERBUFFER;
    if (texType.has_value())
        glType = texType.value()._to_integral();

    GLint actualFormat;
    glGetInternalformativ(glType, GetOglEnum(), GL_INTERNALFORMAT_PREFERRED,
                          1, &actualFormat);
    return (GLenum)actualFormat;
}
bool Format::IsNativelySupported(std::optional<Textures::Types> texType) const
{
    return GetNativeOglEnum(texType) == GetOglEnum();
}

#undef SWITCH_DEFAULT