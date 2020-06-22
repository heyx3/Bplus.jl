#include "TextureCube.h"

using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Textures;


TextureCube::TextureCube(const glm::uvec2& _size, Format format,
                         const Sampler<2>& sampler,
                         uint_mipLevel_t nMips)
    : Texture(Types::Cubemap, format,
              (nMips < 1) ? GetMaxNumbMipmaps(_size) : nMips,
              sampler.ChangeDimensions<3>()),
      size(_size)
{
    //Allocate GPU storage.
    glTextureStorage2D(GetOglPtr().Get(), GetNMipLevels(), GetFormat().GetOglEnum(),
                       size.x, size.y);

    //Cubemaps should always use clamping.
    BPAssert(GetSampler().GetWrapping() == +WrapModes::Clamp,
             "Only Clamp wrapping is supported for cubemap textures");
    //Make sure all cubemaps sample nicely around the edges.
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

TextureCube::TextureCube(TextureCube&& src)
    : Texture(std::move(src)),
      size(src.size)
{
}
TextureCube& TextureCube::operator=(TextureCube&& src)
{
    //Destruct this instance, then move-construct it.
    this->~TextureCube();
    new (this) TextureCube(std::move(src));

    return *this;
}


glm::uvec2 TextureCube::GetSize(uint_mipLevel_t mipLevel) const
{
    auto _size = size;
    for (uint_mipLevel_t i = 0; i < mipLevel; ++i)
        _size = glm::max(_size / glm::uvec2{ 2 },
                         glm::uvec2{ 1 });
    return _size;
}


void TextureCube::ClearData(void* clearValue,
                            GLenum valueFormat, GLenum valueType,
                            const SetDataCubeParams& params)
{
    auto fullSize = GetSize(params.MipLevel);
    auto range = params.GetRange(fullSize);
    auto range3D = params.ToRange3D(range);

    glClearTexSubImage(GetOglPtr().Get(), params.MipLevel,
                       range3D.MinCorner.x, range3D.MinCorner.y, range3D.MinCorner.z,
                       range3D.Size.x, range3D.Size.y, range3D.Size.z,
                       valueFormat, valueType, clearValue);

    //Update mips.
    if (params.RecomputeMips)
    {
        //If we've cleared the entire texture, skip mipmap generation
        //    and just clear all smaller mips.
        if (range.Size == fullSize)
        {
            for (uint_mipLevel_t mipI = params.MipLevel + 1; mipI < GetNMipLevels(); ++mipI)
            {
                auto mipFullSize = GetSize(mipI);

                glClearTexSubImage(GetOglPtr().Get(), mipI,
                                   0, 0, range3D.MinCorner.z,
                                   mipFullSize.x, mipFullSize.y, range3D.Size.z,
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
void TextureCube::SetData(const void* data,
                          GLenum dataChannels, GLenum dataType,
                          const SetDataCubeParams& params)
{
    auto sizeAtMip = GetSize(params.MipLevel);
    auto range = params.GetRange(sizeAtMip);

    for (glm::length_t d = 0; d < 2; ++d)
        BPAssert(range.GetMaxCornerInclusive()[d] < sizeAtMip[d],
                 "GetData() call would go past the texture bounds");

    auto range3D = params.ToRange3D(range);
    glTextureSubImage3D(GetOglPtr().Get(), params.MipLevel,
                        range3D.MinCorner.x, range3D.MinCorner.y, range3D.MinCorner.z,
                        range3D.Size.x, range3D.Size.y, range3D.Size.z,
                        dataChannels, dataType, data);

    //Recompute mips if requested.
    if (params.RecomputeMips)
        RecomputeMips();
}
void TextureCube::GetData(void* data,
                          GLenum dataChannels, GLenum dataType,
                          const GetDataCubeParams& params) const
{
    auto sizeAtMip = GetSize(params.MipLevel);
    auto range = params.GetRange(sizeAtMip);

    for (glm::length_t d = 0; d < 2; ++d)
    {
        BPAssert(range.GetMaxCornerInclusive()[d] < sizeAtMip[d],
                 "GetData() call would go past the texture bounds");
    }

    auto range3D = params.ToRange3D(range);
    auto byteSize = (GLsizei)GetFormat().GetByteSize(range3D.Size);
    glGetTextureSubImage(GetOglPtr().Get(), params.MipLevel,
                         range3D.MinCorner.x, range3D.MinCorner.y, range3D.MinCorner.z,
                         range3D.Size.x, range3D.Size.y, range3D.Size.z,
                         dataChannels, dataType,
                         byteSize, data);
}

void TextureCube::Set_Compressed(const std::byte* compressedData,
                                 std::optional<CubeFaces> face,
                                 Math::Box2Du destBlockRange,
                                 uint_mipLevel_t mipLevel)
{
    //Convert block range to pixel size.
    auto texSize = GetSize(mipLevel);
    auto blockSize = GetBlockSize(GetFormat().AsCompressed());
    auto destPixelRange = Math::Box2Du::MakeMinSize(destBlockRange.MinCorner * blockSize,
                                                    destBlockRange.Size * blockSize);

    //Process default arguments.
    if (glm::all(glm::equal(destPixelRange.Size, glm::uvec2{ 0 })))
        destPixelRange = Math::Box2Du::MakeSize(texSize);
    BPAssert(glm::all(glm::lessThan(destPixelRange.GetMaxCornerInclusive(), texSize)),
             "Block range goes beyond the texture's size");


    //Upload.
    auto range3D = SetDataCubeParams(face, destPixelRange, mipLevel).ToRange3D(destPixelRange);
    auto byteSize = (GLsizei)GetFormat().GetByteSize(range3D.Size);
    glCompressedTextureSubImage3D(GetOglPtr().Get(), mipLevel,
                                  range3D.MinCorner.x, range3D.MinCorner.y, range3D.MinCorner.z,
                                  range3D.Size.x, range3D.Size.y, range3D.Size.z,
                                  GetFormat().GetOglEnum(),
                                  byteSize, compressedData);
}
void TextureCube::Get_Compressed(std::byte* compressedData,
                                 std::optional<CubeFaces> face,
                                 Math::Box2Du blockRange,
                                 uint_mipLevel_t mipLevel) const
{
    //Convert block range to pixel size.
    auto texSize = GetSize(mipLevel);
    auto blockSize = GetBlockSize(GetFormat().AsCompressed());
    auto pixelRange = Math::Box2Du::MakeMinSize(blockRange.MinCorner * blockSize,
                                                blockRange.Size * blockSize);

    //Process default arguments.
    if (glm::all(glm::equal(pixelRange.Size, glm::uvec2{ 0 })))
        pixelRange = Math::Box2Du::MakeSize(texSize);
    BPAssert(glm::all(glm::lessThan(pixelRange.GetMaxCornerInclusive(), texSize)),
             "Block range goes beyond the texture's size");

    //Download.
    auto range3D = SetDataCubeParams(face, pixelRange, mipLevel).ToRange3D(pixelRange);
    auto byteSize = (GLsizei)GetFormat().GetByteSize(range3D.Size);
    glGetCompressedTextureSubImage(GetOglPtr().Get(), mipLevel,
                                   range3D.MinCorner.x, range3D.MinCorner.y, range3D.MinCorner.z,
                                   range3D.Size.x, pixelRange.Size.y, range3D.Size.z,
                                   byteSize, compressedData);
}