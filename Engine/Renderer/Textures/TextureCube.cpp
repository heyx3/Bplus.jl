#include "TextureCube.h"

using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Textures;


TextureCube::TextureCube(const glm::uvec2& _size, Format format,
                         uint_mipLevel_t nMips)
    : Texture(Types::Cubemap, format, nMips),
      size(_size)
{
    //Allocate GPU storage.
    glTextureStorage2D(glPtr.Get(), nMipLevels, format.GetOglEnum(),
                       size.x, size.y);

    //Load the initial sampler data.
    GLint p;
    glGetTextureParameteriv(glPtr.Get(), GL_TEXTURE_MIN_FILTER, &p);
    minFilter = TextureMinFilters::_from_integral(p);
    glGetTextureParameteriv(glPtr.Get(), GL_TEXTURE_MAG_FILTER, &p);
    magFilter = TextureMagFilters::_from_integral(p);

    //Cubemaps should always use clamping, and should sample nicely around edges.
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    p = (GLenum)TextureWrapping::Clamp;
    glTextureParameteriv(glPtr.Get(), GL_TEXTURE_WRAP_S, &p);
    glTextureParameteriv(glPtr.Get(), GL_TEXTURE_WRAP_T, &p);
}
TextureCube::~TextureCube()
{
    if (glPtr != OglPtr::Texture::Null)
        glDeleteTextures(1, &glPtr.Get());
}

TextureCube::TextureCube(TextureCube&& src)
    : Texture(std::move(src)), size(src.size),
      minFilter(src.minFilter), magFilter(src.magFilter)
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

size_t TextureCube::GetTotalByteSize() const
{
    size_t sum = 0;
    for (uint_mipLevel_t mip = 0; mip < nMipLevels; ++mip)
        sum += GetByteSize(mip);
    return sum;
}

void TextureCube::SetMinFilter(TextureMinFilters filter) const
{
    auto val = (GLint)filter._to_integral();
    glTextureParameteriv(glPtr.Get(), GL_TEXTURE_MIN_FILTER, &val);
}
void TextureCube::SetMagFilter(TextureMagFilters filter) const
{
    auto val = (GLint)filter._to_integral();
    glTextureParameteriv(glPtr.Get(), GL_TEXTURE_MAG_FILTER, &val);
}

void TextureCube::RecomputeMips()
{
    BPAssert(!format.IsCompressed(),
             "Can't compute mipmaps for a compressed texture!");

    glGenerateTextureMipmap(glPtr.Get());
}

void TextureCube::SetData_Compressed(CubeFaces face, const std::byte* compressedData,
                                     uint_mipLevel_t mipLevel,
                                     Math::Box2Du destBlockRange)
{
    //Convert block range to pixel size.
    auto blockSize = GetBlockSize(format.AsCompressed());
    auto destPixelRange = Math::Box2Du::MakeMinSize(destBlockRange.MinCorner * blockSize,
                                                    destBlockRange.Size * blockSize);

    //Process default arguments.
    if (glm::all(glm::equal(destPixelRange.Size, glm::uvec2{ 0 })))
        destPixelRange = Math::Box2Du::MakeMinSize(glm::uvec2{ 0 }, size);

    BPAssert(glm::all(glm::lessThan(destPixelRange.GetMaxCorner(), size)),
             "Block range goes beyond the texture's size");

    //Upload.
    glCompressedTextureSubImage3D(glPtr.Get(), mipLevel,
                                  destPixelRange.MinCorner.x, destPixelRange.MinCorner.y, (GLsizei)face._to_index(),
                                  destPixelRange.Size.x, destPixelRange.Size.y, 1,
                                  format.GetOglEnum(),
                                  (GLsizei)format.GetByteSize(GetSize(mipLevel)),
                                  compressedData);
}
void TextureCube::GetData_Compressed(CubeFaces face, std::byte* compressedData,
                                     Math::Box2Du blockRange,
                                     uint_mipLevel_t mipLevel) const
{
    //Convert block range to pixel size.
    auto blockSize = GetBlockSize(format.AsCompressed());
    auto pixelRange = Math::Box2Du::MakeMinSize(blockRange.MinCorner * blockSize,
                                                blockRange.Size * blockSize);

    //Process default arguments.
    if (glm::all(glm::equal(pixelRange.Size, glm::uvec2{ 0 })))
        pixelRange = Math::Box2Du::MakeMinSize(glm::uvec2{ 0 }, size);

    BPAssert(glm::all(glm::lessThan(pixelRange.GetMaxCorner(), size)),
             "Block range goes beyond the texture's size");

    //Download.
    glGetCompressedTextureSubImage(glPtr.Get(), mipLevel,
                                   pixelRange.MinCorner.x, pixelRange.MinCorner.y, (GLint)face._to_index(),
                                   pixelRange.Size.x, pixelRange.Size.y, 1,
                                   format.GetByteSize(GetSize(mipLevel)),
                                   compressedData);
}


void TextureCube::SetData(const void* data, CubeFaces face,
                          GLenum dataFormat, GLenum dataType,
                          const SetDataParams& params)
{
    //Process default arguments.
    auto destRange = params.DestRange;
    if (glm::all(glm::equal(destRange.Size, glm::uvec2{ 0 })))
        destRange = Math::Box2Du::MakeMinSize(glm::uvec2{ 0 }, size);

    //Note that for modern OpenGL Direct State Access,
    //    cubemaps are thought of as a cubemap array with length 1.
    //So we use the 3D functions to set data, and the Z offset of the texture
    //    is based on the face being set.

    glTextureSubImage3D(glPtr.Get(), params.MipLevel,
                        destRange.MinCorner.x, destRange.MinCorner.y, (GLint)face._to_index(),
                        destRange.Size.x, destRange.Size.y, 1,
                        dataFormat, dataType, data);
}
void TextureCube::GetData(void* data, CubeFaces face,
                          GLenum dataFormat, GLenum dataType,
                          const GetDataParams& params) const
{
    //Process default arguments.
    auto range = params.Range;
    if (glm::all(glm::equal(range.Size, glm::uvec2{ 0 })))
        range = Math::Box2Du::MakeMinSize(glm::uvec2{ 0 }, size);

    //Download.
    glGetTextureSubImage(glPtr.Get(), params.MipLevel,
                         range.MinCorner.x, range.MinCorner.y, (GLint)face._to_index(),
                         range.Size.x, range.Size.y, 1,
                         dataFormat, dataType,
                         (GLsizei)GetByteSize(params.MipLevel),
                         data);
}