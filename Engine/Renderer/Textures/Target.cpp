#include "Target.h"

#include "../Device.h"


using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Textures;


TargetOutput::TargetOutput(Texture1D* tex, uint_mipLevel_t mipLevel)
    : data(tex), MipLevel(mipLevel) { }
TargetOutput::TargetOutput(Texture2D* tex, uint_mipLevel_t mipLevel)
    : data(tex), MipLevel(mipLevel) { }
TargetOutput::TargetOutput(Texture3D* tex, uint_mipLevel_t mipLevel)
    : data(tex), MipLevel(mipLevel) { }
TargetOutput::TargetOutput(TextureCube* tex, uint_mipLevel_t mipLevel)
    : data(tex), MipLevel(mipLevel) { }
TargetOutput::TargetOutput(std::tuple<Texture3D*, size_t> tex, uint_mipLevel_t mipLevel)
    : data(tex), MipLevel(mipLevel) { }
TargetOutput::TargetOutput(std::tuple<TextureCube*, CubeFaces> data, uint_mipLevel_t mipLevel)
    : data(data), MipLevel(mipLevel) { }

bool TargetOutput::IsLayered() const
{
    if (IsTex3D() | IsTexCube())
        return true;
    else if (IsTex1D() | IsTex2D() | IsTex3DSlice() | IsTexCubeFace())
        return false;
    else
    {
        BPAssert(false, "Unknown TargetOutput type");
        return false;
    }
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
glm::uvec2 TargetOutput::GetSize() const
{
    if (IsTex1D())
        return glm::uvec2(GetTex1D()->GetSize().x, 1);
    else if (IsTex2D())
        return GetTex2D()->GetSize();
    else if (IsTex3D())
        return GetTex3D()->GetSize();
    else if (IsTexCube())
        return GetTexCube()->GetSize();
    else if (IsTexCubeFace())
        return std::get<0>(GetTexCubeFace())->GetSize();
    else if (IsTex3DSlice())
        return std::get<0>(GetTex3DSlice())->GetSize();
    else
    {
        BPAssert(false, "Unknown type for TargetOutput");
        return { 0 };
    }
}
size_t TargetOutput::GetLayer() const
{
    BPAssert(!IsLayered(),
             "Trying to get the specific layer from a multi-layered output");

    if (IsTex1D() | IsTex2D())
        return 0;
    else if (IsTex3DSlice())
        return std::get<1>(GetTex3DSlice());
    else if (IsTexCubeFace())
        return std::get<1>(GetTexCubeFace())._to_index();
    else if (IsTex3D() | IsTexCube())
        { BPAssert(false, "Shouldn't ever get here"); return -1; }
    else
    {
        BPAssert(false, "Unknown TargetOutput type");
        return -1;
    }
}
size_t TargetOutput::GetLayerCount() const
{
    if (!IsLayered())
        return 1;
    else if (IsTex3D())
        return (size_t)GetTex3D()->GetSize().z;
    else if (IsTexCube())
        return 6;
    else
    {
        BPAssert(false, "Unknown TargetOutput type");
        return 1;
    }
}


Target::Target(const glm::uvec2& _size, size_t nLayers)
    : size(_size)
{
    BPAssert(size.x > 0, "Target's width can't be 0");
    BPAssert(size.y > 0, "Target's height can't be 0");

    GLuint glPtrU;
    glCreateFramebuffers(1, &glPtrU);
    glPtr.Get() = glPtrU;

    glNamedFramebufferParameteri(glPtr.Get(), GL_FRAMEBUFFER_DEFAULT_WIDTH, (GLint)size.x);
    glNamedFramebufferParameteri(glPtr.Get(), GL_FRAMEBUFFER_DEFAULT_HEIGHT, (GLint)size.y);
    if (nLayers > 1)
        glNamedFramebufferParameteri(glPtr.Get(), GL_FRAMEBUFFER_DEFAULT_LAYERS, (GLint)nLayers);
}
Target::Target(const glm::uvec2& _size,
               const Format& colorFormat, DepthStencilFormats depthFormat,
               bool depthIsRenderBuffer, uint_mipLevel_t nMips)
    : Target(_size)
{
    auto colorTex = new Texture2D(size, colorFormat, nMips);
    managedTextures.insert(colorTex);
    OutputSet_Color({ colorTex }, true);

    if (depthIsRenderBuffer)
    {
        AttachDepthPlaceholder(depthFormat);
    }
    else
    {
        auto depthTex = new Texture2D(size, depthFormat, nMips);
        managedTextures.insert(depthTex);
        if (depthTex->GetFormat().IsDepthOnly())
            OutputSet_Depth({ depthTex }, true);
        else if (depthTex->GetFormat().IsStencilOnly())
            OutputSet_Stencil({ depthTex }, true);
        else
            OutputSet_DepthStencil({ depthTex }, true);
    }
}

Target::Target(const TargetOutput& color, const TargetOutput& depth)
    : Target(glm::min(color.GetSize(), depth.GetSize()),
             std::min(color.GetLayerCount(), depth.GetLayerCount()))
{
    BPAssert(depth.GetSize() == color.GetSize(),
             "Color and depth aren't same size");

    OutputSet_Depth(depth);
    OutputSet_Color(color);
}
Target::Target(const TargetOutput& color, DepthStencilFormats depthFormat)
    : Target(color.GetSize(), color.GetLayerCount())
{
    OutputSet_Color(color);
    AttachDepthPlaceholder(depthFormat);
}

#pragma region Helper functions for the last Target() constructor
namespace
{
    //Used to find the min size and layer count for
    //    one of the Target constructors.
    template<typename T,
             T(*ValueGetter)(const TargetOutput&),
             T(*MinFunc)(const T&, const T&)>
    T ComputeMin(const TargetOutput* outputsList, size_t nOutputsInList,
                 std::optional<TargetOutput> optionalOutput,
                 T maxVal, T defaultVal)
    {
        if (nOutputsInList == 0 && !optionalOutput.has_value())
            return defaultVal;

        T output = maxVal;
        if (optionalOutput.has_value())
            output = MinFunc(output, ValueGetter(optionalOutput.value()));

        for (size_t i = 0; i < nOutputsInList; ++i)
            output = MinFunc(output, ValueGetter(outputsList[i]));

        return output;
    }

    glm::uvec2 GetTargOutSize(const TargetOutput& to) { return to.GetSize(); }
    size_t GetTargOutLayerCount(const TargetOutput& to) { return to.GetLayerCount(); }
}
#pragma endregion
Target::Target(const TargetOutput* colorOutputs, size_t nColorOutputs,
               std::optional<TargetOutput> depthOutput)
    : Target(ComputeMin<glm::uvec2, GetTargOutSize, glm::min>(
                colorOutputs, nColorOutputs, depthOutput,
                glm::uvec2{ std::numeric_limits<glm::u32>().max() },
                glm::uvec2{ 1 }),
             ComputeMin<size_t, GetTargOutLayerCount, std::min>(
                colorOutputs, nColorOutputs, depthOutput,
                (size_t)std::numeric_limits<size_t>().max(),
                (size_t)1))
{
    //Set up the color attachments.
    for (size_t i = 0; i < nColorOutputs; ++i)
        OutputSet_Color(colorOutputs[i], i);

    //Set up the depth attachment.
    if (depthOutput.has_value())
        OutputSet_Depth(depthOutput.value());
    else
        AttachDepthPlaceholder(DepthStencilFormats::Depth_24U);
}

Target::~Target()
{
    //First clean up any managed textures.
    for (auto* texPtr : managedTextures)
        delete texPtr;

    //Next, clean up the depth renderbuffer.
    if (glPtr_DepthRenderBuffer.Get() != OglPtr::TargetBuffer::Null)
        glDeleteRenderbuffers(1, &glPtr.Get());

    //Finally, clean up the FBO itself.
    glDeleteFramebuffers(1, &glPtr.Get());
}


Target::Target(Target&& from)
    : Target(from.size)
{
    glPtr = from.glPtr;
    tex_colors = std::move(from.tex_colors);
    tex_depth = from.tex_depth;
    tex_stencil = from.tex_stencil;
    isDepthStencilBound = from.isDepthStencilBound;
    glPtr_DepthRenderBuffer = from.glPtr_DepthRenderBuffer;
    managedTextures = std::move(from.managedTextures);

    from.glPtr.Get() = OglPtr::Target::Null;
    from.glPtr_DepthRenderBuffer.Get() = OglPtr::TargetBuffer::Null;
}

TargetStates Target::Validate() const
{
    //Make sure none of the formats are compressed.
    if ((tex_depth.has_value() &&
         tex_depth.value().GetTex()->GetFormat().IsCompressed()) ||
        (tex_stencil.has_value() &&
         tex_stencil.value().GetTex()->GetFormat().IsCompressed()) ||
        std::any_of(tex_colors.begin(), tex_colors.end(),
                    [](const TargetOutput& o)
                        { return o.GetTex()->GetFormat().IsCompressed(); }))
    {
        return TargetStates::CompressedFormat;
    }

    //Ask OpenGL if any other errors were detected.
    auto status = glCheckNamedFramebufferStatus(glPtr.Get(), GL_DRAW_FRAMEBUFFER);
    if (status == GL_FRAMEBUFFER_COMPLETE)
        return TargetStates::Ready;
    else if (status == GL_FRAMEBUFFER_UNSUPPORTED)
        return TargetStates::DriverUnsupported;
    else if (status == GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS)
        return TargetStates::LayerMixup;
    //All other errors should have been handled by the logic in this class.
    else
    {
        std::string errMsg = "Unexpected glCheckFramebufferStatus code: ";
        errMsg += IO::ToHex(status);
        BPAssert(false, errMsg.c_str());
    }
}

void Target::TakeOwnership(Texture* tex)
{
    managedTextures.insert(tex);
}
void Target::ReleaseOwnership(Texture* tex)
{
    //Find the texture in the set of managed textures.
    auto found = managedTextures.find(tex);
    BPAssert(found != managedTextures.end(),
             "This Target doesn't own the given texture");

    //Stop tracking it.
    managedTextures.erase(found);
}
void Target::HandleRemoval(Texture* tex)
{
    //If the texture is being managed and isn't used anywhere, clean it up.
    auto found = managedTextures.find(tex);
    if (found != managedTextures.end() &&
        (!tex_depth.has_value() || tex_depth.value().GetTex() != tex) &&
        (!tex_stencil.has_value() || tex_stencil.value().GetTex() != tex) &&
        std::none_of(tex_colors.begin(), tex_colors.end(),
                     [&](const auto& tex) { return tex.GetTex() = tex; }))
    {
        delete *found;
        managedTextures.erase(found);
    }
}

void Target::AttachAt(GLenum attachment, const TargetOutput& output)
{
    if (output.IsLayered())
    {
        glNamedFramebufferTexture(glPtr.Get(), attachment,
                                  output.GetTex()->GetOglPtr().Get(),
                                  output.MipLevel);
    }
    else
    {
        glNamedFramebufferTextureLayer(glPtr.Get(), attachment,
                                       output.GetTex()->GetOglPtr().Get(),
                                       output.MipLevel, (GLint)output.GetLayer());
    }
}
void Target::RemoveAt(GLenum attachment)
{
    glNamedFramebufferTexture(glPtr.Get(), attachment, 0, 0);
}

void Target::RecomputeSize()
{
    //Edge-case: there are no attachments, so fall back to the default size.
    if (tex_colors.size() == 0 && !tex_depth.has_value() && !tex_stencil.has_value())
    {
        GLint x, y;
        glGetNamedFramebufferParameteriv(glPtr.Get(), GL_FRAMEBUFFER_DEFAULT_WIDTH, &x);
        glGetNamedFramebufferParameteriv(glPtr.Get(), GL_FRAMEBUFFER_DEFAULT_HEIGHT, &y);
        size = { (glm::u32)x, (glm::u32)y };
    }
    else
    {
        size = { std::numeric_limits<glm::u32>().max() };
        size = ComputeMin<glm::uvec2, GetTargOutSize, glm::min>(
                   tex_colors.data(), tex_colors.size(), tex_depth,
                   glm::uvec2{ std::numeric_limits<glm::u32>().max() },
                   glm::uvec2{ 0 });
        if (tex_stencil.has_value())
            size = glm::min(size, tex_stencil.value().GetSize());
    }
}

void Target::OutputSet_Color(const TargetOutput& newOutput, size_t index,
                             bool takeOwnership)
{
    //Take ownership over the texture, if requested.
    if (takeOwnership)
        TakeOwnership(newOutput.GetTex());

    //Make sure we can have this many color attachments.
    BPAssert(index < Device::GetContextDevice()->GetMaxColorTargets(),
             "Driver doesn't support this many color outputs in one Target");

    //If there's already an attachment at this index, replace it.
    if (index < tex_colors.size())
    {
        auto* texBeingRemoved = tex_colors[index].GetTex();
        tex_colors[index] = newOutput;
        HandleRemoval(texBeingRemoved);
    }
    //Otherwise, add a new element to the colors array.
    else
    {
        BPAssert(index == tex_colors.size(),
                 "Trying to add a new color attachment out of order");
        tex_colors.push_back(newOutput);
    }
    AttachAt(GL_COLOR_ATTACHMENT0 + index, newOutput);

    //Update the size of this Target.
    size = glm::min(size, newOutput.GetSize());
}
//TODO: Other OutputSet_...()

void Target::OutputRemove_Color(size_t index)
{
    //Make sure there are no color attachments after this one,
    //    so we don't create a "hole" in the list here.
    BPAssert(index == tex_colors.size() - 1,
             "Trying to remove a color attachment out of order");

    auto* texToRemove = tex_colors[index].GetTex();
    tex_colors.erase(tex_colors.end() - 1);

    RemoveAt(GL_COLOR_ATTACHMENT0 + index);
    HandleRemoval(texToRemove);
    RecomputeSize();
}
//TODO: Other OutputRemove_...()

void Target::OutputRelease(Texture* tex, bool removeOutputs)
{
    //Remove the texture from management.
    auto found = managedTextures.find(tex);
    BPAssert(found == managedTextures.end(), "Can't release a texture we don't own!");
    managedTextures.erase(found);

    //If we also need to remove it as an output, do so.
    if (removeOutputs)
    {
        //Start from the end of the color attachments, so things are removed in order.
        for (auto i = (int64_t)GetNColorOutputs() - 1; i >= 0; --i)
            if (GetOutput_Color(i)->GetTex() == tex)
                OutputRemove_Color(i);

        //Remove it from depth/stencil attachments if applicable.
        if (GetOutput_Stencil() != nullptr && GetOutput_Stencil()->GetTex() == tex)
            OutputRemove_Stencil();
        if (GetOutput_Depth() != nullptr && GetOutput_Depth()->GetTex() == tex)
            OutputRemove_Depth(true);
    }
}