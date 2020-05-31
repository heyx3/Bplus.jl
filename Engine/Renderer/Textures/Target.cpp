#include "Target.h"

#include "../Device.h"

#include <glm/gtc/type_ptr.hpp>


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
TargetOutput::TargetOutput(std::tuple<Texture3D*, uint32_t> tex, uint_mipLevel_t mipLevel)
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
        return glm::uvec2{ 0 };
    }
}
uint32_t TargetOutput::GetLayer() const
{
    BPAssert(!IsLayered(),
             "Trying to get the specific layer from a multi-layered output");

    if (IsTex1D() | IsTex2D())
        return 0;
    else if (IsTex3DSlice())
        return std::get<1>(GetTex3DSlice());
    else if (IsTexCubeFace())
        return (uint32_t)std::get<1>(GetTexCubeFace())._to_index();
    else if (IsTex3D() | IsTexCube())
        { BPAssert(false, "Shouldn't ever get here"); return -1; }
    else
    {
        BPAssert(false, "Unknown TargetOutput type");
        return -1;
    }
}
uint32_t TargetOutput::GetLayerCount() const
{
    if (!IsLayered())
        return 1;
    else if (IsTex3D())
        return (uint32_t)GetTex3D()->GetSize().z;
    else if (IsTexCube())
        return 6;
    else
    {
        BPAssert(false, "Unknown TargetOutput type");
        return 1;
    }
}


Target::Target(const glm::uvec2& _size, uint32_t nLayers)
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

        for (uint32_t i = 0; i < nOutputsInList; ++i)
            output = MinFunc(output, ValueGetter(outputsList[i]));

        return output;
    }

    glm::uvec2 GetTargOutSize(const TargetOutput& to) { return to.GetSize(); }
    uint32_t GetTargOutLayerCount(const TargetOutput& to) { return to.GetLayerCount(); }

    uint32_t MinU32(const uint32_t& a, const uint32_t& b) { return std::min(a, b); }
}
#pragma endregion
Target::Target(const TargetOutput* colorOutputs, uint32_t nColorOutputs,
               std::optional<TargetOutput> depthOutput)
    : Target(ComputeMin<glm::uvec2, GetTargOutSize, glm::min>(
                colorOutputs, nColorOutputs, depthOutput,
                glm::uvec2{ std::numeric_limits<glm::u32>().max() },
                glm::uvec2{ 1 }),
             ComputeMin<uint32_t, GetTargOutLayerCount, MinU32>(
                colorOutputs, nColorOutputs, depthOutput,
                std::numeric_limits<uint32_t>().max(),
                (uint32_t)1))
{
    //Set up the color attachments.
    for (uint32_t i = 0; i < nColorOutputs; ++i)
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
    if (depthBuffer.has_value())
        depthBuffer.reset();

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
    isDepthRBBound = from.isDepthRBBound;
    isStencilRBBound = from.isStencilRBBound;
    depthBuffer = std::move(from.depthBuffer);
    managedTextures = std::move(from.managedTextures);

    from.glPtr.Get() = OglPtr::Target::Null;
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
        return TargetStates::Unknown;
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
                     [&](const TargetOutput& tex2) { return tex == tex2.GetTex(); }))
    {
        delete *found;
        managedTextures.erase(found);
    }
}

