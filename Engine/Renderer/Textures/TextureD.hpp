#pragma once

#include "Texture.h"

namespace Bplus::GL::Textures
{
    //A simple 1D, 2D, or 3D texture.
    template<glm::length_t D>
    class TextureD : public Texture
    {
    public:
        static constexpr Types GetClassType()
        {
            if constexpr (D == 1) {
                return Types::OneD;
            } else if constexpr (D == 2) {
                return Types::TwoD;
            } else if constexpr (D == 3) {
                return Types::ThreeD;
            } else {
                static_assert(false, "TextureD<> should only be 1-, 2-, or 3-dimensional");
            }
        }

        using uVec_t = glm::vec<D, glm::u32>;
        using iVec_t = glm::vec<D, glm::i32>;
        using fVec_t = glm::vec<D, glm::f32>;

        using uBox_t = Math::Box<D, glm::u32>;
        using iBox_t = Math::Box<D, glm::i32>;

        static const glm::length_t NDimensions = D;


        //Creates a new texture.
        //Pass "1" for nMipLevels to not use mip-maps.
        //Pass "0" for nMipLevels to generate full mip-maps down to a single pixel.
        //Pass anything else to generate a fixed amount of mip levels.
        TextureD(const uVec_t& _size, Format _format,
                 uint_mipLevel_t _nMipLevels = 0)
            : size(_size),
              Texture(GetClassType(), _format,
                      (_nMipLevels < 1) ? GetMaxNumbMipmaps(_size) : _nMipLevels)
        {
            //Allocate GPU storage.
            if constexpr (D == 1) {
                glTextureStorage1D(glPtr.Get(), nMipLevels, format.GetOglEnum(), size.x);
            } else if constexpr (D == 2) {
                glTextureStorage2D(glPtr.Get(), nMipLevels, format.GetOglEnum(), size.x, size.y);
            } else if constexpr (D == 3) {
                glTextureStorage3D(glPtr.Get(), nMipLevels, format.GetOglEnum(), size.x, size.y, size.z);
            } else {
                static_assert(false, "TextureD<> should only be 1-, 2-, or 3-dimensional");
            }

            //Load the initial sampler data.
            GLint p;
            glGetTextureParameteriv(glPtr.Get(), GL_TEXTURE_MIN_FILTER, &p);
            sampler.MinFilter = TextureMinFilters::_from_integral(p);
            glGetTextureParameteriv(glPtr.Get(), GL_TEXTURE_MAG_FILTER, &p);
            sampler.MagFilter = TextureMagFilters::_from_integral(p);
            const std::array<GLenum, 3> wrapEnums = {
                GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R
            };
            for (glm::length_t d = 0; d < D; ++d)
            {
                glGetTextureParameteriv(glPtr.Get(), wrapEnums[d], &p);
                sampler.Wrapping[d] = TextureWrapping::_from_integral(p);
            }
        }
        virtual ~TextureD()
        {
            if (glPtr != OglPtr::Texture::Null)
                glDeleteTextures(1, &glPtr.Get());
        }

        //Note that the copy constructor/operator is automatically deleted via the parent class.

        TextureD(TextureD&& src)
            : Texture(std::move(src)), size(src.size), sampler(src.sampler) { }
        TextureD& operator=(TextureD&& src)
        {
            //Destruct this instance, then move-construct it.
            this->~TextureD<D>();
            new (this) TextureD<D>(std::move(src));

            return *this;
        }


        uVec_t GetSize(uint_mipLevel_t mipLevel = 0) const
        {
            auto _size = size;
            for (uint_mipLevel_t i = 0; i < mipLevel; ++i)
                _size = glm::max(_size / uVec_t{ 2 },
                                 uVec_t{ 1 });
            return _size;
        }

        //Gets the number of bytes needed to store this texture in its native format.
        size_t GetByteSize(uint_mipLevel_t mipLevel = 0) const
        {
            return format.GetByteSize(GetSize(mipLevel));
        }
        //Gets the total byte-size of this texture, across all mip levels.
        size_t GetTotalByteSize() const
        {
            size_t sum = 0;
            for (uint_mipLevel_t mip = 0; mip < nMipLevels; ++mip)
                sum += GetByteSize(mip);
            return sum;
        }

