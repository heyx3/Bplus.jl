#include "Views.h"

#include "Texture.h"


using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Textures;

namespace
{
    std::array<WrapModes, 3> BuildWrapData(const Texture* tex)
    {
        std::array<WrapModes, 3> result;
        for (size_t i = 0; i < result.size(); ++i)
            result[i] = WrapModes::Clamp;
    }
}


TextureView::TextureView(const Texture* src)
    : texture(src),
      MinFilter(texture->GetMinFilter()),
      MagFilter(texture->GetMagFilter()),
      WrapParams3D(BuildWrapData(texture)),
      MipClampRange(Math::IntervalF::MakeMinSize(glm::vec1(-1000.0f), //This is the OpenGL standardized default.
                                                 glm::vec1(2000.0f))),
      MipOffset(0)
{
    viewGlPtr.Get() = glGetTextureHandleARB(src.Get());
}
TextureView::TextureView(const Texture* src,
                         TextureMinFilters minFilter, TextureMagFilters magFilter,
                         WrapModes* wrappingPerAxis, glm::length_t samplerDimensions)
    : texGlPtr(src)
{
    glCreateSamplers(1, &samplerGlPtr.Get());
    glSamplerParameteri(samplerGlPtr.Get(), GL_TEXTURE_MIN_FILTER, (GLint)minFilter);
    glSamplerParameteri(samplerGlPtr.Get(), GL_TEXTURE_MAG_FILTER, (GLint)magFilter);
}

TextureView::~TextureView()
{
    //Note that the handle does not have to get cleaned up manually;
    //    it presumably goes away when the texture itself is cleaned up.
    if (!samplerGlPtr.IsNull())
        glDeleteSamplers(1, &samplerGlPtr.Get());
}