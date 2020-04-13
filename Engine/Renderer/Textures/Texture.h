#pragma once

#include "Sampler.h"
#include "../../Math/Box.hpp"


namespace Bplus::GL::Textures
{
    //The different kinds of textures in OpenGL.
    BETTER_ENUM(TextureTypes, GLenum,
        OneD = GL_TEXTURE_1D,
        TwoD = GL_TEXTURE_2D,
        ThreeD = GL_TEXTURE_3D,
        Cubemap = GL_TEXTURE_CUBE_MAP,

        //A special kind of 2D texture that supports MSAA.
        TwoD_Multisample = GL_TEXTURE_2D_MULTISAMPLE
    );
    
    //Different types of specially-packed data that can be uploaded to texture memory.
    //These are meant to speed up load times
    //    when using the corresponding "Special" texture format on the GPU side.
    //Each type of format can also be stored "reversed", meaning
    //    the components (but NOT the bits inside each component) are in reverse order.
    //Note that, because these packed formats are read as whole, multi-byte integers,
    //    the endianness of your platform needs to be taken into account.
    //For example, on Intel platforms, data is stored little-endian,
    //    so data that you think is stored as U32_RGBA_8888 is actually stored as U32_ABGR_8888.
    //TODO: Probably have to remove this enum; it won't get used :(
    BETTER_ENUM(TextureUploadPackedFormats, GLenum,
        //RGB packed into an unsigned 8-bit integer
        //    as 332 (R is 3 bits, G is 3 bits, B is 2 bits).
        U8_RGB_332 = GL_UNSIGNED_BYTE_3_3_2,
        //RGB packed reversed into an unsigned 8-bit integer
        //    as 233 (B is 2 bits, G is 3 bits, R is 3 bits).
        U8_BGR_233 = GL_UNSIGNED_BYTE_2_3_3_REV,
                
        //RGB packed into an unsigned 16-bit integer
        //    as 565 (R is 5 bits, G is 6 bits, B is 5 bits).
        U16_RGB_565 = GL_UNSIGNED_SHORT_5_6_5,
        //RGB packed reversed into an unsigned 16-bit integer
        //    as 565 (B is 5 bits, G is 6 bits, R is 5 bits).
        U16_BGR_565 = GL_UNSIGNED_SHORT_5_6_5_REV,

        //RGBA packed into an unsigned short with 4 bits per component.
        U16_RGBA_4444 = GL_UNSIGNED_SHORT_4_4_4_4,
        //RGBA packed backwards (so, ABGR) into an unsigned short with 4 bits per component.
        U16_ABGR_4444 = GL_UNSIGNED_SHORT_4_4_4_4_REV,

        //RGBA packed into an unsigned short with 5 bits per color component,
        //    and 1 bit for alpha.
        U16_RGBA_5551 = GL_UNSIGNED_SHORT_5_5_5_1,
        //RGBA packed backwards (so, ABGR) into an unsigned short
        //    with 1 bit for alpha, and 5 bits per color component.
        U16_ABGR_1555 = GL_UNSIGNED_SHORT_1_5_5_5_REV,
                
        //RGBA packed into an unsigned int32 with 8 bits per component.
        U32_RGBA_8888 = GL_UNSIGNED_INT_8_8_8_8,
        //RGBA packed backwards (so, ABGR) into an unsigned int32 with 8 bits per component.
        U32_ABGR_8888 = GL_UNSIGNED_INT_8_8_8_8,

        //RGBA packed into an unsigned int32 with 10 bits per color component,
        //    and 2 bits for alpha.
        U32_RGBA_1010102 = GL_UNSIGNED_INT_10_10_10_2,
        //RGBA packed backwards (so, ABGR) into an unsigned int32
        //    with 2 bits for alpha, and 10 bits per color component.
        U32_ABGR_2101010 = GL_UNSIGNED_INT_2_10_10_10_REV,

        //RGB packed backwards (so, BGR) into an unsigned int32 as smaller floats:
        //    10-bit for B, 11-bit for G, 11-bit for R.
        U32_BGR_101111 = GL_UNSIGNED_INT_10F_11F_11F_REV,
        //RGB packed backwards (so, BGR) into an unsigned int32
        //    as 14-bit floats who share their 5-bit exponent.
        U32_EBGR_5999 = GL_UNSIGNED_INT_5_9_9_9_REV,

        //A special stencil upload format -- the first 24 bits are ignored (representing depth),
        //    and the last 8 are the stencil value.
        U32_Stencil_Last8 = GL_UNSIGNED_INT_24_8,
        //A special depth/stencil upload format --
        //    the first 32 bits are the depth value as a 32-bit float,
        //    the next 24 bits are ignored, and the last 8 bits are the stencil.
        F32U32_DepthStencil_32_Last8 = GL_FLOAT_32_UNSIGNED_INT_24_8_REV
    );


