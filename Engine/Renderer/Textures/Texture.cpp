#include "Texture.h"

#include <glm/gtx/component_wise.hpp>

using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Textures;

//TODO: A static lookup like the other OpenGL objects.
//TODO: At debug-time, provide safe deallocation checks for use in e.x. Targets.


#pragma region Handles

TexHandle::TexHandle(const Texture* src)
    : ViewGlPtr(glGetTextureHandleARB(src->GetOglPtr().Get()))
{
}
ImgHandle::ImgHandle(const Texture* src, const ImgHandleData& params)
    : Params(params),
      ViewGlPtr(glGetImageHandleARB(src->GetOglPtr().Get(),
                                    (GLint)Params.MipLevel,
                                    Params.SingleLayer.has_value(),
                                    Params.SingleLayer.value_or(0),
                                    Params.Access._to_integral()))
{

}

TexHandle::TexHandle(const Texture* src,
                     const Sampler<3>& sampler3D)
    : SamplerGlPtr(GlCreate(glCreateSamplers)),
      ViewGlPtr(glGetTextureSamplerHandleARB(src->GetOglPtr().Get(),
                                             SamplerGlPtr.Get()))
{
    sampler3D.AssertFormatIsAllowed(src->GetFormat());
    sampler3D.Apply(SamplerGlPtr);
}

TexHandle::~TexHandle()
{
    if (skipDestructor)
        return;

    //Make sure this handle is deactivated first.
    //TODO: provide some kind of memory leak tracking, and use it here.
    while (activeCount > 0)
        Deactivate();

    //Clean up the sampler object if necessary.
    if (!SamplerGlPtr.IsNull())
        glDeleteSamplers(1, &SamplerGlPtr.Get());
}
ImgHandle::~ImgHandle()
{
    if (skipDestructor)
        return;

    //Make sure this handle is deactivated first.
    //TODO: provide some kind of memory leak tracking, and use it here.
    while (activeCount > 0)
        Deactivate();
}

TexHandle::TexHandle(TexHandle&& src)
    : ViewGlPtr(src.ViewGlPtr), SamplerGlPtr(src.SamplerGlPtr),
      activeCount(src.activeCount), skipDestructor(src.skipDestructor)
{
    src.skipDestructor = true;
}
ImgHandle::ImgHandle(ImgHandle&& src)
    : ViewGlPtr(src.ViewGlPtr), Params(src.Params),
      activeCount(src.activeCount), skipDestructor(src.skipDestructor)
{
    src.skipDestructor = true;
}

void TexHandle::Activate()
{
    activeCount += 1;
    if (activeCount == 1)
        glMakeTextureHandleResidentARB(ViewGlPtr.Get());
}
void ImgHandle::Activate()
{
    activeCount += 1;
    if (activeCount == 1)
        glMakeImageHandleResidentARB(ViewGlPtr.Get(), (GLenum)Params.Access);
}

void TexHandle::Deactivate()
{
    BPAssert(activeCount > 0,
             "Deactivate() called too many times");

    activeCount -= 1;
    if (activeCount < 1)
        glMakeTextureHandleNonResidentARB(ViewGlPtr.Get());
}
void ImgHandle::Deactivate()
{
    BPAssert(activeCount > 0,
             "Deactivate() called too many times");

    activeCount -= 1;
    if (activeCount < 1)
        glMakeImageHandleNonResidentARB(ViewGlPtr.Get());
}

#pragma endregion

#pragma region Views

TexView::TexView(const Texture& owner, TexHandle& handle)
    : GlPtr(handle.ViewGlPtr), Owner(owner), Handle(handle)
{
    Handle.Activate();
}
TexView::~TexView()
{
    Handle.Deactivate();
}
TexView& TexView::operator=(const TexView& cpy)
{
    //Only bother changing things if they represent different handles.
    if (&Handle != &cpy.Handle)
    {
        this->~TexView();
        new (this) TexView(cpy);
    }
    //Otherwise, just assert that they are exactly equal.
    else
    {
        BPAssert(GlPtr == cpy.GlPtr, "GlPtr fields don't match up in TexView");
    }

    return *this;
}

ImgView::ImgView(const Texture& owner, ImgHandle& handle)
    : GlPtr(handle.ViewGlPtr), Owner(owner), Handle(handle)
{
    Handle.Activate();
}
ImgView::~ImgView()
{
    Handle.Deactivate();
}
ImgView& ImgView::operator=(const ImgView& cpy)
{
    //Only bother changing things if they represent different handles.
    if (&Handle != &cpy.Handle)
    {
        this->~ImgView();
        new (this) ImgView(cpy);
    }
    //Otherwise, just assert that they are exactly equal.
    else
    {
        BPAssert(GlPtr == cpy.GlPtr, "GlPtr fields don't match up in ImgView");
    }

    return *this;
}

#pragma endregion

#pragma region Texture

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
    sampler3D.AssertFormatIsAllowed(format);
    sampler3D.Apply(glPtr);
}
Texture::~Texture()
{
    //Destroy the handles first.
    texHandles.clear();
    imgHandles.clear();

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

size_t Texture::GetTotalByteSize() const
{
    size_t sum = 0;
    for (uint_mipLevel_t mip = 0; mip < GetByteSize(); ++mip)
        sum += GetByteSize(mip);
    return sum;
}

TexView Texture::GetViewFull(std::optional<Sampler<3>> customSampler) const
{
    auto sampler = customSampler.value_or(sampler3D);
    auto found = texHandles.find(sampler);
    if (found == texHandles.end())
        if (customSampler.has_value())
            texHandles.emplace(sampler, std::make_unique<TexHandle>(this, sampler));
        else
            texHandles.emplace(sampler, std::make_unique<TexHandle>(this));

    return TexView(*this, *texHandles[sampler]);
}
ImgView Texture::GetView(ImgHandleData params) const
{
    auto found = imgHandles.find(params);
    if (found == imgHandles.end())
    {
        texHandles.emplace(params,
                           std::make_unique<ImgHandle>(this, params.MipLevel,
                                                       params.SingleLayer, params.Access));
    }
    
    return ImgView(*this, imgHandles[params]);
}

#pragma endregion