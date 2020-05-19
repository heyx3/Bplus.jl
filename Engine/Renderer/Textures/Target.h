#pragma once

#include "TextureD.hpp"
#include "TextureCube.h"


namespace Bplus::GL::Textures
{
    //TODO: A "TargetOutput" struct representing various texture types, and being layered/non-layered.


    //A set of textures that can be rendered into.
    //Can optionally manage the lifetime of the textures attached to it.
    class BP_API Target
    {
    public:
        //TODO: A special singleton Target representing the screen. Replace a lot of the state in Context with this.


        Target(const glm::uvec2& size);
        Target(const Texture* colorTextures, size_t nColorTextures,
               const Texture* depth = nullptr,
               const Texture* stencil = nullptr);

        ~Target();


        //Note that the size of this Target is equal to the smallest size of its outputs.
        glm::uvec2 GetSize() const { return size; }

        const Sampler<2>& GetSampler() const { return sampler; }
        void SetSampler(const Sampler<2>& newSampler);


        //Gets whether this Target can be used for rendering yet.
        bool IsValid() const;


        #pragma region Managing "output" texture attachments

        //TODO: Remake the below. Set attachments with glNamedFramebufferTexture(). Handle layered vs non-layered.

        //NOTE: If attaching something with multiple 2D slices (cubemap, 3D texture, etc),
        //    this Target becomes "layered", meaning that:
        //    a) All attachments must be layered
        //    b) The Geometry Shader can render to each 2D "layer" at once,
        //          on top of the usual ability of fragment shaders
        //          to output to multiple color attachments.


        //There are five types of functions for managing attachments:
        //    "OutputSet...(Texture* tex, ...)" to set the output without taking ownership
        //    "OutputSet...(Texture&& tex, ...)" to set the output and take ownership
        //    "OutputRemove...(...)" to remove the output (and delete it if owned by this Target)
        //    "OutputRelease...(...)" to release ownership of the output (and optionally "Remove" it too)
        //    "OutputGet...(...)" to get a given output texture (or nullptr if nonexistent)
        //Each of those five has four different versions for the various attachment types:
        //    Color, Depth, Stencil, and Depth/Stencil hybrid.
        //The Color type has an extra parameter for which color attachment to modify.
        //Each of THOSE four has multiple versions for each of the texture types.


        #define FUNC_PER_TEX_TYPE(name, args, useArgs, baseFunc) \
            void Name(Texture1D* tex1D, args, uint_mipLevel_t mipLevel = 0) \
                { baseFunc(tex1D, 1, useArgs, ); }

        //The below functions set this target's outputs,
        //    without transferring ownership of the texture.
        void OutputSet_Color(Texture* tex, size_t index = 0);
        void OutputSet_Depth(Texture* tex);
        void OutputSet_Stencil(Texture* tex);
        void OutputSet_DepthStencil(Texture* tex);

        //The below functions set this target's outputs,
        //    and give this Target ownership over the texture.
        void OutputSet_Color(Texture&& tex, size_t index = 0);
        void OutputSet_Depth(Texture&& tex);
        void OutputSet_Stencil(Texture&& tex);
        void OutputSet_DepthStencil(Texture&& tex);

        //The below functions remove an output from this target.
        //If the output was managed by this target, it will be deleted automatically.
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
        Sampler<2> sampler;

        std::vector<Texture*> tex_colors;
        Texture* tex_depth = nullptr;
        Texture* tex_stencil = nullptr;

        //TODO: Other state that comes with FrameBuffers.

        OglPtr::TargetBuffer glPtr_DepthRenderBuffer = OglPtr::TargetBuffer{ OglPtr::TargetBuffer::Null };
        std::unordered_set<std::unique_ptr<Texture>> managedTextures;
    };
}