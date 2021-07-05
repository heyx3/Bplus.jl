#pragma once

#include <numeric>
#include <array>

#include "Sampler.h"
#include "Format.h"

#include "../../Math/Box.hpp"


namespace Bplus::GL::Textures
{
    //Subsets of color channels when uploading/downloading pixel data, in byte order.
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

    //Gets the OpenGL enum representing an integer-type version of the given components.
    GLenum GetIntegerVersion(PixelIOChannels components);

    //Determines the subset of components that match a GLM vector of the given number of dimensions.
    //For example, L=2 means fvec2, which results in PixelIOChannels::RG.
    template<glm::length_t L>
    inline PixelIOChannels GetPixelIOChannels(bool bgrOrdering,
                                              PixelIOChannels valueFor1D = PixelIOChannels::Red)
    {
        if constexpr (L == 1) {
            return PixelIOChannels::Red;
        } else if constexpr (L == 2) {
            return PixelIOChannels::RG;
        } else if constexpr (L == 3) {
            return (bgrOrdering ?
                        PixelIOChannels::BGR :
                        PixelIOChannels::RGB);
        } else if constexpr (L == 4) {
            return (bgrOrdering ?
                        PixelIOChannels::BGRA :
                        PixelIOChannels::RGBA);
        } else {
            static_assert(false, "L should be between 1 and 4");
            return PixelIOChannels::Greyscale;
        }
    }


    //Data types that GPU pixel data can be uploaded/downloaded in.
    BETTER_ENUM(PixelIOTypes, GLenum,
        UInt8 = GL_UNSIGNED_BYTE,
        UInt16 = GL_UNSIGNED_SHORT,
        UInt32 = GL_UNSIGNED_INT,
        Int8 = GL_BYTE,
        Int16 = GL_SHORT,
        Int32 = GL_INT,
        Float32 = GL_FLOAT
    );
    //Gets the byte-size of the given pixel data type.
    inline uint_fast32_t GetByteSize(PixelIOTypes channelDataType)
    {
        switch (channelDataType)
        {
            case PixelIOTypes::UInt8:
            case PixelIOTypes::Int8:
                return 1;
            case PixelIOTypes::UInt16:
            case PixelIOTypes::Int16:
                return 2;
            case PixelIOTypes::UInt32:
            case PixelIOTypes::Int32:
            case PixelIOTypes::Float32:
                return 4;
            default:
                BP_ASSERT_STR(false,
                              "Unexpected Bplus::GL::Textures::PixelIOTypes::" +
                                  channelDataType._to_string());
                return 1;
        }
    }
    //Compile-time determination of a type for GPU texture upload/download.
    template<typename T>
    inline PixelIOTypes GetPixelIOType()
    {
        if constexpr (std::is_same_v<T, bool>) {
            if constexpr (sizeof(bool) == 1) {
                return PixelIOTypes::UInt8;
            } else if constexpr (sizeof(bool) == 2) {
                return PixelIOTypes::UInt16;
            } else if constexpr (sizeof(bool) == 4) {
                return PixelIOTypes::UInt32;
            } else {
                static_assert(false, "Unexpected value for sizeof(bool)");
            }
        } else if constexpr (std::is_same_v<T, glm::u8>) {
            return PixelIOTypes::UInt8;
        } else if constexpr (std::is_same_v<T, glm::u16>) {
            return PixelIOTypes::UInt16;
        } else if constexpr (std::is_same_v<T, glm::u32>) {
            return PixelIOTypes::UInt32;
        } else if constexpr (std::is_same_v<T, glm::i8>) {
            return PixelIOTypes::Int8;
        } else if constexpr (std::is_same_v<T, glm::i16>) {
            return PixelIOTypes::Int16;
        } else if constexpr (std::is_same_v<T, glm::i32>) {
            return PixelIOTypes::Int32;
        } else if constexpr (std::is_same_v<T, glm::f32>) {
            return PixelIOTypes::Float32;
        } else {
            static_assert(false, "T is an unexpected type");
            return PixelIOTypes::UInt32;
        }
    }

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

//Make these types hashable/equatable:

BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Textures::PixelIOChannels);
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Textures::PixelIOTypes);
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Textures::ImageAccessModes);

BP_HASH_EQ_TEMPL_START(Bplus::GL::Textures,
                       glm::length_t N, SetDataParams<N>,
                       d.DestRange, d.Miplevel, d.RecomputeMips)
    return a.DestRange == b.DestRange &&
           a.MipLevel == b.MipLevel &&
           a.RecomputeMips == b.RecomputeMips;
BP_HASH_EQ_END
BP_HASH_EQ_TEMPL_START(Bplus::GL::Textures,
                       glm::length_t N, GetDataParams<N>,
                       d.Range, d.Miplevel)
    return a.Range == b.Range &&
           a.MipLevel == b.MipLevel;
BP_HASH_EQ_END