#include "Texture.h"

#include <glm/gtx/component_wise.hpp>

using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Textures;

//TODO: A static lookup like the other OpenGL objects.
//TODO: At debug-time, provide safe deallocation checks for use in e.x. Targets.


Texture::Texture(Types _type, Format _format, uint_mipLevel_t nMips,
                 const Sampler<3>& _sampler3D)
    : type(_type), format(_format), nMipLevels(nMips),
      sampler3D(_sampler3D)
{
    BPAssert(format.GetOglEnum() != GL_NONE, "OpenGL format is invalid");

    //Create the texture handle.
    OglPtr::Texture::Data_t texPtr;
    glCreateTextures((GLenum)type, 1, &texPtr);
    glPtr.Get() = texPtr;

    //Set up the sampler settings.
    BPAssert(sampler3D.DepthSampling == +DepthComparisonModes::None ||
               format.IsDepthAndStencil() || format.IsDepthOnly(),
             "Trying to use a Depth Comparison sampler for a non-depth texture");
    sampler3D.Apply(glPtr);
}
Texture::~Texture()
{
    if (!glPtr.IsNull())
        glDeleteTextures(1, &glPtr.Get());
}

Texture::Texture(Texture&& src)
    : glPtr(src.glPtr), type(src.type),
      nMipLevels(src.nMipLevels), format(src.format),
      sampler3D(src.sampler3D)
{
    src.glPtr = OglPtr::Texture::Null();
}

void Texture::RecomputeMips()
{
    BPAssert(!format.IsCompressed(),
             "Can't compute mipmaps for a compressed texture!");

    glGenerateTextureMipmap(glPtr.Get());
}