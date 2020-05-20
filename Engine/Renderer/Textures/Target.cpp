#include "Target.h"


using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Textures;


TargetOutput::TargetOutput(Texture1D* tex, bool own)
    : TakeOwnership(own), data(tex) { }
TargetOutput::TargetOutput(Texture2D* tex, bool own)
    : TakeOwnership(own), data(tex) { }
TargetOutput::TargetOutput(Texture3D* tex, bool own)
    : TakeOwnership(own), data(tex) { }
TargetOutput::TargetOutput(TextureCube* tex, bool own)
    : TakeOwnership(own), data(tex) { }
TargetOutput::TargetOutput(Texture3D* tex, size_t zSlice, bool own)
    : TakeOwnership(own), data(std::tuple{ tex, zSlice }) { }
TargetOutput::TargetOutput(TextureCube* tex, CubeFaces face, bool own)
    : TakeOwnership(own), data(std::tuple{ tex, face }) { }

bool TargetOutput::IsLayered() const
{
    return IsTex3D() || IsTexCube();
}
Texture* TargetOutput::GetTex() const
{
    if (IsTex1D())
        return GetTex1D();
    else if (IsTex2D())
        return GetTex2D();
    else if (IsTex3D())
        return GetTex3D();
    else if (IsTex3DSlice())
        return std::get<Texture3D*>(GetTex3DSlice());
    else if (IsTexCubeFace())
        return std::get<TextureCube*>(GetTexCubeFace());
    else
    {
        BPAssert(false, "Unknown type for TargetOutput");
        return nullptr;
    }
}


Target::Target(const glm::uvec2& _size)
    : size(_size)
{
    GLuint glPtrU;
    glCreateFramebuffers(1, &glPtrU);
    glPtr.Get() = glPtrU;
}
Target::Target(const glm::uvec2& _size,
               const Format& colorFormat, DepthStencilFormats depthFormat,
               bool depthIsRenderBuffer, uint_mipLevel_t nMips)
    : Target(_size)
{
    auto colorTex = new Texture2D(size, colorFormat, nMips);
    managedTextures.insert(std::unique_ptr<Texture>(colorTex));
    OutputSet_Color({ colorTex, true });

    if (depthIsRenderBuffer)
    {
        this->AttachDepthPlaceholder(depthFormat);
    }
    else
    {
        auto depthTex = new Texture2D(size, depthFormat, nMips);
        managedTextures.insert(std::unique_ptr<Texture>(depthTex));
        if (depthTex->GetFormat().IsDepthOnly())
            OutputSet_Depth({ depthTex, true });
        else if (depthTex->GetFormat().IsStencilOnly())
            OutputSet_Stencil({ depthTex, true });
        else
            OutputSet_DepthStencil({ depthTex, true });
    }
}

//TODO: Other constructors.

Target::~Target()
{
    //First clean up any managed textures.
    managedTextures.clear();

    //Next, clean up the FBO itself.
    glDeleteFramebuffers(1, &glPtr.Get());
}

//TODO: The rest of the functions to implement.