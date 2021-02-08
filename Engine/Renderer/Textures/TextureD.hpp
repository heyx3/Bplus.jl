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
                 uint_mipLevel_t _nMipLevels = 0,
                 Sampler<D> sampler = { })
            : size(_size),
              Texture(GetClassType(), _format,
                      (_nMipLevels < 1) ? GetMaxNumbMipmaps(_size) : _nMipLevels,
                      sampler.ChangeDimensions<3>())
        {
            //Depth and stencil textures are not supported on 3D textures.
            if constexpr (D == 3)
            {
                BPAssert(!GetFormat().IsDepthStencil(),
                         "3D textures cannot use a depth/stencil format");
            }

            //Allocate GPU storage.
            if constexpr (D == 1) {
                glTextureStorage1D(GetOglPtr().Get(), GetNMipLevels(), GetFormat().GetOglEnum(), size.x);
            } else if constexpr (D == 2) {
                glTextureStorage2D(GetOglPtr().Get(), GetNMipLevels(), GetFormat().GetOglEnum(), size.x, size.y);
            } else if constexpr (D == 3) {
                glTextureStorage3D(GetOglPtr().Get(), GetNMipLevels(), GetFormat().GetOglEnum(), size.x, size.y, size.z);
            } else {
                static_assert(false, "TextureD<> should only be 1-, 2-, or 3-dimensional");
            }
        }

        //Note that the copy constructor/operator is automatically deleted via the parent class.

        TextureD(TextureD<D>&& src)
            : Texture(std::move(src)),
              size(src.size) { }
        TextureD& operator=(TextureD<D>&& src)
        {
            //Call deconstructor, then move constructor.
            //Only bother changing things if they represent different handles.
            if (this != &src)
            {
                this->~TextureD<D>();
                new (this) TextureD<D>(std::move(src));
            }

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
        size_t GetByteSize(uint_mipLevel_t mipLevel = 0) const override
        {
            return GetFormat().GetByteSize(GetSize(mipLevel));
        }

        //Gets (or creates) a view of this texture with the given sampler.
        TexView GetView(std::optional<Sampler<D>> customSampler = std::nullopt) const
        {
            return GetViewFull(customSampler.has_value() ?
                               std::make_optional(customSampler.value().ChangeDimensions<3>()) :
                               std::nullopt);
        }

        const Sampler<D>& GetSampler() const { return GetSamplerFull().ChangeDimensions<D>(); }


        #pragma region Clearing data

        //Clears part or all of this color texture to the given value.
        //Not allowed for compressed-format textures.
        template<glm::length_t L, typename T>
        void Clear_Color(const glm::vec<L, T>& value,
                         SetDataParams<D> optionalParams = { },
                         bool bgrOrdering = false)
        {
            BPAssert(!GetFormat().IsCompressed(), "Can't clear a compressed texture!");
            BPAssert(!GetFormat().IsDepthStencil(),
                     "Can't clear a depth/stencil texture with `Clear_Color()`!");
            if constexpr (!std::is_integral_v<T>)
                BPAssert(!GetFormat().IsInteger(), "Can't clear an integer texture to a non-integer value");

            ClearData(glm::value_ptr(value),
                      GetOglChannels(GetComponents<L>(bgrOrdering)),
                      GetOglInputFormat<T>(),
                      optionalParams);
        }

        //Note that you can't clear compressed textures, though you can set their pixels with Set_Compressed.

        //Clears part or all of this depth texture to the given value.
        template<typename T>
        void Clear_Depth(T depth, SetDataParams<D> optionalParams = { })
        {
            BPAssert(GetFormat().IsDepthOnly(),
                     "Trying to clear depth value in a color, stencil, or depth-stencil texture");

            ClearData(&depth, GL_DEPTH_COMPONENT, GetOglInputFormat<T>(),
                      optionalParams);
        }

        //Clears part or all of this stencil texture to the given value.
        void Clear_Stencil(uint8_t stencil, SetDataParams<D> optionalParams = { })
        {
            BPAssert(GetFormat().IsStencilOnly(),
                     "Trying to clear the stencil value in a color, depth, or depth-stencil texture");

            ClearData(&stencil,
                      GL_STENCIL_INDEX, GetOglInputFormat<uint8_t>(),
                      optionalParams);
        }

        //Clears part or all of this depth/stencil hybrid texture.
        //Must use the format Depth24U_Stencil8.
        void Clear_DepthStencil(Unpacked_Depth24uStencil8u value,
                                SetDataParams<D> optionalParams = { })
        {
            BPAssert(GetFormat() == +DepthStencilFormats::Depth24U_Stencil8,
                     "Trying to clear depth/stencil texture with 24U depth, but it doesn't have 24U depth");
            
            auto packed = Pack_DepthStencil(value);
            ClearData(&packed,
                      GL_STENCIL_INDEX, GL_UNSIGNED_INT_24_8,
                      optionalParams);
        }
        //Clears part of all of this depth/stencil hybrid texture.
        //Must use the format Depth32F_Stencil8.
        void Clear_DepthStencil(float depth, uint8_t stencil,
                                SetDataParams<D> optionalParams = { })
        {
            BPAssert(GetFormat() == +DepthStencilFormats::Depth32F_Stencil8,
                     "Trying to clear depth/stencil texture with 32F depth, but it doesn't have 32F depth");

            auto packed = Pack_DepthStencil(Unpacked_Depth32fStencil8u(depth, stencil));
            ClearData(&packed,
                      GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV,
                      optionalParams);
        }


        //The implementation for clearing any kind of data:
    private:
        void ClearData(void* clearValue, GLenum valueFormat, GLenum valueType,
                       const SetDataParams<D>& params)
        {
            auto fullSize = GetSize(params.MipLevel);
            auto range = params.GetRange(fullSize);
            auto range3D = range.ChangeDimensions<3>();

            glClearTexSubImage(GetOglPtr().Get(), params.MipLevel,
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

                        glClearTexSubImage(GetOglPtr().Get(), mipI, 0, 0, 0,
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
    public:

        #pragma endregion


        //Note that pixel data in OpenGL is ordered from left to right,
        //    then from bottom to top, then from back to front.
        //In other words, rows are contiguous and then grouped vertically.

        #pragma region Setting data

        //Sets this color texture with the given data.
        //Not allowed for compressed-format textures.
        //If the texture's format is an Integer-type, then the input data must be as well.
        //Note that the upload to the GPU will be slower if
        //    the data doesn't exactly match the texture's pixel format.
        template<typename T>
        void Set_Color(const T* data, PixelIOChannels components,
                       SetDataParams<D> optionalParams = { })
        {
            //Note that the OpenGL standard does allow you to set compressed textures
            //    with normal RGBA values, but the implementation qualities vary widely
            //    so we choose not to allow it.
            BPAssert(!GetFormat().IsCompressed(),
                     "Can't set a compressed texture with Set_Color()! Use Set_Compressed()");

            BPAssert(!GetFormat().IsDepthStencil(),
                     "Can't set a depth/stencil texture with Set_Color()!");
            if constexpr (!std::is_integral_v<T>)
                BPAssert(!GetFormat().IsInteger(), "Can't set an integer texture with non-integer data");

            SetData((const void*)data,
                    GetOglChannels(components), GetOglInputFormat<T>(),
                    optionalParams);
        }

        //Sets any kind of color texture with the given data.
        //Not allowed for compressed-format textures.
        //If the texture's format is an Integer-type, then the input data must be as well.
        //The number of components is determined by the size of the vector type --
        //    1D is Red/Depth, 2D is Red-Green, 3D is RGB, 4D is RGBA.
        //If "bgrOrdering" is true, then the incoming RGB(A) data is actually in BGR(A) order.
        //BGR order is often more efficient to give to the GPU.
        template<glm::length_t L, typename T>
        void Set_Color(const glm::vec<L, T>* pixels,
                       bool bgrOrdering = false,
                       SetDataParams<D> optionalParams = { })
        {
            Set_Color<T>(glm::value_ptr(*pixels),
                         GetComponents<L>(bgrOrdering),
                         optionalParams);
        }

        //Directly sets block-compressed data for the texture,
        //    based on its current format.
        //The input data should have as many bytes as "GetFormat().GetByteSize(pixelsRange)".
        //Note that, because Block-Compression works in square "blocks" of pixels,
        //    the destination rectangle is in units of blocks, not individual pixels.
        //Additionally, mipmaps cannot be regenerated automatically.
        void Set_Compressed(const std::byte* compressedData,
                            uint_mipLevel_t mipLevel = 0,
                            uBox_t destBlockRange = { })
        {
            //Convert block range to pixel size.
            auto texSize = GetSize(mipLevel);
            auto blockSize = GetBlockSize(GetFormat().AsCompressed());
            auto destPixelRange = uBox_t::MakeMinSize(destBlockRange.MinCorner * blockSize,
                                                      destBlockRange.Size * blockSize);

            //Process default arguments.
            if (glm::all(glm::equal(destPixelRange.Size, uVec_t{ 0 })))
                destPixelRange = uBox_t::MakeMinSize(uVec_t{ 0 }, texSize);

            BPAssert(glm::all(glm::lessThanEqual(destPixelRange.GetMaxCorner(), texSize)),
                     "Block range goes beyond the texture's size");

            //Upload.
            auto byteSize = (GLsizei)GetFormat().GetByteSize(destPixelRange.Size);
            if constexpr (D == 1) {
                glCompressedTextureSubImage1D(GetOglPtr().Get(), mipLevel,
                                              destPixelRange.MinCorner.x, destPixelRange.Size.x,
                                              GetFormat().GetOglEnum(),
                                              byteSize, compressedData);
            } else if constexpr (D == 2) {
                glCompressedTextureSubImage2D(GetOglPtr().Get(), mipLevel,
                                              destPixelRange.MinCorner.x, destPixelRange.MinCorner.y,
                                              destPixelRange.Size.x, destPixelRange.Size.y,
                                              GetFormat().GetOglEnum(),
                                              byteSize, compressedData);
            } else if constexpr (D == 3) {
                glCompressedTextureSubImage3D(GetOglPtr().Get(), mipLevel,
                                              destPixelRange.MinCorner.x, destPixelRange.MinCorner.y, destPixelRange.MinCorner.z,
                                              destPixelRange.Size.x, destPixelRange.Size.y, destPixelRange.Size.z,
                                              GetFormat().GetOglEnum(),
                                              byteSize, compressedData);
            } else {
                static_assert(false, "TextureD<> should only be 1-, 2-, or 3-dimensional");
            }
        }

        //Sets part or all of this depth texture to the given value.
        template<typename T>
        void Set_Depth(const T* pixels, SetDataParams<D> optionalParams = { })
        {
            BPAssert(GetFormat().IsDepthOnly(),
                     "Trying to set depth data for a non-depth texture");

            SetData(&pixels, GL_DEPTH_COMPONENT, GetOglInputFormat<T>(),
                    optionalParams);
        }

        //Sets part or all of this stencil texture to the given value.
        void Set_Stencil(const uint8_t* pixels, SetDataParams<D> optionalParams = { })
        {
            BPAssert(GetFormat().IsStencilOnly(),
                     "Trying to set the stencil values in a color, depth, or depth-stencil texture");

            SetData(&pixels,
                    GL_STENCIL_INDEX, GetOglInputFormat<uint8_t>(),
                    optionalParams);
        }

        
        //Sets part or all of this depth/stencil hybrid texture.
        //Must be using the format Depth24U_Stencil8.
        void Set_DepthStencil(const uint32_t* packedPixels_Depth24uStencil8u,
                              SetDataParams<D> optionalParams = { })
        {
            BPAssert(GetFormat() == +DepthStencilFormats::Depth24U_Stencil8,
                     "Trying to set depth/stencil texture with a 24U depth, but it doesn't use 24U depth");
            
            SetData(&packedPixels_Depth24uStencil8u,
                    GL_STENCIL_INDEX, GL_UNSIGNED_INT_24_8,
                    optionalParams);
        }
        //Sets part of all of this depth/stencil hybrid texture.
        //Must be using the format Depth32F_Stencil8.
        void Set_DepthStencil(const uint64_t* packedPixels_Depth32fStencil8u,
                              SetDataParams<D> optionalParams = { })
        {
            BPAssert(GetFormat() == +DepthStencilFormats::Depth32F_Stencil8,
                     "Trying to set depth/stencil texture with a 32F depth, but it doesn't use 32F depth");

            SetData(&packedPixels_Depth32fStencil8u,
                    GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV,
                    optionalParams);
        }


        //The implementation for setting any kind of data:
    private:
        void SetData(const void* data,
                     GLenum dataChannels, GLenum dataType,
                     const SetDataParams<D>& params)
        {
            auto sizeAtMip = GetSize(params.MipLevel);
            auto range = params.GetRange(sizeAtMip);

            for (glm::length_t d = 0; d < D; ++d)
                BPAssert(range.GetMaxCornerInclusive()[d] < sizeAtMip[d],
                         "GetData() call would go past the texture bounds");

            //Note that B+ always uses tighty-packed byte data --
            //    no padding between pixels or rows.

            //Upload.
            if constexpr (D == 1) {
                glTextureSubImage1D(GetOglPtr().Get(), params.MipLevel,
                                    range.MinCorner.x, range.Size.x,
                                    dataChannels, dataType, data);
            } else if constexpr (D == 2) {
                glTextureSubImage2D(GetOglPtr().Get(), params.MipLevel,
                                    range.MinCorner.x, range.MinCorner.y,
                                    range.Size.x, range.Size.y,
                                    dataChannels, dataType, data);
            } else if constexpr (D == 3) {
                glTextureSubImage3D(GetOglPtr().Get(), params.MipLevel,
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
    public:

        #pragma endregion
        
        #pragma region Getting data

        //Gets any kind of color texture data, writing it into the given buffer.
        template<typename T>
        void Get_Color(T* data, PixelIOChannels components,
                       GetDataParams<D> optionalParams = { }) const
        {
            BPAssert(!GetFormat().IsDepthStencil(),
                     "Can't read a depth/stencil texture with Get_Color()!");
            if constexpr (!std::is_integral_v<T>)
                BPAssert(!GetFormat().IsInteger(), "Can't read an integer texture as non-integer data");

            GetData((void*)data,
                    sizeof(decltype(data[0])) * GetNChannels(components),
                    GetOglChannels(components), GetOglInputFormat<T>(),
                    optionalParams);
        }

        //Gets any kind of color texture data, writing it into the given buffer.
        //1D data is interpreted as R channel (or depth values),
        //    2D as RG, 3D as RGB, and 4D as RGBA.
        //If "bgrOrdering" is true, then the incoming RGB(A) data is actually in BGR(A) order.
        template<glm::length_t L, typename T>
        void Get_Color(glm::vec<L, T>* pixels, bool bgrOrdering = false,
                       GetDataParams<D> optionalParams = { }) const
        {
            Get_Color<T>(glm::value_ptr(*pixels),
                         GetComponents<L>(bgrOrdering),
                         optionalParams);
        }

        //Directly reads block-compressed data from the texture,
        //    based on its current format.
        //This is a fast, direct copy of the byte data stored in the texture.
        //The output data should have as many bytes as "GetFormat().GetByteSize(pixelsRange)".
        //Note that, because Block-Compression works in square groups of pixels,
        //    the "range" rectangle is in units of blocks, not individual pixels.
        void Get_Compressed(std::byte* compressedData,
                            uBox_t blockRange = { },
                            uint_mipLevel_t mipLevel = 0) const
        {
            //Convert block range to pixel size.
            auto texSize = GetSize(mipLevel);
            auto blockSize = GetBlockSize(GetFormat().AsCompressed());
            auto pixelRange = uBox_t::MakeMinSize(blockRange.MinCorner * blockSize,
                                                  blockRange.Size * blockSize);

            //Process default arguments.
            if (glm::all(glm::equal(pixelRange.Size, uVec_t{ 0 })))
                pixelRange = uBox_t::MakeMinSize(uVec_t{ 0 }, texSize);

            BPAssert(glm::all(glm::lessThan(pixelRange.GetMaxCornerInclusive(), texSize)),
                     "Block range goes beyond the texture's size");

            //Download.
            auto range3D = pixelRange.ChangeDimensions<3>();
            auto byteSize = (GLsizei)GetFormat().GetByteSize(pixelRange.Size);
            glGetCompressedTextureSubImage(GetOglPtr().Get(), mipLevel,
                                           range3D.MinCorner.x, range3D.MinCorner.y, range3D.MinCorner.z,
                                           range3D.Size.x, pixelRange.Size.y, range3D.Size.z,
                                           byteSize, compressedData);
        }
        
        //Gets part or all of this depth texture, writing it into the given buffer.
        template<typename T>
        void Get_Depth(T* pixels, GetDataParams<D> optionalParams = { })
        {
            BPAssert(GetFormat().IsDepthOnly(),
                     "Trying to get depth data for a non-depth texture");

            GetData((void*)pixels, sizeof(decltype(pixels[0])),
                    GL_DEPTH_COMPONENT, GetOglInputFormat<T>(),
                    optionalParams);
        }

        //Gets part or all of this stencil texture to the given value.
        void Get_Stencil(uint8_t* pixels, GetDataParams<D> optionalParams = { })
        {
            BPAssert(GetFormat().IsStencilOnly(),
                     "Trying to get the stencil values in a color, depth, or depth-stencil texture");

            GetData((void*)pixels, sizeof(decltype(pixels[0])),
                    GL_STENCIL_INDEX, GetOglInputFormat<uint8_t>(),
                    optionalParams);
        }

        
        //Gets part or all of this depth/stencil hybrid texture, writing it into the given buffer.
        //Must be using the format Depth24U_Stencil8.
        void Get_DepthStencil(uint32_t* packedPixels_Depth24uStencil8u,
                              GetDataParams<D> optionalParams = { })
        {
            BPAssert(GetFormat() == +DepthStencilFormats::Depth24U_Stencil8,
                     "Trying to set depth/stencil texture with a 24U depth, but it doesn't use 24U depth");
            
            GetData((void*)packedPixels_Depth24uStencil8u,
                    sizeof(decltype(packedPixels_Depth24uStencil8u[0])),
                    GL_STENCIL_INDEX, GL_UNSIGNED_INT_24_8,
                    optionalParams);
        }
        //Sets part of all of this depth/stencil hybrid texture, writing it into the given buffer.
        //Must be using the format Depth32F_Stencil8.
        void Get_DepthStencil(uint64_t* packedPixels_Depth32fStencil8u,
                              GetDataParams<D> optionalParams = { })
        {
            BPAssert(GetFormat() == +DepthStencilFormats::Depth32F_Stencil8,
                     "Trying to get depth/stencil texture with a 32F depth, but it doesn't use 32F depth");

            GetData((void*)packedPixels_Depth32fStencil8u,
                    sizeof(decltype(packedPixels_Depth32fStencil8u[0])),
                    GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV,
                    optionalParams);
        }


        //The implementation for reading any kind of data:
    private:
        void GetData(void* data, size_t dataPixelSize,
                     GLenum dataChannels, GLenum dataType,
                     const GetDataParams<D>& params) const
        {
            auto sizeAtMip = GetSize(params.MipLevel);
            auto range = params.GetRange(sizeAtMip);

            for (glm::length_t d = 0; d < D; ++d)
                BPAssert(range.GetMaxCornerInclusive()[d] < sizeAtMip[d],
                         "GetData() call would go past the texture bounds");

            auto range3D = range.ChangeDimensions<3>();
            auto byteSize = (GLsizei)(dataPixelSize * glm::compMul(range3D.Size));

            //Note that B+ always uses tighty-packed byte data --
            //    no padding between pixels or rows.

            glGetTextureSubImage(GetOglPtr().Get(), params.MipLevel,
                                 range3D.MinCorner.x, range3D.MinCorner.y, range3D.MinCorner.z,
                                 range3D.Size.x, range3D.Size.y, range3D.Size.z,
                                 dataChannels, dataType, byteSize, data);
        }
    public:

        #pragma endregion


    protected:

        uVec_t size;
    };

    using Texture1D = TextureD<1>;
    using Texture2D = TextureD<2>;
    using Texture3D = TextureD<3>;
}