void Target::AttachAt(GLenum attachment, const TargetOutput& output)
{
    if (output.IsLayered())
    {
        glNamedFramebufferTextureLayer(glPtr.Get(), attachment,
                                       output.GetTex()->GetOglPtr().Get(),
                                       output.MipLevel, (GLint)output.GetLayer());
    }
    else
    {
        glNamedFramebufferTexture(glPtr.Get(), attachment,
                                  output.GetTex()->GetOglPtr().Get(),
                                  output.MipLevel);
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

void Target::OutputSet_Color(const TargetOutput& newOutput, uint32_t index,
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
    AttachAt(GL_COLOR_ATTACHMENT0 + (GLenum)index, newOutput);
    RecomputeSize();
}
void Target::OutputSet_Depth(const TargetOutput& newOutput, bool takeOwnership)
{
    //Take ownership over the texture, if requested.
    if (takeOwnership)
        TakeOwnership(newOutput.GetTex());

    BPAssert(newOutput.GetTex()->GetFormat().IsDepthOnly(),
             "Trying to attach a texture with non-depth format to the depth!");

    //If there's already a texture attached here, replace it.
    if (tex_depth.has_value())
    {
        auto* texBeingRemoved = tex_depth.value().GetTex();
        tex_depth = newOutput; //Doing this before HandleRemoval() is important.
        HandleRemoval(texBeingRemoved);
    }

    tex_depth = newOutput;
    AttachAt(GL_DEPTH_ATTACHMENT, newOutput);
    RecomputeSize();
    isDepthRBBound = false;
}
void Target::OutputSet_Stencil(const TargetOutput& newOutput, bool takeOwnership)
{
    //Take ownership over the texture, if requested.
    if (takeOwnership)
        TakeOwnership(newOutput.GetTex());

    BPAssert(newOutput.GetTex()->GetFormat().IsStencilOnly(),
             "Trying to attach a texture with non-stencil format to the stencil!");

    //If there's already a texture attached here, replace it.
    if (tex_stencil.has_value())
    {
        auto* texBeingRemoved = tex_stencil.value().GetTex();
        tex_stencil = newOutput; //Doing this before HandleRemoval() is important.
        HandleRemoval(texBeingRemoved);
    }

    tex_stencil = newOutput;
    AttachAt(GL_STENCIL_ATTACHMENT, newOutput);
    RecomputeSize();
    isStencilRBBound = false;
}
void Target::OutputSet_DepthStencil(const TargetOutput& newOutput, bool takeOwnership)
{
    //Take ownership over the texture, if requested.
    if (takeOwnership)
        TakeOwnership(newOutput.GetTex());

    BPAssert(newOutput.GetTex()->GetFormat().IsDepthAndStencil(),
             "Trying to attach a depth/stencil hybrid output using a texture of the wrong format!");

    //If there's already a texture attached here, replace it.
    if (tex_depth.has_value())
    {
        auto* texBeingRemoved = tex_depth.value().GetTex();
        tex_depth = newOutput; //Doing this before HandleRemoval() is important.
        HandleRemoval(texBeingRemoved);
    }
    if (tex_stencil.has_value())
    {
        auto* texBeingRemoved = tex_stencil.value().GetTex();
        tex_stencil = newOutput;
        HandleRemoval(texBeingRemoved);
    }

    tex_depth = newOutput;
    tex_stencil = newOutput;
    AttachAt(GL_DEPTH_STENCIL_ATTACHMENT, newOutput);
    RecomputeSize();
    isDepthRBBound = false;
    isStencilRBBound = false;
}

void Target::OutputRemove_Color(uint32_t index)
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
void Target::OutputRemove_Depth(bool replaceWithRenderbuffer)
{
    auto* texToRemove = tex_depth.has_value() ? tex_depth.value().GetTex() : nullptr;
    tex_depth = std::nullopt;

    RemoveAt(GL_DEPTH_ATTACHMENT);
    HandleRemoval(texToRemove);
    RecomputeSize();

    if (replaceWithRenderbuffer)
        AttachDepthPlaceholder();
    else
        isDepthRBBound = true;
}
void Target::OutputRemove_Stencil()
{
    auto* texToRemove = tex_stencil.has_value() ? tex_stencil.value().GetTex() : nullptr;
    tex_stencil = std::nullopt;

    RemoveAt(GL_STENCIL_ATTACHMENT);
    HandleRemoval(texToRemove);
    RecomputeSize();

    isStencilRBBound = false;
}
void Target::OutputRemove_DepthStencil(bool replaceWithRenderBuffer)
{
    OutputRemove_Stencil();
    OutputRemove_Depth(replaceWithRenderBuffer);
}

void Target::AttachDepthPlaceholder(DepthStencilFormats format)
{
    BPAssert(!tex_stencil.has_value(),
             "Can't use a stencil texture and a depth buffer separately; they must be the same texture");

    //Create a new renderbuffer if we need it.
    if (!depthBuffer.has_value() || depthBuffer.value().GetSize() != size)
        depthBuffer = TargetBuffer{ Format{format}, size };

    //Attach the renderbuffer.
    GLenum attachment;
    if (IsDepthOnly(format))
        attachment = GL_DEPTH_ATTACHMENT;
    else if (IsDepthAndStencil(format))
        attachment = GL_DEPTH_STENCIL_ATTACHMENT;
    else
        BPAssert(false, "Attaching Renderbuffer for FBO, but format isn't supported");
    glNamedFramebufferRenderbuffer(glPtr.Get(), attachment,
                                   GL_RENDERBUFFER, depthBuffer.value().GetOglPtr().Get());
}
void Target::DeleteDepthPlaceholder()
{
    depthBuffer.reset();
}

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
            if (GetOutput_Color((uint32_t)i)->GetTex() == tex)
                OutputRemove_Color((uint32_t)i);

        //Remove it from depth/stencil attachments if applicable.
        if (GetOutput_Stencil() != nullptr && GetOutput_Stencil()->GetTex() == tex)
            OutputRemove_Stencil();
        if (GetOutput_Depth() != nullptr && GetOutput_Depth()->GetTex() == tex)
            OutputRemove_Depth(true);
    }
}


