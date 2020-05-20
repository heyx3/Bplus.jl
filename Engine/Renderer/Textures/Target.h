#pragma once

#include <memory>

#include "TextureD.hpp"
#include "TextureCube.h"


namespace Bplus::GL::Textures
{
    //A reference to part or all of a texture, to be used in a render Target.
    struct BP_API TargetOutput
    {
    public:

        //If true, the render target takes ownership of this instance's texture,
        //    meaning it gets auto-deleted as soon as the Target stops using it.
        bool TakeOwnership = false;

        TargetOutput(Texture1D* tex, bool takeOwnership = false);
        TargetOutput(Texture2D* tex, bool takeOwnership = false);

        TargetOutput(Texture3D* tex, size_t zSlice, bool takeOwnership = false);
        TargetOutput(TextureCube* tex, CubeFaces face, bool takeOwnership = false);

        TargetOutput(Texture3D* tex, bool takeOwnership = false);
        TargetOutput(TextureCube* tex, bool takeOwnership = false);

        bool IsLayered() const;
        Texture* GetTex() const;

        bool IsTex1D() const { return std::holds_alternative<Texture1D*>(data); }
        bool IsTex2D() const { return std::holds_alternative<Texture2D*>(data); }
        bool IsTex3D() const { return std::holds_alternative<Texture3D*>(data); }
        bool IsTexCube() const { return std::holds_alternative<TextureCube*>(data); }
        bool IsTex3DSlice() const { return std::holds_alternative<std::tuple<Texture3D*, size_t>>(data); }
        bool IsTexCubeFace() const { return std::holds_alternative<std::tuple<TextureCube*, CubeFaces>>(data); }

        Texture1D* GetTex1D() const { return std::get<Texture1D*>(data); }
        Texture2D* GetTex2D() const { return std::get<Texture2D*>(data); }
        Texture3D* GetTex3D() const { return std::get<Texture3D*>(data); }
        TextureCube* GetTexCube() const { return std::get<TextureCube*>(data); }
        std::tuple<Texture3D*, size_t> GetTex3DSlice() const { return std::get<std::tuple<Texture3D*, size_t>>(data); }
        std::tuple<TextureCube*, CubeFaces> GetTexCubeFace() const { return std::get<std::tuple<TextureCube*, CubeFaces>>(data); }

    private:

        //The allowable attachments:
        //   * 1D texture (treated as a 2D texture of height 1)
        //   * 2D texture (pretty self-explanatory)
        //   * 3D texture at a specific Z-slice, treated as a 2D texture
        //   * Cubemap texture on a specific face, treated as a 2D texture.
        //   * An entire 3D texture. This makes the Target "layered", allowing you to output to 1 or more Z-slices at once
        //   * An entire cubemap. This makes the Target "layered", allowing you to output to 1 or more faces at once
        std::variant<Texture1D*, Texture2D*,
                     std::tuple<Texture3D*, size_t>,
                     Texture3D*, TextureCube*,
                     std::tuple<TextureCube*, CubeFaces>
                    > data;
    };


    //A set of textures that can be rendered into.
    //Can optionally manage the lifetime of textures attached to it.
    class BP_API Target
    {
    public:
        //TODO: A special singleton Target representing the screen. Replace a lot of the state in Context with this.

        //Creates a new Target with no outputs.
        Target(const glm::uvec2& size);

        //Creates a new Target with the given size and output formats.
        //Automatically creates and takes ownership of the necessary textures.
        Target(const glm::uvec2& size,
               const Format& colorFormat, DepthStencilFormats depthFormat,
               bool depthIsRenderbuffer = true,
               uint_mipLevel_t nMips = 0);

        //Creates a Target with the given color and depth outputs.
        Target(const TargetOutput& color, const TargetOutput& depth);
        //Creates a target with the given color output and a depth renderbuffer.
        Target(const TargetOutput& color,
               DepthStencilFormats depthFormat = DepthStencilFormats::Depth_32U);

        //Creates a target with the given color outputs and depth output.
        //If no depth output is passed, a renderbuffer is used
        //    with the Depth_32U format.
        Target(const TargetOutput* colorOutputs, size_t nColorOutputs,
               std::optional<TargetOutput> depthOutput = std::nullopt);

        ~Target();


        //No copy, but you can move.
        Target(const Target& cpy) = delete;
        Target& operator=(const Target& cpy) = delete;
        Target(Target&& from);
        Target& operator=(Target&& from)
        {
            //Deconstruct then move-construct in place.
            this->~Target();
            new (this) Target(std::move(from));

            return *this;
        }


        //The size of this Target is equal to the smallest size of its outputs.
        glm::uvec2 GetSize() const { return size; }


        //Gets whether this Target can be used for rendering yet.
        bool IsValid() const;


        #pragma region Managing "output" texture attachments

        //The below functions set this target's outputs.
        void OutputSet_Color(const TargetOutput& newOutput, size_t index = 0);
        void OutputSet_Depth(const TargetOutput& newOutput);
        void OutputSet_Stencil(const TargetOutput& newOutput);
        void OutputSet_DepthStencil(const TargetOutput& newOutput);

        //The below functions remove an output from this target.
        //If the output was managed by this target, it will be deleted automatically.
        //If removing the depth buffer, you can optionally have it replaced with
        //    a render-buffer (like a texture, but not usable in other things like shaders).
        void OutputRemove_Color(size_t index = 0);
        void OutputRemove_Depth(bool attachRenderBuffer = true);
        void OutputRemove_Stencil();
        void OutputRemove_DepthStencil(bool attachRenderBuffer = true);

        //Releases ownership over the given texture.
        //The texture must have been given to this texture previously.
        //Optionally also removes the texture as an output
        //    (make sure you get a reference to it before doing this!)
        void OutputRelease_Color(size_t index = 0, bool alsoRemove = false);
        void OutputRelease_Depth(bool alsoRemove = false);
        void OutputRelease_Stencil(bool alsoRemove = false);
        void OutputRelease_DepthStencil(bool alsoRemove = false);


        //Uses a special texture for this target's depth
        //    which works for rendering, but can't be sampled by another shader.
        //If the special texture has been used before, it will be re-used
        //    regardless of the "format" argument.
        //If a depth texture is already attached, it will be released
        //    (and deleted if owned by this Target).
        void AttachDepthPlaceholder(DepthStencilFormats format = DepthStencilFormats::Depth_24U);

        #pragma endregion

        //TODO: implement Clearing.
        #pragma region Clearing


        #pragma endregion


    private:

        OglPtr::Target glPtr;
        glm::uvec2 size;

        std::vector<Texture*> tex_colors;
        Texture* tex_depth = nullptr;
        Texture* tex_stencil = nullptr;
        bool isDepthStencilBound = false;

        //TODO: Other state that comes with FrameBuffers.

        OglPtr::TargetBuffer glPtr_DepthRenderBuffer = OglPtr::TargetBuffer{ OglPtr::TargetBuffer::Null };
        std::unordered_set<std::unique_ptr<Texture>> managedTextures;
    };
}