    //Gets the maximum number of mipmaps for a texture of the given size.
    template<glm::length_t L>
    uint_fast16_t GetMaxNumbMipmaps(const glm::vec<L, glm::u32>& texSize)
    {
        auto largestAxis = glm::compMax(texSize);
        return 1 + (uint_fast16_t)floor(log2(largestAxis));
    }


    //An OpenGL object representing a 2D grid of pixels that can be "sampled" in shaders.
    //May also be an array of textures.
    class Texture2D
    {
    public:

        //Creates a new texture.
        //Pass "1" for nMipLevels to not use mip-maps.
        //Pass "0" for nMipLevels to generate full mip-maps down to 1x1.
        //Pass anything else to generate a fixed amount of mip levels.
        Texture2D(const glm::uvec2& size, Format format,
                  uint_fast16_t nMipLevels = 0);

        virtual ~Texture2D();


        //No copying.
        Texture2D(const Texture2D& cpy) = delete;
        Texture2D& operator=(const Texture2D& cpy) = delete;

        //Move operators.
        Texture2D(Texture2D&& src);
        Texture2D& operator=(Texture2D&& src);


        glm::uvec2 GetSize(uint_fast16_t mipLevel = 0) const; //TODO: Implement.

        TextureTypes GetType() const { return type; }
        uint_fast16_t GetMipCount() const { return nMipLevels; }

        Sampler<2> GetSampler() const { return sampler; }
        void SetSampler(const Sampler<2>& s);

        #pragma region SetData methods

        //TODO: SetData that handles pre-packed AND pre-compressed formats.
        //TODO: SetData for depth/stencil formats.


        //Below are functions for setting texture data using simple arrays of pixel components.
        //The input data does not have to match up with the texture's actual pixel format
        //    once it's in GPU memory; the driver does all the heavy lifting to convert it.
        //However, note that the driver implementations for Block-Compressing textures
        //    can vary widely in quality, so it's recommended to pre-compress them yourself.

        //They all take the form of two overloads:
        //    void SetData(const T* data, FormatComponents dataChannels,
        //                 const Math::Box2D<glm::u32>& destRange,
        //                 uint8_t mipLevel = 0);
        //    void SetData(const T* data, FormatComponents dataChannels,
        //                 uint8_t mipLevel = 0);
        //There are lots of different number types, so we use a macro to make things easier.
        
        //The pixel data should be ordered from left to right, then from bottom to top
        //    (then from back to front for 3D data).
        //In other words, rows are contiguous.

        #define MAKE_FUNC_SET_DATA_SIMPLE(numberType, dataType) \
            void SetData(const numberType* data, FormatComponents dataChannels, \
                         const Math::Box2D<glm::u32>& destRange, \
                         uint_fast16_t mipLevel = 0) { \
                SetData((const void*)data, \
                        (GLenum)dataChannels._to_integral(), dataType, \
                        destRange, mipLevel); \
            } \
            void SetData(const numberType* data, FormatComponents dataChannels, \
                         uint_fast16_t mipLevel = 0) { \
                SetData(data, dataChannels, \
                        Math::Box2D<glm::u32>{ { 0 }, GetSize(mipLevel) }, \
                        mipLevel); \
            }
        
            MAKE_FUNC_SET_DATA_SIMPLE(uint8_t,  GL_UNSIGNED_BYTE);
            MAKE_FUNC_SET_DATA_SIMPLE(int8_t,   GL_BYTE);
            MAKE_FUNC_SET_DATA_SIMPLE(uint16_t, GL_UNSIGNED_SHORT);
            MAKE_FUNC_SET_DATA_SIMPLE(int16_t,  GL_SHORT);
            MAKE_FUNC_SET_DATA_SIMPLE(uint32_t, GL_UNSIGNED_INT);
            MAKE_FUNC_SET_DATA_SIMPLE(int32_t,  GL_INT);
            MAKE_FUNC_SET_DATA_SIMPLE(float,    GL_FLOAT);

        #undef MAKE_FUNC_SET_DATA_SIMPLE

        #pragma endregion


    private:

        OglPtr::Texture glPtr;

        TextureTypes type;
        //TODO: Bool multisample
        Format format;

        glm::uvec2 size = { 0, 0 };
        uint_fast16_t nMipLevels;

        Sampler<2> sampler;

        void SetData(const void* data,
                     GLenum dataFormat, GLenum dataType,
                     const Math::Box2D<glm::u32>& destRange,
                     uint_fast16_t mipLevel);
    };


    //TODO: 'Handle' class.
}