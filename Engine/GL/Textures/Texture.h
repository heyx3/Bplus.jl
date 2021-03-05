#pragma once

#include <numeric>
#include <array>

#include "TexturesData.h"
#include "Format.h"


namespace Bplus::GL::Textures
{
    class Texture;
    struct TexView;
    struct ImgView;


    #pragma region TexHandle and ImgHandle

    //A helper class for Texture, representing a bindless handle.
    //Should not and cannot be used outside of that class,
    //    but it can't be nested in Texture due to forward-declaration problems.
    struct BP_API TexHandle
    {
        const OglPtr::Sampler SamplerGlPtr = OglPtr::Sampler::Null();
        const OglPtr::View ViewGlPtr;

        //No copying, but moves are fine.
        TexHandle(const TexHandle& cpy) = delete;
        TexHandle& operator=(const TexHandle& cpy) = delete;
        TexHandle(TexHandle&& src);
        TexHandle& operator=(TexHandle&& src)
        {
            //Call deconstructor, then move constructor.
            //Only bother changing things if they represent different handles.
            if (this != &src)
            {
                this->~TexHandle();
                new (this) TexHandle(std::move(src));
            }

            return *this;
        }

        void Activate();
        void Deactivate();

        bool IsActive() const { return activeCount > 0; }


    private:

        friend class Texture;
        friend struct TexView;
        friend struct std::default_delete<TexHandle>;
        friend class std::unique_ptr<TexHandle>;

        TexHandle(const Texture* src);
        TexHandle(const Texture* src, const Sampler<3>& sampler3D);
        ~TexHandle();

        uint_fast32_t activeCount = 0;
        bool skipDestructor = false;
    };


    //Represents the parameters that come with an "ImgView".
    struct BP_API ImgHandleData
    {
        uint_mipLevel_t MipLevel;
        std::optional<uint_fast32_t> SingleLayer;
        ImageAccessModes Access;

        ImgHandleData(ImageAccessModes access = ImageAccessModes::ReadWrite,
                      std::optional<uint_fast32_t> singleLayer = std::nullopt,
                      uint_mipLevel_t mipLevel = 0)
            : MipLevel(mipLevel), SingleLayer(singleLayer), Access(access) { }

        bool operator==(const ImgHandleData& other) const
        {
            return (SingleLayer == other.SingleLayer) &
                   (Access == other.Access) &
                   (MipLevel == other.MipLevel);
        }
        bool operator!=(const ImgHandleData& other) const { return !operator==(other); }
    };

    //A helper class for Texture, representing a bindless handle.
    //Should not and cannot be used outside of that class,
    //    but it can't be nested in Texture due to forward-declaration problems.
    struct BP_API ImgHandle
    {
        const OglPtr::View ViewGlPtr;
        const ImgHandleData Params;

        //No copying, but moves are fine.
        ImgHandle(const ImgHandle& cpy) = delete;
        ImgHandle& operator=(const ImgHandle& cpy) = delete;
        ImgHandle(ImgHandle&& src);
        ImgHandle& operator=(ImgHandle&& src)
        {
            //Call deconstructor, then move constructor.
            //Only bother changing things if they represent different handles.
            if (this != &src)
            {
                this->~ImgHandle();
                new (this) ImgHandle(std::move(src));
            }

            return *this;
        }

        void Activate();
        void Deactivate();

        bool IsActive() const { return activeCount > 0; }


    private:

        friend class Texture;
        friend struct ImgView;
        friend struct std::default_delete<ImgHandle>;
        friend class std::unique_ptr<ImgHandle>;

        ImgHandle(const Texture* src, const ImgHandleData& params);
        ~ImgHandle();


        uint_fast32_t activeCount = 0;
        bool skipDestructor = false;
    };

    #pragma endregion


    //A texture combined with a custom sampler.
    struct BP_API TexView
    {
        const OglPtr::View GlPtr;

        const Texture& Owner;
        TexHandle& Handle;

        TexView(const Texture& owner, TexHandle& handle);
        ~TexView();

        TexView(const TexView& cpy) : TexView(cpy.Owner, cpy.Handle) { }
        TexView& operator=(const TexView& cpy);
        TexView(TexView&& from) : TexView(from) { }
        TexView& operator=(TexView&& from) { return operator=(from); }

        bool operator==(const TexView& v) const { return (GlPtr == v.GlPtr); }
        bool operator!=(const TexView& v) const { return (GlPtr != v.GlPtr); }
    };

    //A specific mip-level of a texture,
    //    for direct reading and writing (no sampling).
    struct BP_API ImgView
    {
        const OglPtr::View GlPtr;

