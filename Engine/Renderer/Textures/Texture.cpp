#include "Texture.h"

#include <glm/gtx/component_wise.hpp>

using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Textures;

//A great reference for getting/setting texture data in OpenGL:
//    https://www.khronos.org/opengl/wiki/Pixel_Transfer


Texture2D::Texture2D(const glm::uvec2& _size, Format _format,
                     uint_mipLevel_t _nMipLevels)
    : type(Types::TwoD),
      size(_size), format(_format),
      nMipLevels((_nMipLevels < 1) ? GetMaxNumbMipmaps(_size) : _nMipLevels)
{
    BPAssert(format.GetOglEnum() != GL_NONE, "Invalid OpenGL format");

    //Create the texture handle.
    OglPtr::Texture::Data_t texPtr;
    glCreateTextures((GLenum)type, 1, &texPtr);
    glPtr.Get() = texPtr;

    //Allocate GPU storage.
    glTextureStorage2D(glPtr.Get(), nMipLevels, format.GetOglEnum(), size.x, size.y);

    //Load the initial sampler data.
    GLint p;
    glGetTextureParameteriv(glPtr.Get(), GL_TEXTURE_MIN_FILTER, &p);
    sampler.MinFilter = TextureMinFilters::_from_integral(p);
    glGetTextureParameteriv(glPtr.Get(), GL_TEXTURE_MAG_FILTER, &p);
    sampler.MagFilter = TextureMagFilters::_from_integral(p);
    glGetTexParameteriv(glPtr.Get(), GL_TEXTURE_WRAP_S, &p);
    sampler.Wrapping[0] = TextureWrapping::_from_integral(p);
    glGetTexParameteriv(glPtr.Get(), GL_TEXTURE_WRAP_T, &p);
    sampler.Wrapping[1] = TextureWrapping::_from_integral(p);
}
Texture2D::~Texture2D()
{
    if (glPtr != OglPtr::Texture::Null)
        glDeleteTextures(1, &glPtr.Get());
}

Texture2D::Texture2D(Texture2D&& src)
    : glPtr(src.glPtr), type(src.type),
      size(src.size), nMipLevels(src.nMipLevels),
      format(src.format), sampler(src.sampler)
{
    src.glPtr.Get() = OglPtr::Texture::Null;
}
Texture2D& Texture2D::operator=(Texture2D&& src)
{
    //Call the deconstructor, then the move constructor, on this instance.
    this->~Texture2D();
    new (this) Texture2D(std::move(src));

    return *this;
}


void Texture2D::SetSampler(const Sampler<2>& s)
{
    sampler = s;

    glTextureParameteri(glPtr.Get(), GL_TEXTURE_MIN_FILTER,
                        (GLint)sampler.MinFilter);
    glTextureParameteri(glPtr.Get(), GL_TEXTURE_MAG_FILTER,
                        (GLint)sampler.MagFilter);
    glTextureParameteri(glPtr.Get(), GL_TEXTURE_WRAP_S,
                        (GLint)sampler.Wrapping[0]);
    glTextureParameteri(glPtr.Get(), GL_TEXTURE_WRAP_T,
                        (GLint)sampler.Wrapping[1]);
}

void Texture2D::RecomputeMips()
{
    BPAssert(!format.IsCompressed(),
             "Can't auto-compute mipmaps for a compressed texture!");

}

void Texture2D::SetData_Compressed(const std::byte* compressedData,
                                   size_t compressedDataSize,
                                   Math::Box2Du destBlockRange,
                                   uint_mipLevel_t mipLevel)
{
    //Convert block range to pixel size.
    auto blockSize = GetBlockSize(format.AsCompressed());
    auto destPixelRange = Math::Box2Du::MakeMinSize(destBlockRange.MinCorner * blockSize,
                                                    destBlockRange.Size * blockSize);

    //Process default arguments.
    if (glm::all(glm::equal(destPixelRange.Size, glm::uvec2{ 0 })))
        destPixelRange = Math::Box2Du::MakeMinSize({ 0 }, size);

    BPAssert(glm::all(glm::lessThan(destPixelRange.GetMaxCorner(), size)),
             "Block range goes beyond the texture's size");

    //Upload.
    glCompressedTextureSubImage2D(glPtr.Get(), mipLevel,
                                  destPixelRange.MinCorner.x, destPixelRange.MinCorner.y,
                                  destPixelRange.Size.x, destPixelRange.Size.y,
                                  format.GetOglEnum(),
                                  compressedDataSize, compressedData);
}
void Texture2D::SetData(const void* data,
                        GLenum dataFormat, GLenum dataType,
                        const SetDataParams& params)
{
    //Process default arguments.
    auto destRange = params.DestRange;
    if (glm::all(glm::equal(destRange.Size, glm::uvec2{ 0 })))
        destRange = Math::Box2Du::MakeMinSize({ 0 }, size);

    //Upload.
    glTextureSubImage2D(glPtr.Get(), params.MipLevel,
                        destRange.MinCorner.x, destRange.MinCorner.y,
                        destRange.Size.x, destRange.Size.y,
                        dataFormat, dataType, data);
}

void Texture2D::GetData_Compressed(void* compressedData,
                                   Math::Box2Du blockRange,
                                   uint_mipLevel_t mipLevel) const
{
    //Convert block range to pixel size.
    auto blockSize = GetBlockSize(format.AsCompressed());
    auto pixelRange = Math::Box2Du::MakeMinSize(blockRange.MinCorner * blockSize,
                                                blockRange.Size * blockSize);

    //Process default arguments.
    if (glm::all(glm::equal(pixelRange.Size, glm::uvec2{ 0 })))
        pixelRange = Math::Box2Du::MakeMinSize({ 0 }, size);

    BPAssert(glm::all(glm::lessThan(pixelRange.GetMaxCorner(), size)),
             "Block range goes beyond the texture's size");

    //Download.
    glGetCompressedTextureSubImage(glPtr.Get(), mipLevel,
                                   pixelRange.MinCorner.x, pixelRange.MinCorner.y, 0,
                                   pixelRange.Size.x, pixelRange.Size.y, 1,
                                   format.GetByteSize(GetSize(mipLevel)),
                                   compressedData);
}
void Texture2D::GetData(void* data,
                        GLenum dataFormat, GLenum dataType,
                        const GetDataParams& params) const
{
    //Process default arguments.
    auto range = params.Range;
    if (glm::all(glm::equal(range.Size, glm::uvec2{ 0 })))
        range = Math::Box2Du::MakeMinSize({ 0 }, size);

    //Download.
    glGetTextureSubImage(glPtr.Get(), params.MipLevel,
                         range.MinCorner.x, range.MinCorner.y, 0,
                         range.Size.x, range.Size.y, 1,
                         dataFormat, dataType,
                         GetByteSize(params.MipLevel), data);
}