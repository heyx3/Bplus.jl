#include "Texture.h"

#include <glm/gtx/component_wise.hpp>

using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Textures;

//TODO: A static lookup like the other OpenGL objects.
//TODO: At debug-time, provide safe deallocation checks for use in e.x. Targets.


Texture::Texture(Types _type, Format _format, uint_mipLevel_t nMips)
    : type(_type), format(_format), nMipLevels(nMips)
{
    BPAssert(format.GetOglEnum() != GL_NONE, "OpenGL format is invalid");

    //Create the texture handle.
    OglPtr::Texture::Data_t texPtr;
    glCreateTextures((GLenum)type, 1, &texPtr);
    glPtr.Get() = texPtr;
}
Texture::~Texture()
{
    if (!glPtr.IsNull())
        glDeleteTextures(1, &glPtr.Get());
}
Texture::Texture(Texture&& src)
    : glPtr(src.glPtr), type(src.type),
      nMipLevels(src.nMipLevels), format(src.format)
{
    src.glPtr.Get() = OglPtr::Texture::Null;
}
Texture& Texture::operator=(Texture&& src)
{
    //Destruct this instance, then move-construct it.
    this->~Texture();
    new (this) Texture(std::move(src));

    return *this;
}

void Texture::RecomputeMips()
{
    BPAssert(!format.IsCompressed(),
             "Can't compute mipmaps for a compressed texture!");

    glGenerateTextureMipmap(glPtr.Get());
}