//Guide to clearing FBO's in OpenGL: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glClearBuffer.xhtml

void Target::ClearColor(const glm::fvec4& rgba, uint32_t index)
{
    BPAssert(index < GetNColorOutputs(), "Not enough color outputs to clear");
    BPAssert(!GetOutput_Color(index)->GetTex()->GetFormat().IsInteger(),
             "Trying to clear an int/uint texture with a float value");

    //Currently there's a bug in GLEW where the rgba array in the below OpenGL call
    //    isn't marked 'const', even though it should be.
    //For now we're making a copy of the data and passing it as non-const,
    //    but there's already a fix in GLEW and we should probably integrate
    //    GLEW 2.2.0 once it comes out.
    //TODO: Update to GLEW 2.2.0 when available: https://github.com/nigels-com/glew
    auto rgbaCopy = rgba;

    glClearNamedFramebufferfv(glPtr.Get(), GL_COLOR, index, glm::value_ptr(rgbaCopy));
}
void Target::ClearColor(const glm::uvec4& rgba, uint32_t index)
{
    BPAssert(index < GetNColorOutputs(), "Not enough color outputs to clear");
    BPAssert(GetOutput_Color(index)->GetTex()->GetFormat().GetComponentType() ==
                 +FormatTypes::UInt,
             "Trying to clear a non-UInt texture with a uint value");

    glClearNamedFramebufferuiv(glPtr.Get(), GL_COLOR, index, glm::value_ptr(rgba));
}
void Target::ClearColor(const glm::ivec4& rgba, uint32_t index)
{
    BPAssert(index < GetNColorOutputs(), "Not enough color outputs to clear");
    BPAssert(GetOutput_Color(index)->GetTex()->GetFormat().GetComponentType() ==
                 +FormatTypes::Int,
             "Trying to clear a non-Int texture with an int value");

    glClearNamedFramebufferiv(glPtr.Get(), GL_COLOR, index, glm::value_ptr(rgba));
}

void Target::ClearDepth(float depth)
{
    glClearNamedFramebufferfv(glPtr.Get(), GL_DEPTH, 0, &depth);
}
void Target::ClearStencil(uint32_t value)
{
    GLint valueI = Reinterpret<uint32_t, GLint>(value);
    glClearNamedFramebufferiv(glPtr.Get(), GL_STENCIL, 0, &valueI);
}
void Target::ClearDepthStencil(float depth, uint32_t stencil)
{
    auto stencilI = Reinterpret<uint32_t, GLint>(stencil);
    glClearNamedFramebufferfi(glPtr.Get(), GL_DEPTH_STENCIL, 0,
                              depth, stencilI);
}