        Sampler<D> GetSampler() const { return sampler; }
        void SetSampler(const Sampler<D>& s)
        {
            sampler = s;

            glTextureParameteri(glPtr.Get(), GL_TEXTURE_MIN_FILTER,
                                (GLint)sampler.MinFilter);
            glTextureParameteri(glPtr.Get(), GL_TEXTURE_MAG_FILTER,
                                (GLint)sampler.MagFilter);

            const std::array<GLenum, 3> wrapEnums = {
                GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R
            };
            for (glm::length_t d = 0; d < D; ++d)
                glTextureParameteri(glPtr.Get(), wrapEnums[d], (GLint)sampler.Wrapping[d]);
        }


        //TODO: How to implement setting/getting of depth and/or stencil? Refer to https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml, plus page 194 of https://www.khronos.org/registry/OpenGL/specs/gl/glspec45.core.pdf

        #pragma region Setting data

        //Optional parameters when uploading texture data.
        struct SetDataParams
        {
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

        //Clears part or all of this texture to the given value.
        //Not allowed for compressed-format textures.
        template<glm::length_t L, typename T>
        void Clear(const glm::vec<L, T> value,
                   SetDataParams optionalParams = { },
                   bool bgrOrdering = false)
        {
            BPAssert(!format.IsCompressed(), "Can't clear a compressed texture!");

            auto range3D = optionalParams.GetRange(size).ChangeDimensions<3>();
            glClearTexSubImage(glPtr.Get(), optionalParams.MipLevel,
                               range3D.MinCorner.x, range3D.MinCorner.y, range3D.MinCorner.z,
                               range3D.Size.x, range3D.Size.y, range3D.Size.z,
                               GetOglInputFormat<T>(),
                               GetOglChannels(GetComponents<L>(bgrOrdering)),
                               glm::value_ptr(value));
        }


        //Note that pixel data in OpenGL is ordered from left to right,
        //    then from bottom to top, then from back to front.
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
        //Note that, because Block-Compression works in square "blocks" of pixels,
        //    the destination rectangle is in units of blocks, not individual pixels.
        //Additionally, mipmaps cannot be regenerated automatically.
        void SetData_Compressed(const std::byte* compressedData,
                                uint_mipLevel_t mipLevel = 0,
                                uBox_t destBlockRange = { })
        {
            //Convert block range to pixel size.
            auto blockSize = GetBlockSize(format.AsCompressed());
            auto destPixelRange = uBox_t::MakeMinSize(destBlockRange.MinCorner * blockSize,
                                                      destBlockRange.Size * blockSize);

            //Process default arguments.
            if (glm::all(glm::equal(destPixelRange.Size, uVec_t{ 0 })))
                destPixelRange = uBox_t::MakeMinSize(uVec_t{ 0 }, size);

            BPAssert(glm::all(glm::lessThan(destPixelRange.GetMaxCorner(), size)),
                     "Block range goes beyond the texture's size");

            //Upload.
            if constexpr (D == 1) {
                glCompressedTextureSubImage1D(glPtr.Get(), mipLevel,
                                              destPixelRange.MinCorner.x, destPixelRange.Size.x,
                                              format.GetOglEnum(),
                                              (GLsizei)format.GetByteSize(GetSize(mipLevel)),
                                              compressedData);
            } else if constexpr (D == 2) {
                glCompressedTextureSubImage2D(glPtr.Get(), mipLevel,
                                              destPixelRange.MinCorner.x, destPixelRange.MinCorner.y,
                                              destPixelRange.Size.x, destPixelRange.Size.y,
                                              format.GetOglEnum(),
                                              (GLsizei)format.GetByteSize(GetSize(mipLevel)),
                                              compressedData);
            } else if constexpr (D == 3) {
                glCompressedTextureSubImage3D(glPtr.Get(), mipLevel,
                                              destPixelRange.MinCorner.x, destPixelRange.MinCorner.y, destPixelRange.MinCorner.z,
                                              destPixelRange.Size.x, destPixelRange.Size.y, destPixelRange.Size.z,
                                              format.GetOglEnum(),
                                              (GLsizei)format.GetByteSize(GetSize(mipLevel)),
                                              compressedData);
            } else {
                static_assert(false, "TextureD<> should only be 1-, 2-, or 3-dimensional");
            }
        }

