#pragma once

#include <numeric>

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

    //Gets the maximum number of mipmaps for a texture of the given size.
    template<glm::length_t L>
    uint_mipLevel_t GetMaxNumbMipmaps(const glm::vec<L, glm::u32>& texSize)
    {
        auto largestAxis = glm::compMax(texSize);
        return 1 + (uint_mipLevel_t)floor(log2(largestAxis));
    }


    //An extremely basic wrapper around OpenGL textures.
    //More full-featured wrappers are defined below and inherit from this one.
    class BP_API Texture
    {
    public:
        //Creates a new texture.
        //Pass "1" for nMipLevels to not use mip-maps.
        //Pass "0" for nMipLevels to generate full mip-maps down to a single pixel.
        //Pass anything else to generate a fixed amount of mip levels.
        Texture(Types type, Format format, uint_mipLevel_t nMipLevels);
        virtual ~Texture();
        
        //No copying.
        Texture(const Texture& cpy) = delete;
        Texture& operator=(const Texture& cpy) = delete;


        const Format& GetFormat() const { return format; }

        Types GetType() const { return type; }
        uint_mipLevel_t GetNMipLevels() const { return nMipLevels; }
        OglPtr::Texture GetOglPtr() const { return glPtr; }

    protected:

        OglPtr::Texture glPtr;
        Types type;
        uint_mipLevel_t nMipLevels;

        Format format;

        
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
            if constexpr (std::is_same_v<T, bool>)
            {
                if constexpr (sizeof(bool) == 1) {
                    type = GL_UNSIGNED_BYTE;
                } else if constexpr (sizeof(bool) == 2) {
                    type = GL_UNSIGNED_SHORT;
                } else if constexpr (sizeof(bool) == 4) {
                    type = GL_UNSIGNED_INT;
                } else {
                    static_assert(false, "Unexpected value for sizeof(bool)");
                }
            }
            else if constexpr (std::is_same_v<T, glm::u8>) {
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


        //Child classes have access to the move operator.
        Texture(Texture&& src);
        Texture& operator=(Texture&& src);
    };

    //A great reference for getting/setting texture data in OpenGL:
    //    https://www.khronos.org/opengl/wiki/Pixel_Transfer


    //TODO: 'Handle' class.

    /*
    //An OpenGL object representing a 2D grid of pixels that can be "sampled" in shaders.
    //May also be an array of textures.
    class BP_API Texture2D
    {
    public:


        //Creates a new texture.
        //Pass "1" for nMipLevels to not use mip-maps.
        //Pass "0" for nMipLevels to generate full mip-maps down to 1x1.
        //Pass anything else to generate a fixed amount of mip levels.
        Texture2D(const glm::uvec2& size, Format format,
                  uint_mipLevel_t nMipLevels = 0);

        virtual ~Texture2D();


        //No copying.
        Texture2D(const Texture2D& cpy) = delete;
        Texture2D& operator=(const Texture2D& cpy) = delete;

        //Move operators.
        Texture2D(Texture2D&& src);
        Texture2D& operator=(Texture2D&& src);


        glm::uvec2 GetSize(uint_mipLevel_t mipLevel = 0) const;

        //Gets the number of bytes needed to store this texture in its native format.
        size_t GetByteSize(uint_mipLevel_t mipLevel = 0) const { return format.GetByteSize(GetSize(mipLevel)); }
        //Gets the total byte-size of this texture, across all mip levels.
        size_t GetTotalByteSize() const
        {
            size_t sum = 0;
            for (uint_mipLevel_t mip = 0; mip < nMipLevels; ++mip)
                sum += GetByteSize(mip);
            return sum;
        }

        Types GetType() const { return type; }
        uint_mipLevel_t GetMipCount() const { return nMipLevels; }


        Sampler<2> GetSampler() const { return sampler; }
        void SetSampler(const Sampler<2>& s);


        //TODO: How to implement setting/getting of depth and/or stencil? Refer to https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml, plus page 194 of https://www.khronos.org/registry/OpenGL/specs/gl/glspec45.core.pdf

        #pragma region Setting data

        //Optional parameters when uploading texture data.
        struct SetDataParams
        {
            //The subset of the texture to set.
            //A size-0 box represents the full texture.
            Math::Box2Du DestRange = Math::Box2Du::MakeCenterSize(glm::uvec2{ 0 }, glm::uvec2{ 0 });
            //The mip level. 0 is the original texture, higher values are smaller mips.
            uint_fast32_t MipLevel = 0;

            //If true, all mip-levels will be automatically recomputed after this operation.
            bool RecomputeMips;

            SetDataParams(bool recomputeMips = true)
                : RecomputeMips(recomputeMips) { }
            SetDataParams(const Math::Box2Du& destRange,
                          bool recomputeMips = true)
                : DestRange(destRange), RecomputeMips(recomputeMips) { }
            SetDataParams(uint_fast32_t mipLevel,
                          bool recomputeMips = false)
                : MipLevel(mipLevel), RecomputeMips(recomputeMips) { }
            SetDataParams(const Math::Box2Du& destRange,
                          uint_fast32_t mipLevel,
                          bool recomputeMips = false)
                : DestRange(destRange), MipLevel(mipLevel), RecomputeMips(recomputeMips) { }
        };


        //Updates mipmaps for this texture.
        //Not allowed if the texture is compressed.
        void RecomputeMips();

        //Clears part or all of this texture to the given value.
        //Not allowed for compressed-format textures.
        template<glm::length_t L, typename T>
        void Clear(const glm::vec<L, T> value, SetDataParams optionalParams = { })
        {
            //TODO: Implement.
        }


        //Note that pixel data in OpenGL is ordered from left to right,
        //    then from bottom to top, then (for 3D data) from back to front.
        //In other words, rows are contiguous and then grouped vertically.

        //Sets a color or depth texture with the given data.
        //Note that the upload to the GPU will be slower if
        //    the data doesn't exactly match the texture's pixel format.
        //It's also not recommended to use this to upload to a compressed texture format,
        //    because GPU drivers may not have good-quality compression algorithms.
        template<typename T>
        void SetData(const T* data, ComponentData components,
                     SetDataParams optionalParams = { })
        {
            SetData((const void*)data,
                    GetOglChannels(components), GetOglInputFormat<T>(),
                    optionalParams);
        }

        //Sets any kind of color or depth texture with the given data.
        //The number of components is determined by the size of the vector type --
        //    1D is Red/Depth, 2D is Red-Green, 3D is RGB, 4D is RGBA.
        //If "bgrOrdering" is true, then the incoming RGB(A) data is actually in BGR(A) order.
        //BGR order is often more efficient to give to the GPU.
        template<glm::length_t L, typename T>
        void SetData(const glm::vec<L, T>* data,
                     bool bgrOrdering = false,
                     SetDataParams optionalParams = { })
        {
            SetData(glm::value_ptr(*data),
                    GetOglChannels(GetComponents<L>(bgrOrdering)),
                    GetOglInputFormat<T>(),
                    optionalParams);
        }

        //Directly sets block-compressed data for the texture,
        //    based on its current format.
        //The input data should be "GetByteCount()" bytes large.
        //This is highly recommended over the other "SetData()" forms for compressed textures;
        //    while the GPU driver should support doing the compression under the hood for you,
        //    the results vary widely and are often bad.
        //Note that, because Block-Compression works in square groups of pixels,
        //    the destination rectangle is in units of blocks, not individual pixels.
        //Additionally, mipmaps cannot be regenerated automatically.
        void SetData_Compressed(const std::byte* compressedData, size_t compressedDataSize,
                                Math::Box2Du destBlockRange = Math::Box2Du(),
                                uint_mipLevel_t mipLevel = 0);

        //TODO: Figure out whether all the trouble for this function is worth it. https://www.khronos.org/opengl/wiki/Pixel_Transfer
        /*
            //Sets the data for a texture using the given pre-processed byte data.
            //This prevents the need for the GPU driver to convert each pixel when uploading.
            //This only works for the "Special" formats,
            //    except for sRGB, sRGB_LinearAlpha, RGB_TinyFloats and RGB_SharedExpFloats.
            //Returns whether it was successful.
            //If this platform is little-endian, then the packed format should be stored "reversed",
            //    meaning the components (but NOT the bits inside each component)
            //    are in reverse order.
            //You can use "IsPlatformLittleEndian()" to check whether this is the case.
            bool SetData_PrePacked(const std::byte* data, SetDataParams optionalParams = { })
            {
                BPAssert(format.IsSpecial(),
                         "Tried to set PrePacked data for a non-special format");

                GLenum glFormat;
                switch (format.AsSpecial())
                {
                    #define CHOOSE(formatVal, littleEndian, bigEndian) \
                        case SpecialFormats::formatVal: \
                            glFormat = (IsPlatformLittleEndian() ? littleEndian : bigEndian); \
                        break
                    CHOOSE(R3_G3_B2, GL_UNSIGNED_BYTE_2_3_3_REV, GL_UNSIGNED_BYTE_3_3_2);
                    CHOOSE(R5_G6_B5, GL_UNSIGNED_SHORT_5_6_5_REV, GL_UNSIGNED_SHORT_5_6_5);
                    CHOOSE(RGB10_A2, GL_UNSIGNED_INT_2_10_10_10_REV, GL_UNSIGNED_INT_10_10_10_2);
                    CHOOSE(RGB10_A2_UInt, GL_UNSIGNED_INT_2_10_10_10_REV, GL_UNSIGNED_INT_10_10_10_2);
                    CHOOSE(RGB5_A1, GL_UNSIGNED_SHORT_1_5_5_5_REV, GL_UNSIGNED_SHORT_5_5_5_1);
                    #undef CHOOSE

                    default: BPAssert(false, "Unknown special format"); return false;
                }
                
                //Unfinished here
            }
        * /

        #pragma endregion
        
        #pragma region Getting data

        //Optional parameters when downloading texture data.
        struct GetDataParams
        {
            //The subset of the texture to set.
            //A size-0 box represents the full texture.
            Math::Box2Du Range = Math::Box2Du::MakeCenterSize(glm::uvec2{ 0 }, glm::uvec2{ 0 });
            //The mip level. 0 is the original texture, higher values are smaller mips.
            uint_fast32_t MipLevel = 0;

            GetDataParams() { }
            GetDataParams(const Math::Box2Du& range)
                : Range(range) { }
            GetDataParams(uint_fast32_t mipLevel)
                : MipLevel(mipLevel) { }
            GetDataParams(const Math::Box2Du& range,
                          uint_fast32_t mipLevel)
                : Range(range), MipLevel(mipLevel) { }
        };

        template<typename T>
        void GetData(T* data, ComponentData components,
                     GetDataParams optionalParams = { }) const
        {
            GetData((void*)data,
                    GetOglChannels(components), GetOglInputFormat<T>(),
                    optionalParams);
        }

        //Gets any kind of color or depth texture data and writes it to the given buffer.
        //1D data is interpreted as R channel (or depth values),
        //    2D as RG, 3D as RGB, and 4D as RGBA.
        //If "bgrOrdering" is true, then the incoming RGB(A) data is actually in BGR(A) order.
        template<glm::length_t L, typename T>
        void GetData(glm::vec<L, T>* data, bool bgrOrdering = false,
                     GetDataParams optionalParams = { }) const
        {
            GetData<T>(glm::value_ptr(*data),
                       GetComponents<L>(bgrOrdering),
                       optionalParams);
        }

        //Directly reads block-compressed data from the texture,
        //    based on its current format.
        //This is a fast, direct copy of the byte data stored in the texture.
        //Note that, because Block-Compression works in square groups of pixels,
        //    the "range" rectangle is in units of blocks, not individual pixels.
        void GetData_Compressed(void* compressedData,
                                Math::Box2Du blockRange = { },
                                uint_mipLevel_t mipLevel = 0) const;

        #pragma endregion


    private:

        OglPtr::Texture glPtr;

        Types type;
        //TODO: multisample?
        Format format;

        glm::uvec2 size = { 0, 0 };
        uint_mipLevel_t nMipLevels;

        Sampler<2> sampler;

        void SetData(const void* data,
                     GLenum dataFormat, GLenum dataType,
                     const SetDataParams& params);
        void GetData(void* data,
                     GLenum dataFormat, GLenum dataType,
                     const GetDataParams& params) const;


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

        template<typename T>
        GLenum GetOglInputFormat() const
        {
            //Deduce the "type" argument for OpenGL, representing
            //    the size of each channel being sent in.
            GLenum type = GL_NONE;
            if constexpr (std::is_same_v<T, bool>)
            {
                if constexpr (sizeof(bool) == 1) {
                    type = GL_UNSIGNED_BYTE;
                } else if constexpr (sizeof(bool) == 2) {
                    type = GL_UNSIGNED_SHORT;
                } else if constexpr (sizeof(bool) == 4) {
                    type = GL_UNSIGNED_INT;
                } else {
                    static_assert(false, "Unexpected value for sizeof(bool)");
                }
            }
            else if constexpr (std::is_same_v<T, glm::u8>) {
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
                static_assert(false, "T is an unexpected type");
            }

            return type;
        }

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
    */
}