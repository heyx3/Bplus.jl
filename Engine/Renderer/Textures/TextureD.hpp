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


        //TODO: Pull SetDataParams into a template class above Texture.
        //TODO: Move Clear functions into their own region.
        //TODO: Move the private Clear/Set/Get functions into their respective pragma regions.
        //TODO: Finish Get and Set based on the changes made to Clear.

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

        //Clears part or all of this color texture to the given value.
        //Not allowed for compressed-format textures.
        template<glm::length_t L, typename T>
        void Clear_Color(const glm::vec<L, T>& value,
                         SetDataParams optionalParams = { },
                         bool bgrOrdering = false)
        {
            BPAssert(!format.IsCompressed(), "Can't clear a compressed texture!");
            BPAssert(!format.IsDepthStencil(), "Can't clear a depth/stencil texture with `Clear_Color()`!");
            ClearData(glm::value_ptr(value),
                      GetOglChannels(GetComponents<L>(bgrOrdering)),
                      GetOglInputFormat<T>(),
                      optionalParams);
        }

        //Clears part or all of this depth texture to the given value.
        template<typename T>
        void Clear_Depth(T depth, SetDataParams optionalParams = { })
        {
            BPAssert(format.IsDepthOnly(),
                     "Trying to clear depth value in a color, stencil, or depth-stencil texture");
            ClearData(&depth, GL_DEPTH_COMPONENT, GetOglInputFormat<T>(),
                      optionalParams);
        }

        //Clears part or all of this stencil texture to the given value.
        void Clear_Stencil(uint8_t stencil, SetDataParams optionalParams = { })
        {
            BPAssert(format.IsStencilOnly(),
                     "Trying to clear the stencil value in a color, depth, or depth-stencil texture");
            ClearData(&stencil,
                      GL_STENCIL_INDEX, GetOglInputFormat<decltype(stencil)>(),
                      optionalParams);
        }

        //Clears part or all of this depth/stencil hybrid texture.
        //Must use the format Depth24U_Stencil8.
        void Clear_DepthStencil(Unpacked_Depth24uStencil8u value,
                                SetDataParams optionalParams = { })
        {
            BPAssert(format == +DepthStencilFormats::Depth24U_Stencil8,
                     "Trying to clear depth/stencil texture with 24U depth, but it doesn't have 24U depth");
            
            auto packed = Pack_DepthStencil(value);
            ClearData(&packed,
                      GL_STENCIL_INDEX, GL_UNSIGNED_INT_24_8,
                      optionalParams);
        }
        //Clears part of all of this depth/stencil hybrid texture.
        //Must use the format Depth32F_Stencil8.
        void Clear_DepthStencil(float depth, uint8_t stencil,
                                SetDataParams optionalParams = { })
        {
            BPAssert(format == +DepthStencilFormats::Depth32F_Stencil8,
                     "Trying to clear depth/stencil texture with 32F depth, but it doesn't have 32F depth");

            auto packed = Pack_DepthStencil(Unpacked_Depth32fStencil8u{ depth, stencil });
            ClearData(&packed,
                      GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV,
                      optionalParams);
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
            auto byteSize = (GLsizei)(format.GetByteSize(GetSize(mipLevel)) * destPixelRange.GetVolume());
            if constexpr (D == 1) {
                glCompressedTextureSubImage1D(glPtr.Get(), mipLevel,
                                              destPixelRange.MinCorner.x, destPixelRange.Size.x,
                                              format.GetOglEnum(),
                                              byteSize, compressedData);
            } else if constexpr (D == 2) {
                glCompressedTextureSubImage2D(glPtr.Get(), mipLevel,
                                              destPixelRange.MinCorner.x, destPixelRange.MinCorner.y,
                                              destPixelRange.Size.x, destPixelRange.Size.y,
                                              format.GetOglEnum(),
                                              byteSize, compressedData);
            } else if constexpr (D == 3) {
                glCompressedTextureSubImage3D(glPtr.Get(), mipLevel,
                                              destPixelRange.MinCorner.x, destPixelRange.MinCorner.y, destPixelRange.MinCorner.z,
                                              destPixelRange.Size.x, destPixelRange.Size.y, destPixelRange.Size.z,
                                              format.GetOglEnum(),
                                              byteSize, compressedData);
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

            uBox_t GetRange(const uVec_t& fullSize) const
            {
                if (glm::all(glm::equal(Range.Size, uVec_t{ 0 })))
                    return uBox_t::MakeMinSize(uVec_t{ 0 }, fullSize);
                else
                    return Range;
            }
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
                                           (GLsizei)(format.GetByteSize(GetSize(mipLevel)) * range3D.GetVolume()),
                                           compressedData);
        }

        #pragma endregion


    protected:

        Sampler<D> sampler;
        uVec_t size;
        

        void SetData(const void* data,
                     GLenum dataChannels, GLenum dataType,
                     const SetDataParams& params)
        {
            auto range = params.GetRange(GetSize(params.MipLevel));

            //Upload.
            if constexpr (D == 1) {
                glTextureSubImage1D(glPtr.Get(), params.MipLevel,
                                    range.MinCorner.x, range.Size.x,
                                    dataChannels, dataType, data);
            } else if constexpr (D == 2) {
                glTextureSubImage2D(glPtr.Get(), params.MipLevel,
                                    range.MinCorner.x, range.MinCorner.y,
                                    range.Size.x, range.Size.y,
                                    dataChannels, dataType, data);
            } else if constexpr (D == 3) {
                glTextureSubImage3D(glPtr.Get(), params.MipLevel,
                                    range.MinCorner.x, range.MinCorner.y, range.MinCorner.z,
                                    range.Size.x, range.Size.y, range.Size.z,
                                    dataChannels, dataType, data);
            } else {
                static_assert(false, "TextureD<> should only be 1-, 2-, or 3-dimensional");
            }

            //Recompute mips if requested.
            if (params.RecomputeMips)
                RecomputeMips();
        }
        void GetData(void* data,
                     GLenum dataChannels, GLenum dataType,
                     const GetDataParams& params) const
        {
            auto range3D = params.GetRange(GetSize(params.MipLevel))
                                 .ChangeDimensions<3>();

            glGetTextureSubImage(glPtr.Get(), params.MipLevel,
                                 range3D.MinCorner.x, range3D.MinCorner.y, range3D.MinCorner.z,
                                 range3D.Size.x, range3D.Size.y, range3D.Size.z,
                                 dataChannels, dataType,
                                 (GLsizei)(GetByteSize(params.MipLevel) * range3D.GetVolume()),
                                 data);
        }
        void ClearData(void* clearValue, GLenum valueFormat, GLenum valueType,
                       const SetDataParams& params)
        {
            auto fullSize = GetSize(params.MipLevel);
            auto range = params.GetRange(size);
            auto range3D = range.ChangeDimensions<3>();

            glClearTexSubImage(glPtr.Get(), params.MipLevel,
                               range3D.MinCorner.x, range3D.MinCorner.y, range3D.MinCorner.z,
                               range3D.Size.x, range3D.Size.y, range3D.Size.z,
                               valueFormat, valueType, clearValue);

            if (params.RecomputeMips)
            {
                //If we've cleared the entire texture, skip mipmap generation
                //    and just clear all smaller mips.
                if (range.Size == fullSize)
                {
                    for (uint_mipLevel_t mipI = params.MipLevel + 1; mipI < GetNMipLevels(); ++mipI)
                    {
                        auto mipFullSize = GetSize(mipI);
                        glm::uvec3 mipFullSize3D;
                        if constexpr (D == 1) {
                            mipFullSize3D = { mipFullSize.x, 1, 1 };
                        } else if constexpr (D == 2) {
                            mipFullSize3D = { mipFullSize, 1 };
                        } else if constexpr (D == 3) {
                            mipFullSize3D = mipFullSize;
                        } else {
                            static_assert(false, "Only supports 1D-3D textures in ClearData()");
                        }

                        glClearTexSubImage(glPtr.Get(), mipI, 0, 0, 0,
                                           mipFullSize3D.x, mipFullSize3D.y, mipFullSize3D.z,
                                           valueFormat, valueType, clearValue);
                    }
                }
                //Otherwise, do the usual mipmap update.
                else
                {
                    RecomputeMips();
                }
            }
        }
    };

    using Texture1D = TextureD<1>;
    using Texture2D = TextureD<2>;
    using Texture3D = TextureD<3>;
}