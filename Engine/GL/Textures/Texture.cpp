#include "Texture.h"

#include <glm/gtx/component_wise.hpp>

using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Textures;

//TODO: A static lookup like the other OpenGL objects.
//TODO: At debug-time, provide safe deallocation checks for use in e.x. Targets.


#pragma region Handles

namespace
{
    OglPtr::Sampler MakeSampler(const Sampler<3>& sampler3D)
    {
        //TODO: Can multiple bindless texture views use the same sampler? If so, should we just keep a static dictionary of samplers instead of many redundant ones?

        OglPtr::Sampler ptr(GlCreate(glCreateSamplers));
        sampler3D.Apply(ptr);
        return ptr;
    }
}

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
    : SamplerGlPtr(MakeSampler(sampler3D)),
      ViewGlPtr(glGetTextureSamplerHandleARB(src->GetOglPtr().Get(),
                                             SamplerGlPtr.Get()))
{

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
    BP_ASSERT(activeCount > 0,
             "Deactivate() called too many times");

    activeCount -= 1;
    if (activeCount < 1)
        glMakeTextureHandleNonResidentARB(ViewGlPtr.Get());
}
void ImgHandle::Deactivate()
{
    BP_ASSERT(activeCount > 0,
             "Deactivate() called too many times");

    activeCount -= 1;
    if (activeCount < 1)
        glMakeImageHandleNonResidentARB(ViewGlPtr.Get());
}

#pragma endregion

#pragma region Views

TexView::TexView(TexHandle& handle)
    : GlPtr(handle.ViewGlPtr), Handle(handle)
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
        BP_ASSERT(GlPtr == cpy.GlPtr, "GlPtr fields don't match up in TexView");
    }

    return *this;
}

ImgView::ImgView(ImgHandle& handle)
    : GlPtr(handle.ViewGlPtr), Handle(handle)
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
        BP_ASSERT(GlPtr == cpy.GlPtr, "GlPtr fields don't match up in ImgView");
    }

    return *this;
}

#pragma endregion

#pragma region Texture

Texture::Texture(Types _type, Format _format, uint_mipLevel_t nMips,
                 const Sampler<3>& _sampler3D,
                 SwizzleRGBA customSwizzling,
                 std::optional<DepthStencilSources> customDepthStencilMode)
    : type(_type), format(_format), nMipLevels(nMips),
      sampler3D(_sampler3D),
      swizzling({ SwizzleSources::Red, SwizzleSources::Green, SwizzleSources::Blue, SwizzleSources::Alpha }),
      depthStencilMode(std::nullopt)
{
    BP_ASSERT(format.GetOglEnum() != GL_NONE, "OpenGL format is invalid");
    BP_ASSERT(!customDepthStencilMode.has_value() || format.IsDepthAndStencil(),
             "Can't give a depth/stencil sampling mode to a texture that isn't depth/stencil");
    BP_ASSERT(customDepthStencilMode.has_value() || !format.IsDepthAndStencil(),
             "Must give a depth/stencil sampling mode if a texture is depth/stencil");

    //Create the texture handle.
    OglPtr::Texture::Data_t texPtr;
    glCreateTextures((GLenum)type, 1, &texPtr);
    glPtr.Get() = texPtr;

    //Set up the sampler settings.
    SetSwizzling(customSwizzling);
    if (customDepthStencilMode.has_value())
        SetDepthStencilSource(*customDepthStencilMode);
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
      sampler3D(src.sampler3D),
      swizzling(src.swizzling), depthStencilMode(src.depthStencilMode)
{
    src.glPtr = OglPtr::Texture::Null();
}

void Texture::RecomputeMips()
{
    BP_ASSERT(!format.IsCompressed(),
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

void Texture::SetSwizzling(const SwizzleRGBA& newSwizzling)
{
    //Tell OpenGL about the change, but skip ones that aren't actually changing.
    const auto glArgs = std::array{ GL_TEXTURE_SWIZZLE_R, GL_TEXTURE_SWIZZLE_G,
                                    GL_TEXTURE_SWIZZLE_B, GL_TEXTURE_SWIZZLE_A };
    for (uint_fast8_t i = 0; i < glArgs.size(); ++i)
    {
        if (newSwizzling[i] != swizzling[i])
            glTextureParameteri(glPtr.Get(), glArgs[i], (GLint)newSwizzling[i]);
    }

    swizzling = newSwizzling;
}
void Texture::SetDepthStencilSource(DepthStencilSources newSource)
{
    BP_ASSERT(format.IsDepthAndStencil(),
             "Can only set DepthStencil mode for a Depth/Stencil hybrid texture");

    if (newSource != depthStencilMode)
    {
        depthStencilMode = newSource;
        glTextureParameteri(glPtr.Get(), GL_DEPTH_STENCIL_TEXTURE_MODE, (GLint)newSource);
    }
}

TexHandle& Texture::GetViewHandleFull(std::optional<Sampler<3>> customSampler) const
{
    auto sampler = customSampler.value_or(sampler3D);

    //Error-checking on the sampler type:
    bool isStencilSampler = format.IsStencilOnly() ||
                            depthStencilMode == +DepthStencilSources::Stencil,
         isDepthSampler = format.IsDepthOnly() ||
                          depthStencilMode == +DepthStencilSources::Depth;
    BP_ASSERT(!isStencilSampler || sampler.PixelFilter == +PixelFilters::Rough,
             "Can't use 'Smooth' filtering on a stencil texture sampler -- the values are integers");
    BP_ASSERT(isDepthSampler || !sampler.DepthComparisonMode.has_value(),
             "Can't use a depth comparison sampler (a.k.a. 'shadow sampler') on a non-depth texture");

    auto found = texHandles.find(sampler);

    //Create the handle if it doesn't exist.
    if (found == texHandles.end())
        if (customSampler.has_value())
            found = texHandles.emplace(sampler, new TexHandle(this, sampler)).first;
        else
            found = texHandles.emplace(sampler, new TexHandle(this)).first;

    return *found->second;
}
ImgHandle& Texture::GetViewHandle(ImgHandleData params) const
{
    auto found = imgHandles.find(params);
    if (found == imgHandles.end())
        found = imgHandles.emplace(params, new ImgHandle(this, params)).first;

    return *found->second;
}

TexView Texture::GetViewFull(std::optional<Sampler<3>> customSampler) const
{
    return TexView(GetViewHandleFull(customSampler));
}
ImgView Texture::GetView(ImgHandleData params) const
{
    return ImgView(GetViewHandle(params));
}

#pragma endregion