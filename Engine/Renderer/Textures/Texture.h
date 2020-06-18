#pragma once

#include <numeric>
#include <array>

#include "Sampler.h"
#include "../../Math/Box.hpp"


namespace Bplus::GL::Textures
{
    //The different kinds of textures in OpenGL.
    BETTER_ENUM(Types, GLenum,
        OneD = GL_TEXTURE_1D,
        TwoD = GL_TEXTURE_2D,
        ThreeD = GL_TEXTURE_3D,
        Cubemap = GL_TEXTURE_CUBE_MAP,

        //A special kind of 2D texture that supports MSAA.
        TwoD_Multisample = GL_TEXTURE_2D_MULTISAMPLE
    );

    //Subsets of color channels when uploading/downloading pixel data,
    //    in byte order.
    BETTER_ENUM(ComponentData, GLenum,
        Red = GL_RED,
        Green = GL_GREEN,
        Blue = GL_BLUE,
        RG = GL_RG,
        RGB = GL_RGB,
        BGR = GL_BGR,
        RGBA = GL_RGBA,
        BGRA = GL_BGRA
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


    //The base class for all OpenGL textures.
    //Designed to be used with OpenGL's Bindless Textures extension.
    class BP_API Texture
    {
    public:
        Texture(Types type, Format format, uint_mipLevel_t nMipLevels,
                const Sampler<3>& sampler3D);

        virtual ~Texture();
        
        //No copying.
        Texture(const Texture& cpy) = delete;
        Texture& operator=(const Texture& cpy) = delete;


        const Format& GetFormat() const { return format; }

        Types GetType() const { return type; }
        uint_mipLevel_t GetNMipLevels() const { return nMipLevels; }
        OglPtr::Texture GetOglPtr() const { return glPtr; }


        //Updates mipmaps for this texture.
        //Not allowed for compressed-format textures.
        void RecomputeMips();


    protected:

        OglPtr::Texture glPtr;
        Types type;
        uint_mipLevel_t nMipLevels;

        Format format;
        Sampler<3> sampler3D;

        //TODO: Track handles.


        //Child classes have access to the move constructor.
        Texture(Texture&& src);
        Texture& operator=(Texture&& src) = delete;
        

        //Given a set of components for texture uploading/downloading,
        //    and the data type of this texture's pixels,
        //    finds the corresponding OpenGL enum value.
        GLenum GetOglChannels(ComponentData components) const
        {
            //If the pixel format isn't integer (i.e. it's float or normalized integer),
            //    we can directly use the enum values.
            //Otherwise, we should be sending the "integer" enum values.
            if (!format.IsInteger())
                return (GLenum)components;
            else switch (components)
            {
                case ComponentData::Red: return GL_RED_INTEGER;
                case ComponentData::Green: return GL_GREEN_INTEGER;
                case ComponentData::Blue: return GL_BLUE_INTEGER;
                case ComponentData::RG: return GL_RG_INTEGER;
                case ComponentData::RGB: return GL_RGB_INTEGER;
                case ComponentData::BGR: return GL_BGR_INTEGER;
                case ComponentData::RGBA: return GL_RGBA_INTEGER;
                case ComponentData::BGRA: return GL_BGRA_INTEGER;

                default:
                    std::string msg = "Unexpected data component type: ";
                    msg += components._to_string();
                    BPAssert(false, msg.c_str());
                    return GL_NONE;
            }
        }

        //Given a type T, finds the corresponding GLenum for that type of data.
        //    bool types are interpreted as unsigned integers of the same size.
        template<typename T>
        GLenum GetOglInputFormat() const
        {
            //Deduce the "type" argument for OpenGL, representing
            //    the size of each channel being sent in.
            GLenum type = GL_NONE;
            if constexpr (std::is_same_v<T, bool>) {
                if constexpr (sizeof(bool) == 1) {
                    type = GL_UNSIGNED_BYTE;
                } else if constexpr (sizeof(bool) == 2) {
                    type = GL_UNSIGNED_SHORT;
                } else if constexpr (sizeof(bool) == 4) {
                    type = GL_UNSIGNED_INT;
                } else {
                    static_assert(false, "Unexpected value for sizeof(bool)");
                }
            } else if constexpr (std::is_same_v<T, glm::u8>) {
                type = GL_UNSIGNED_BYTE;
            } else if constexpr (std::is_same_v<T, glm::u16>) {
                type = GL_UNSIGNED_SHORT;
            } else if constexpr (std::is_same_v<T, glm::u32>) {
                type = GL_UNSIGNED_INT;
            } else if constexpr (std::is_same_v<T, glm::i8>) {
                type = GL_BYTE;
            } else if constexpr (std::is_same_v<T, glm::i16>) {
                type = GL_SHORT;
            } else if constexpr (std::is_same_v<T, glm::i32>) {
                type = GL_INT;
            } else if constexpr (std::is_same_v<T, glm::f32>) {
                type = GL_FLOAT;
            } else {
                T ta;
                ta.aslflskjflsjf4444 = 4;
                static_assert(false, "T is an unexpected type");
            }

            return type;
        }

        //Given a number of dimensions, and a switch for reversed (BGR) ordering,
        //    finds the corresponding enum value representing component ordering
        //    for texture upload/download.
        template<glm::length_t L>
        ComponentData GetComponents(bool bgrOrdering) const
        {
            if constexpr (L == 1) {
                return ComponentData::Greyscale;
            } else if constexpr (L == 2) {
                return ComponentData::RG;
            } else if constexpr (L == 3) {
                return (bgrOrdering ?
                            ComponentData::BGR :
                            ComponentData::RGB);
            } else if constexpr (L == 4) {
                return (bgrOrdering ?
                            ComponentData::BGRA :
                            ComponentData::RGBA);
            } else {
                static_assert(false, "L should be between 1 and 4");
                return ComponentData::Greyscale;
            }
        }
    };
}