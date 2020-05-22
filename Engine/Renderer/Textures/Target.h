#pragma once

#include <memory>

#include "TextureD.hpp"
#include "TextureCube.h"
#include "TargetBuffer.h"


namespace Bplus::GL::Textures
{
    //A reference to part or all of a texture, to be used in a render Target.
    struct BP_API TargetOutput
    {
    public:

        //0 represents the original texture; subsequent values
        //    represent smaller mip levels.
        uint_mipLevel_t MipLevel = 0;


        TargetOutput(Texture1D* tex, uint_mipLevel_t mipLevel = 0);
        TargetOutput(Texture2D* tex, uint_mipLevel_t mipLevel = 0);

        TargetOutput(std::tuple<Texture3D*, size_t> zSliceOfTex3D, uint_mipLevel_t mipLevel = 0);
        TargetOutput(std::tuple<TextureCube*, CubeFaces> faceOfCube, uint_mipLevel_t mipLevel = 0);

        TargetOutput(Texture3D* tex, uint_mipLevel_t mipLevel = 0);
        TargetOutput(TextureCube* tex, uint_mipLevel_t mipLevel = 0);


        Texture* GetTex() const;
        glm::uvec2 GetSize() const;

        //Gets whether this output has multiple layers (e.x. a full 3D texture).
        bool IsLayered() const;
        //Gets the number of layers in this output.
        //Non-layered outputs (e.x. 1D or 2D textures) only have 1.
        size_t GetLayerCount() const;
        //Returns which "layer" of the texture to use.
        //Returns 0 if there is only 1 layer available (i.e. 1D or 2D texture).
        //Fails if Islayered() is true.
        size_t GetLayer() const;


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

        bool operator==(const TargetOutput& other) const { return data == other.data; }

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

    //Status codes for Targets indicating whether they can be used for rendering.
    BETTER_ENUM(TargetStates, uint8_t,
        //Everything is fine and the Target can be used.
        Ready,
        //One of the attachments uses a "compressed" format,
        //    which generally can't be rendered into.
        CompressedFormat,
        //Some attachments are "layered" (e.x. full 3D textures or cubemaps), and some aren't.
        LayerMixup,
        //The graphics driver on your machine doesn't support this particular combination
        //    of texture formats; try simpler ones.
        DriverUnsupported,
        //Some other unknown error has occurred. Run in Debug mode to see an assert fail.
        Unknown
    );


    //A set of textures that can be rendered into -- color(s), depth, and stencil.
    //Can optionally manage the lifetime of textures attached to it.
    //The textures are specified via the "TargetOutput" data structure above.
    class BP_API Target
    {
    public:
        //TODO: A special singleton Target representing the screen. Replace a lot of the state in Context with this.

        //Creates a new Target with no outputs.
        //Optionally acts like a "layered" Target, supporting multiple Geometry Shader outputs,
        //    even before any layered Outputs are attached to it.
        Target(const glm::uvec2& size, size_t nLayers = 1);

        //Creates a new Target with the given size and output formats.
        //Automatically creates and takes ownership of the output textures.
        Target(const glm::uvec2& size,
               const Format& colorFormat, DepthStencilFormats depthFormat,
               bool depthIsRenderbuffer = true,
               uint_mipLevel_t nMips = 0);

        //Creates a Target with the given color and depth outputs.
        Target(const TargetOutput& color, const TargetOutput& depth);
        //Creates a target with the given color output and a depth renderbuffer.
        Target(const TargetOutput& color,
               DepthStencilFormats depthFormat = DepthStencilFormats::Depth_24U);

        //Creates a target with the given color outputs and depth output.
        //If no depth output is passed, a renderbuffer is used
        //    with the Depth_24U format.
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
        //Gets the number of color outputs currently in this target.
        size_t GetNColorOutputs() const { return tex_colors.size(); }


        //Checks the status of this Target.
        TargetStates Validate() const;


        #pragma region Managing "output" texture attachments

        //Functions to get various outputs.
        //They all return nullptr if the output doesn't exist.
        const TargetOutput* GetOutput_Color(size_t index = 0) const { return (index < tex_colors.size()) ? &tex_colors[index] : nullptr; }
        const TargetOutput* GetOutput_Depth() const { return tex_depth.has_value() ? &tex_depth.value() : nullptr; }
        const TargetOutput* GetOutput_Stencil() const { return tex_stencil.has_value() ? &tex_stencil.value() : nullptr; }
        const TargetOutput* GetOutput_DepthStencil() const
        {
            if (tex_depth.has_value() && tex_stencil.has_value() &&
                tex_depth.value() == tex_stencil.value())
            {
                return &tex_depth.value();
            }
            else
            {
                return nullptr;
            }
        }

        //The below functions set this target's outputs.
        void OutputSet_Color(const TargetOutput& newOutput, size_t index = 0, bool takeOwnership = false);
        void OutputSet_Depth(const TargetOutput& newOutput, bool takeOwnership = false);
        void OutputSet_Stencil(const TargetOutput& newOutput, bool takeOwnership = false);
        void OutputSet_DepthStencil(const TargetOutput& newOutput, bool takeOwnership = false);

        //The below functions remove an output from this target.
        //If the output was managed by this target, it will be deleted automatically.
        //If removing the depth buffer, you can optionally have it replaced with
        //    a render-buffer (like a texture, but not usable in other things like shaders).
        void OutputRemove_Color(size_t index = 0);
        void OutputRemove_Depth(bool attachRenderBuffer = true);
        void OutputRemove_Stencil();
        void OutputRemove_DepthStencil(bool attachRenderBuffer = true);

        //Releases ownership over the given texture.
        //The texture must have been given to this target previously.
        //Optionally also removes the texture from any of this Target's Outputs.
        //If removing it from the depth Output, it will be automatically replaced
        //    with a renderbuffer of the same format.
        void OutputRelease(Texture* tex, bool removeOutputs = false);


        //Uses a special texture for this target's depth
        //    which works for rendering, but can't be sampled by another shader.
        //If the special texture has been used before, it will be re-used
        //    regardless of the "format" argument.
        void AttachDepthPlaceholder(DepthStencilFormats format = DepthStencilFormats::Depth_24U);
        //Cleans up the depth placeholder if one exists.
        //The next time it's needed, it will be recreated at a new size/format.
        //If it's currently being used, it will automatically be removed.
        void DeleteDepthPlaceholder();

        #pragma endregion

        //TODO: implement Clearing.
        #pragma region Clearing


        #pragma endregion


    private:

        OglPtr::Target glPtr;
        glm::uvec2 size;

        std::vector<TargetOutput> tex_colors;
        std::optional<TargetOutput> tex_depth;
        std::optional<TargetOutput> tex_stencil;

        //TODO: Other state that comes with FrameBuffers.

        std::optional<TargetBuffer> depthBuffer;
        bool isDepthRBBound = false,
             isStencilRBBound = false;

        //Note that "managedTextures" originally contained unique_ptr instances,
        //    but that made it more complicated/less efficient to search through it.
        std::unordered_set<Texture*> managedTextures;

        void TakeOwnership(Texture* tex);
        void ReleaseOwnership(Texture* tex);
        void HandleRemoval(Texture* tex);
        void RecomputeSize();

        void AttachAt(GLenum attachment, const TargetOutput& output);
        void RemoveAt(GLenum attachment);
    };
}