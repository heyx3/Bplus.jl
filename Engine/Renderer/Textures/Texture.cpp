#include "Texture.h"

using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Textures;


Texture2D::Texture2D(const glm::uvec2& _size, Format _format,
                     uint_fast16_t _nMipLevels, uint_fast32_t _arrayCount)
    : type(TextureTypes::TwoD), arrayCount(_arrayCount),
      size(_size), format(_format),
      nMipLevels((_nMipLevels < 1) ? GetMaxNumbMipmaps(_size) : _nMipLevels)
{
    BPAssert(_arrayCount > 0, "Can't make a Texture2D array with 0 elements");
    BPAssert(format.GetOglEnum() != GL_NONE, "Invalid OpenGL format");

    //Create the texture handle.
    OglPtr::Texture::Data_t texPtr;
    glCreateTextures((GLenum)type, 1, &texPtr);
    glPtr.Get() = texPtr;

    //Allocate GPU storage.
    if (IsArray())
        glTextureStorage3D(glPtr.Get(), nMipLevels, format.GetOglEnum(), size.x, size.y, arrayCount);
    else
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
      size(src.size), arrayCount(src.arrayCount), nMipLevels(src.nMipLevels),
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