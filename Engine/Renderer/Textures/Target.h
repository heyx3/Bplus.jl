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

        TargetOutput(std::tuple<Texture3D*, uint32_t> zSliceOfTex3D, uint_mipLevel_t mipLevel = 0);
        TargetOutput(std::tuple<TextureCube*, CubeFaces> faceOfCube, uint_mipLevel_t mipLevel = 0);

        TargetOutput(Texture3D* tex, uint_mipLevel_t mipLevel = 0);
        TargetOutput(TextureCube* tex, uint_mipLevel_t mipLevel = 0);


        Texture* GetTex() const;
        glm::uvec2 GetSize() const;

        //Gets whether this output has multiple layers (e.x. a full 3D texture).
        bool IsLayered() const;
        //Gets whether this output represents an entire un-layered texture,
        //    i.e. not 3D or cubemap.
        bool IsFlat() const;

        //Gets the number of layers in this output.
        //Non-layered outputs (e.x. 1D or 2D textures) only have 1.
        uint32_t GetLayerCount() const;
        //Returns which "layer" of the texture to use.
        //Returns 0 if there is only 1 layer available (i.e. 1D or 2D texture).
        //Fails if Islayered() is true.
        uint32_t GetLayer() const;


        bool IsTex1D() const { return std::holds_alternative<Texture1D*>(data); }
        bool IsTex2D() const { return std::holds_alternative<Texture2D*>(data); }
        bool IsTex3D() const { return std::holds_alternative<Texture3D*>(data); }
        bool IsTexCube() const { return std::holds_alternative<TextureCube*>(data); }
        bool IsTex3DSlice() const { return std::holds_alternative<std::tuple<Texture3D*, uint32_t>>(data); }
        bool IsTexCubeFace() const { return std::holds_alternative<std::tuple<TextureCube*, CubeFaces>>(data); }

        Texture1D* GetTex1D() const { return std::get<Texture1D*>(data); }
        Texture2D* GetTex2D() const { return std::get<Texture2D*>(data); }
        Texture3D* GetTex3D() const { return std::get<Texture3D*>(data); }
        TextureCube* GetTexCube() const { return std::get<TextureCube*>(data); }
        std::tuple<Texture3D*, uint32_t> GetTex3DSlice() const { return std::get<std::tuple<Texture3D*, uint32_t>>(data); }
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
                     std::tuple<Texture3D*, uint32_t>,
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
        //Your graphics driver doesn't support this particular combination
        //    of texture formats; try simpler ones.
        DriverUnsupported,
        //Some other unknown error has occurred. Run in Debug mode to see an assert fail.
        Unknown
    );


    //A set of textures that can be rendered into -- color(s), depth, and stencil.
    //The textures are specified via the "TargetOutput" data structure above.
    //Once created, the Target's attachments are immutable.
    class BP_API Target
    {
    public:

        //Here is the process for OpenGL Framebuffers:
        //   * Create it
        //       * Attach textures/images to it (possibly layered)
        //            with glNamedFramebufferTexture[Layer]()
        //   * Use it
        //       * Set which attachments to use for color outputs
        //            with glNamedFramebufferDrawBuffers()
        //       * Depth/stencil outputs are already taken directly from the attachments
        //Our implementation makes the attachments immutable to simplify things,
        //    although you can still change which draw buffers are active at any one time.

        //TODO: Implement Copying: http://docs.gl/gl4/glBlitFramebuffer
        //TODO: A special singleton Target representing the screen.

        //Finds the Target from the given OpenGL object pointer.
        //Only works on the main OpenGL thread.
        //Returns nullptr if not found.
        static const Target* Find(OglPtr::Target ptr);


        //Creates a Target with no outputs,
        //    which pretends to have the given size and (optionally) a number of layers.
        Target(TargetStates& outStatus,
               const glm::uvec2& size, uint32_t nLayers = 1);

        //Creates a new Target with the given output size/formats.
        //Will create corresponding textures, which are then destroyed in the destructor.
        //By default, uses a "renderbuffer" for depth, meaning
        //    it isn't a true texture and can't be sampled or modified by the user.
        Target(TargetStates& outStatus,
               const glm::uvec2& size,
               const Format& colorFormat, DepthStencilFormats depthFormat,
               bool depthIsRenderbuffer = true,
               uint_mipLevel_t nMips = 0);

        //Creates a Target with the given color and depth outputs.
        //The given textures are NOT automatically cleaned up
        //    when this target is destroyed.
        Target(TargetStates& outStatus,
               const TargetOutput& color, const TargetOutput& depth);
        //Creates a target with the given color output and a depth buffer.
        //The color texture is NOT automatically cleaned up when this target is destroyed.
        Target(TargetStates& outStatus,
               const TargetOutput& color,
               DepthStencilFormats depthFormat = DepthStencilFormats::Depth_24U);

        //Creates a target with the given color outputs and depth output.
        //Note that the given textures are not managed by this Target;
        //    they will not be cleaned up when this Target is destroyed.
        //If no depth output is passed, an internal renderbuffer is used
        //    with the Depth_24U format.
        Target(TargetStates& outStatus,
               const TargetOutput* colorOutputs, uint32_t nColorOutputs,
               std::optional<TargetOutput> depthOutput = std::nullopt);

        ~Target();


        //No copying, but you can move.
        Target(const Target& cpy) = delete;
        Target& operator=(const Target& cpy) = delete;
        Target(Target&& from);
        Target& operator=(Target&& from)
        {
            //Only bother changing things if they represent different handles.
            if (this != &from)
            {
                //Call deconstructor, then move constructor.
                this->~Target();
                new (this) Target(std::move(from));
            }

            return *this;
        }


        //Tells this Target which subset of its color attachments to use during drawing,
        //    and the order of those attachments.
        //The order of this list matches the order of fragment shader outputs.
        //If an entry is missing, then nothing happens when a fragment shader writes to that output.
        void SetDrawBuffers(const std::optional<uint32_t>* attachmentList, size_t nAttachments);
        //Tells this Target which subset of its color attachments to use during drawing,
        //    and the order of those attachments.
        //The order of this list matches the order of fragment shader outputs.
        //If an entry is missing, then nothing happens when a fragment shader writes to that output.
        void SetDrawBuffers(const std::vector<std::optional<uint32_t>>& attachments) { SetDrawBuffers(attachments.data(), attachments.size()); }
        
        //Gets the current subset of color attachments that are actually used
        //    when rendering to this Target.
        const std::vector<std::optional<uint32_t>>& GetDrawBuffers() const { return activeColorAttachments; }
        //Gets the number of colors outputs this target is currently set to use.
        const size_t GetNDrawBuffers() const { return activeColorAttachments.size(); }


        //The size of this Target is equal to the smallest size of its outputs.
        glm::uvec2 GetSize() const { return size; }
        //Gets the number of color attachments in this target.
        uint32_t GetNColorOutputs() const { return (uint32_t)tex_colors.size(); }

        //Gives this Target ownership over the given Texture,
        //    so that it gets cleaned up as soon as this Target is destroyed.
        void TakeOwnership(Texture* tex) { managedTextures.insert(tex); }
        //Gets the OpenGL handle to this texture.
        OglPtr::Target GetGlPtr() const { return glPtr; }


        //Functions to get various outputs.
        //They all return nullptr if the output doesn't exist.

        const TargetOutput* GetOutput_Color(uint32_t index = 0) const;
        const TargetOutput* GetOutput_Depth() const { return tex_depth.has_value() ? &tex_depth.value() : nullptr; }
        const TargetOutput* GetOutput_Stencil() const { return tex_stencil.has_value() ? &tex_stencil.value() : nullptr; }
        const TargetOutput* GetOutput_DepthStencil() const { return (tex_depth.has_value() &&
                                                                     tex_stencil.has_value() &&
                                                                     (tex_depth.value() == tex_stencil.value()))
                                                                       ? &tex_depth.value() :
                                                                         nullptr; }

        //Gets the texture attached to this Target at the given index.
        //Note that this is different than the Target's current color outputs;
        //    this is the pool of color textures that those outputs are chosen from.
        //Returns nullptr if the output doesn't exist.
        const TargetOutput* GetColorAttachment(uint32_t index = 0) const { return (index < tex_colors.size()) ? &tex_colors[index] : nullptr; }


        #pragma region Clear functions

        //Clears a color buffer that has a floating-point or normalized integer format.
        void ClearColor(const glm::fvec4& rgba, uint32_t index = 0);

        //Clears a color buffer that has a UInteger format.
        void ClearColor(const glm::uvec4& rgba_UInt, uint32_t index = 0);
        //Clears a color buffer that has an Integer format.
        void ClearColor(const glm::ivec4& rgba_Int, uint32_t index = 0);
         
        void ClearDepth(float depth = 1.0f);
        void ClearStencil(uint32_t value = 0);
        void ClearDepthStencil(float depth = 1.0f, uint32_t stencil = 0);

        #pragma endregion


    private:

        OglPtr::Target glPtr;
        glm::uvec2 size;

        //Attachments:  
        std::vector<TargetOutput> tex_colors;
        std::optional<TargetOutput> tex_depth;
        std::optional<TargetOutput> tex_stencil;
        //Color attachment management:
        std::vector<std::optional<uint32_t>> activeColorAttachments;
        std::vector<GLenum> internalActiveColorAttachments;

        //Renderbuffer management:
        std::optional<TargetBuffer> depthBuffer;
        bool isDepthRBBound = false, //Is the internal TargetBuffer bound to depth?
             isStencilRBBound = false; //Is the internal TargetBuffer bound to stencil?

        //The textures that this Target should clean up on destruction.
        //Note that this originally contained unique_ptr instances,
        //    but apparently that causes problems with sets/maps.
        std::unordered_set<Texture*> managedTextures;


        void RecomputeSize();
        TargetStates GetStatus() const;


        void AttachTexture(GLenum attachment, const TargetOutput& output);
        void AttachBuffer(DepthStencilFormats format);

        static GLenum GetAttachmentType(DepthStencilFormats format);
    };
}