        #pragma endregion
        
        #pragma region Getting data

        //Optional parameters when downloading texture data.
        struct GetDataParams
        {
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
        void GetData_Compressed(std::byte* compressedData,
                                uBox_t blockRange = { },
                                uint_mipLevel_t mipLevel = 0) const
        {
            //Convert block range to pixel size.
            auto blockSize = GetBlockSize(format.AsCompressed());
            auto pixelRange = uBox_t::MakeMinSize(blockRange.MinCorner * blockSize,
                                                  blockRange.Size * blockSize);

            //Process default arguments.
            if (glm::all(glm::equal(pixelRange.Size, uVec_t{ 0 })))
                pixelRange = uBox_t::MakeMinSize(uVec_t{ 0 }, size);

            BPAssert(glm::all(glm::lessThan(pixelRange.GetMaxCorner(), size)),
                     "Block range goes beyond the texture's size");

            //Download.
            auto range3D = pixelRange.ChangeDimensions<3>();
            glGetCompressedTextureSubImage(glPtr.Get(), mipLevel,
                                           range3D.MinCorner.x, range3D.MinCorner.y, range3D.MinCorner.z,
                                           range3D.Size.x, pixelRange.Size.y, range3D.Size.z,
                                           format.GetByteSize(GetSize(mipLevel)),
                                           compressedData);
        }

        #pragma endregion


    protected:

        Sampler<D> sampler;
        uVec_t size;
        

        void SetData(const void* data,
                     GLenum dataFormat, GLenum dataType,
                     const SetDataParams& params)
        {
            //Process default arguments.
            auto destRange = params.DestRange;
            if (glm::all(glm::equal(destRange.Size, uVec_t{ 0 })))
                destRange = uBox_t::MakeMinSize(uVec_t{ 0 }, size);

            //Upload.
            if constexpr (D == 1) {
                glTextureSubImage1D(glPtr.Get(), params.MipLevel,
                                    destRange.MinCorner.x, destRange.Size.x,
                                    dataFormat, dataType, data);
            } else if constexpr (D == 2) {
                glTextureSubImage2D(glPtr.Get(), params.MipLevel,
                                    destRange.MinCorner.x, destRange.MinCorner.y,
                                    destRange.Size.x, destRange.Size.y,
                                    dataFormat, dataType, data);
            } else if constexpr (D == 3) {
                glTextureSubImage3D(glPtr.Get(), params.MipLevel,
                                    destRange.MinCorner.x, destRange.MinCorner.y, destRange.MinCorner.z,
                                    destRange.Size.x, destRange.Size.y, destRange.Size.z,
                                    dataFormat, dataType, data);
            } else {
                static_assert(false, "TextureD<> should only be 1-, 2-, or 3-dimensional");
            }
        }
        void GetData(void* data,
                     GLenum dataFormat, GLenum dataType,
                     const GetDataParams& params) const
        {
            //Process default arguments.
            auto range = params.Range;
            if (glm::all(glm::equal(range.Size, uVec_t{ 0 })))
                range = uBox_t::MakeMinSize(uVec_t{ 0 }, size);

            //Download.
            auto range3D = range.ChangeDimensions<3>();
            glGetTextureSubImage(glPtr.Get(), params.MipLevel,
                                 range3D.MinCorner.x, range3D.MinCorner.y, range3D.MinCorner.z,
                                 range3D.Size.x, range3D.Size.y, range3D.Size.z,
                                 dataFormat, dataType,
                                 (GLsizei)GetByteSize(params.MipLevel),
                                 data);
        }
    };

    using Texture1D = TextureD<1>;
    using Texture2D = TextureD<2>;
    using Texture3D = TextureD<3>;
}