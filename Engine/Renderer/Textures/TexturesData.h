#pragma once

#include <numeric>
#include <array>

#include "Sampler.h"
#include "../../Math/Box.hpp"


namespace Bplus::GL::Textures
{
    //Subsets of color channels when uploading/downloading pixel data,
    //    in byte order.
    //TODO: Rename "PixelIOChannels"
    BETTER_ENUM(PixelIOChannels, GLenum,
        Red = GL_RED,
        Green = GL_GREEN,
        Blue = GL_BLUE,
        RG = GL_RG,
        RGB = GL_RGB,
        BGR = GL_BGR,
        RGBA = GL_RGBA,
        BGRA = GL_BGRA
    );
    uint8_t BP_API GetNChannels(PixelIOChannels data);
    bool BP_API UsesChannel(PixelIOChannels components, ColorChannels channel);
    uint8_t BP_API GetChannelIndex(PixelIOChannels components, ColorChannels channel);

    //The different modes that an ImgView can be used in.
    BETTER_ENUM(ImageAccessModes, GLenum,
        Read = GL_READ_ONLY,
        Write = GL_WRITE_ONLY,
        ReadWrite = GL_READ_WRITE
    );


    //The unsigned integer type used to represent mip levels.
    using uint_mipLevel_t = uint_fast16_t;


    #pragma region Depth-Stencil packing helpers

    struct Unpacked_Depth24uStencil8u
    {
        unsigned Depth : 24;
        unsigned Stencil : 8;
    };
    struct Unpacked_Depth32fStencil8u
    {
        float   Depth;
        uint8_t Stencil;
        Unpacked_Depth32fStencil8u(float depth, uint8_t stencil)
            : Depth(depth), Stencil(stencil) { }
    };

    inline uint32_t Pack_DepthStencil(Unpacked_Depth24uStencil8u depthStencilValues)
    {
        return (((uint32_t)depthStencilValues.Depth) << 8) |
               depthStencilValues.Stencil;
    }
    inline uint64_t Pack_DepthStencil(Unpacked_Depth32fStencil8u depthStencilValues)
    {
        uint32_t depthAsInt = Reinterpret<float, uint32_t>(depthStencilValues.Depth),
                 stencil = (uint32_t)depthStencilValues.Stencil;

        uint64_t result = 0;
        result |= depthAsInt;
        result <<= 32;
        result |= stencil;

        return result;
    }
    
    inline Unpacked_Depth24uStencil8u Unpack_DepthStencil(uint32_t packed)
    {
        return {
                 ((packed & 0xffffff00) >> 8),
                 (packed & 0x000000ff)
               };
    }
    inline Unpacked_Depth32fStencil8u Unpack_DepthStencil(uint64_t packed)
    {
        uint32_t depthAsInt = (uint32_t)((packed & 0xffffffff00000000) >> 32);
        uint8_t stencil = (uint8_t)(packed & 0x00000000000000ff);
        return {
                   Reinterpret<uint32_t, float>(depthAsInt),
                   stencil
               };
    }

    #pragma endregion


    //Gets the maximum number of mipmaps for a texture of the given size.
    template<glm::length_t L>
    uint_mipLevel_t GetMaxNumbMipmaps(const glm::vec<L, glm::u32>& texSize)
    {
        auto largestAxis = glm::compMax(texSize);
        return 1 + (uint_mipLevel_t)floor(log2(largestAxis));
    }

    //TODO: Texture2DMSAA class.
    //TODO: Copying from one texture to another (and from framebuffer into texture? it's redundant though).
    //TODO: Memory Barriers. https://www.khronos.org/opengl/wiki/Memory_Model#Texture_barrier

    #pragma region SetDataParams and GetDataParams, for texture data IO
    
    template<glm::length_t N>
    //Optional parameters when uploading texture data.
    struct SetDataParams
    {
        using uBox_t = Math::Box<N, glm::u32>;
        using uVec_t = glm::vec<N, glm::u32>;


        //The subset of the texture to set.
        //A size-0 box represents the full texture.
        uBox_t DestRange = uBox_t::MakeCenterSize(uVec_t{ 0 }, uVec_t{ 0 });

        //The mip level. 0 is the original texture, higher values are smaller mips.
        uint_mipLevel_t MipLevel = 0;

        //If true, all mip-levels will be automatically recomputed after this operation.
        bool RecomputeMips;


        SetDataParams(bool recomputeMips = true)
            : RecomputeMips(recomputeMips) { }
        SetDataParams(const uBox_t& destRange,
                        bool recomputeMips = true)
            : DestRange(destRange), RecomputeMips(recomputeMips) { }
        SetDataParams(uint_mipLevel_t mipLevel,
                        bool recomputeMips = false)
            : MipLevel(mipLevel), RecomputeMips(recomputeMips) { }
        SetDataParams(const uBox_t& destRange,
                        uint_mipLevel_t mipLevel,
                        bool recomputeMips = false)
            : DestRange(destRange), MipLevel(mipLevel), RecomputeMips(recomputeMips) { }


        uBox_t GetRange(const uVec_t& fullSize) const
        {
            if (glm::all(glm::equal(DestRange.Size, uVec_t{ 0 })))
                return uBox_t::MakeMinSize(uVec_t{ 0 }, fullSize);
            else
                return DestRange;
        }
    };
    using SetData1DParams = SetDataParams<1>;
    using SetData2DParams = SetDataParams<2>;
    using SetData3DParams = SetDataParams<3>;
    

    template<glm::length_t N>
    //Optional parameters when downloading texture data.
    struct GetDataParams
    {
        using uBox_t = Math::Box<N, glm::u32>;
        using uVec_t = glm::vec<N, glm::u32>;


        //The subset of the texture to set.
        //A size-0 box represents the full texture.
        uBox_t Range = uBox_t::MakeCenterSize(uVec_t{ 0 }, uVec_t{ 0 });
        //The mip level. 0 is the original texture, higher values are smaller mips.
        uint_mipLevel_t MipLevel = 0;

        GetDataParams() { }
        GetDataParams(const uBox_t& range)
            : Range(range) { }
        GetDataParams(uint_mipLevel_t mipLevel)
            : MipLevel(mipLevel) { }
        GetDataParams(const uBox_t& range,
                        uint_mipLevel_t mipLevel)
            : Range(range), MipLevel(mipLevel) { }

        uBox_t GetRange(const uVec_t& fullSize) const
        {
            if (glm::all(glm::equal(Range.Size, uVec_t{ 0 })))
                return uBox_t::MakeMinSize(uVec_t{ 0 }, fullSize);
            else
                return Range;
        }
    };
    using GetData1DParams = GetDataParams<1>;
    using GetData2DParams = GetDataParams<2>;
    using GetData3DParams = GetDataParams<3>;
    
    #pragma endregion
}

//Allow enums in this file to be hashed, for use in STL collections.
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Textures::PixelIOChannels);
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Textures::Types);
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Textures::ImageAccessModes);