        const Texture& Owner;
        ImgHandle& Handle;

        ImgView(const Texture& owner, ImgHandle& handle);
        ~ImgView();

        ImgView(const ImgView& cpy) : ImgView(cpy.Owner, cpy.Handle) { }
        ImgView& operator=(const ImgView& cpy);
        ImgView(ImgView&& from) : ImgView(from) { }
        ImgView& operator=(ImgView&& from) { return operator=(from); }

        bool operator==(const ImgView& v) const { return (GlPtr == v.GlPtr); }
        bool operator!=(const ImgView& v) const { return (GlPtr != v.GlPtr); }
    };
}
BP_HASHABLE_SIMPLE(Bplus::GL::Textures::ImgHandleData,
                   d.Access, d.MipLevel, d.SingleLayer)

namespace Bplus::GL::Textures
{
    //The base class for all OpenGL textures.
    //Designed to be used with OpenGL's Bindless Textures extension.
    class BP_API Texture
    {
    public:

        Texture(Types type, Format format, uint_mipLevel_t nMipLevels,
                const Sampler<3>& sampler3D,
                SwizzleRGBA swizzling = DefaultSwizzling(),
                std::optional<DepthStencilSources> depthStencilMode = std::nullopt);

        virtual ~Texture();
        
        //No copying.
        Texture(const Texture& cpy) = delete;
        Texture& operator=(const Texture& cpy) = delete;


        const Format& GetFormat() const { return format; }
        const SwizzleRGBA& GetSwizzling() const { return swizzling; }

        //Gets a 3D version of this texture's sampler.
        //If this texture is less than 3-dimensional,
        //    then you should ignore that higher-dimensional data.
        const Sampler<3>& GetSamplerFull() const { return sampler3D; }

        Types GetType() const { return type; }
        uint_mipLevel_t GetNMipLevels() const { return nMipLevels; }
        OglPtr::Texture GetOglPtr() const { return glPtr; }

        //Change the values coming out of this texture when it's sampled in a shader.
        //For example, you could swap the Red and Blue values,
        //    or replace the Alpha with a constant 1.
        //This does not change the actual pixel data; merely how it's sampled.
        void SetSwizzling(const SwizzleRGBA& newSwizzling);
        //Change how this depth/stencil hybrid texture can be sampled.
        //You can sample the depth OR the stencil, but not both at once.
        void SetDepthStencilSource(DepthStencilSources newValue);


        //Gets the number of bytes needed to store one mip leve of this texture
        //    in its native format.
        virtual size_t GetByteSize(uint_mipLevel_t mipLevel = 0) const = 0;
        //Gets the total byte-size of this texture's data, across all mip levels.
        virtual size_t GetTotalByteSize() const;


        //Updates mipmaps for this texture.
        //Not allowed for compressed-format textures.
        void RecomputeMips();
        
        //Gets (or creates) an "image" view of this texture,
        //    allowing simple reads/writes but no sampling.
        ImgView GetView(ImgHandleData params) const;

        //Gets (or creates) a view of this texture with the given 3D sampler.
        //Child classes should provide a public "GetView() with
        //    the correct-dimensional sampler.
        TexView GetViewFull(std::optional<Sampler<3>> customSampler = std::nullopt) const;


    protected:

        //Child classes have access to the move constructor.
        Texture(Texture&& src);
        Texture& operator=(Texture&& src) = delete;
        

        //Given a set of components for texture uploading/downloading,
        //    and the data type of this texture's pixels,
        //    finds the corresponding OpenGL enum value.
        GLenum GetOglChannels(PixelIOChannels components) const
        {
            if (!format.IsInteger())
                return (GLenum)components;
            else
                return GetIntegerVersion(components);
        }


    private:

        OglPtr::Texture glPtr;
        Types type;
        uint_mipLevel_t nMipLevels;

        Format format;
        Sampler<3> sampler3D;
        SwizzleRGBA swizzling;
        std::optional<DepthStencilSources> depthStencilMode;

        //Texture views represent different ways of sampling from this texture in a shader.
        //This field is a cache of the views that have already been created.
        //They are stored as unique_ptr so that their pointer doesn't change.
        mutable std::unordered_map<Sampler<3>, std::unique_ptr<TexHandle>> texHandles;

        //Image views represent different parts of this texture for shaders to read/write.
        //This field is a cache of the views that have already been created.
        //They are stored as unique_ptr so that their pointer doesn't change.
        mutable std::unordered_map<ImgHandleData, std::unique_ptr<ImgHandle>> imgHandles;
    };
}