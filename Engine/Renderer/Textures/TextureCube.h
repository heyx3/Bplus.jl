#pragma once

#include "Texture.h"

namespace Bplus::GL::Textures
{
    //The six faces of a cube, defined to match the OpenGL cubemap texture faces.
    //Note that they are ordered in the same way that OpenGL orders them.
    BETTER_ENUM(CubeFaces, GLenum,
        PosX = GL_TEXTURE_CUBE_MAP_POSITIVE_X,
        NegX = GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        PosY = GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
        NegY = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        PosZ = GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
        NegZ = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    );

    struct BP_API CubeFace
    {
        unsigned Axis : 2; //0=X, 1=Y, 2=Z
        int      Dir : 1; //Interpret 0 as -1, and 1 as +1.

        CubeFace() : CubeFace(0, 0) { }
        CubeFace(unsigned axis, int dir) : Axis(axis), Dir(dir) { }
        CubeFace(CubeFaces fromEnum)
        {
            //Figure out direction from the enum.
            switch (fromEnum)
            {
                case CubeFaces::NegX:
                case CubeFaces::NegY:
                case CubeFaces::NegZ:
                    Dir = 0;

                case CubeFaces::PosX:
                case CubeFaces::PosY:
                case CubeFaces::PosZ:
                    Dir = 1;

                default: BPAssert(false, fromEnum._to_string()); Dir = 1;
            }

            //Figure out axis from the enum.
            switch (fromEnum)
            {
                case CubeFaces::NegX:
                case CubeFaces::PosX:
                    Axis = 0;

                case CubeFaces::PosY:
                case CubeFaces::NegY:
                    Axis = 1;

                case CubeFaces::PosZ:
                case CubeFaces::NegZ:
                    Axis = 2;

                default: BPAssert(false, fromEnum._to_string()); Axis = 1;
            }
        }

        CubeFaces ToFaceEnum() const
        {
            switch (Axis)
            {
                case 0: return Dir ? CubeFaces::PosX : CubeFaces::NegX;
                case 1: return Dir ? CubeFaces::PosY : CubeFaces::NegY;
                case 2: return Dir ? CubeFaces::PosZ : CubeFaces::NegZ;
                default: BPAssert(false, "Unknown axis"); return CubeFaces::NegX;
            }
        }

        template<typename T = glm::i32>
        glm::vec<3, T> ToEdge3D() const
        {
            glm::ivec3 result{ T{0} };
            result[Axis] = T{ Dir ? 1 : -1 };
            return result;
        }
    };


    //A "Cubemap" texture, which has 6 2D textures for faces.
    class BP_API TextureCube : public Texture
    {
    public:
        static constexpr Types GetClassType() { return Types::Cubemap; }


        //Creates a new cube-map.
        //Pass "1" for nMipLevels to not use mip-maps.
        //Pass "0" for nMipLevels to generate full mip-maps down to a single pixel.
        //Pass anything else to generate a fixed amount of mip levels.
        TextureCube(const glm::uvec2& size, Format format,
                    uint_mipLevel_t nMipLevels = 0);
        virtual ~TextureCube();

        //Note that the copy constructor/operator is automatically deleted via the parent class.

        TextureCube(TextureCube&& src);
        TextureCube& operator=(TextureCube&& src);


        glm::uvec2 GetSize(uint_mipLevel_t mipLevel = 0) const;

        //Gets the number of bytes needed to store this texture in its native format.
        //This includes all six faces; divide the result by 6 to get the byte-size per face.
        size_t GetByteSize(uint_mipLevel_t mipLevel = 0) const { return 6 * format.GetByteSize(GetSize(mipLevel)); }
        //Gets the total byte-size of this texture, across all mip levels.
        size_t GetTotalByteSize() const;


        TextureMinFilters GetMinFilter() const { return minFilter; }
        TextureMagFilters GetMagFilter() const { return magFilter; }
        Sampler<2> GetSampler() const { return { minFilter, magFilter, TextureWrapping::Clamp }; }

        void SetMinFilter(TextureMinFilters filter) const;
        void SetMagFilter(TextureMagFilters filter) const;

        
        #pragma region Setting data

        //Optional parameters when uploading texture data.
        struct SetDataParams
        {
            //The subset of the texture to set.
            //A size-0 box represents the full texture.
            Math::Box2Du DestRange = Math::Box2Du::MakeCenterSize(glm::uvec2{ 0 }, glm::uvec2{ 0 });
            //The mip level. 0 is the original texture, higher values are smaller mips.
            uint_mipLevel_t MipLevel = 0;

            //If true, all mip-levels will be automatically recomputed after this operation.
            bool RecomputeMips;

            SetDataParams(bool recomputeMips = true)
                : RecomputeMips(recomputeMips) { }
            SetDataParams(const Math::Box2Du& destRange,
                          bool recomputeMips = true)
                : DestRange(destRange), RecomputeMips(recomputeMips) { }
            SetDataParams(uint_mipLevel_t mipLevel,
                          bool recomputeMips = false)
                : MipLevel(mipLevel), RecomputeMips(recomputeMips) { }
            SetDataParams(const Math::Box2Du& destRange,
                          uint_mipLevel_t mipLevel,
                          bool recomputeMips = false)
                : DestRange(destRange), MipLevel(mipLevel), RecomputeMips(recomputeMips) { }

            Math::Box2Du GetRange(const glm::uvec2& fullSize) const
            {
                if (glm::all(glm::equal(DestRange.Size, glm::uvec2{ 0 })))
                    return Math::Box2Du::MakeMinSize(glm::uvec2{ 0 }, fullSize);
                else
                    return DestRange;
            }
        };


        //Clears part or all of this texture to the given value,
        //    on all 6 faces.
        //Not allowed for compressed-format textures.
        template<glm::length_t L, typename T>
        void Clear(const glm::vec<L, T> value,
                   SetDataParams optionalParams = { },
                   bool bgrOrdering = false)
        {
            BPAssert(!format.IsCompressed(), "Can't clear a compressed texture!");

            auto range = optionalParams.GetRange(size);
            glClearTexSubImage(glPtr.Get(), optionalParams.MipLevel,
                               range.MinCorner.x, range.MinCorner.y, 0,
                               range.Size.x, range.Size.y, 6,
                               GetOglInputFormat<T>(),
                               GetOglChannels(GetComponents<L>(bgrOrdering)),
                               glm::value_ptr(value));
        }

        //Clears part of all of the given face on this texture to the given value.
        //Not allowed for compressed-format textures.
        template<glm::length_t L, typename T>
        void Clear(CubeFaces face, const glm::vec<L, T> value,
                   SetDataParams optionalParams = { },
                   bool bgrOrdering = false)
        {
            BPAssert(!format.IsCompressed(), "Can't clear a compressed texture!");

            auto range = optionalParams.GetRange(size);
            glClearTexSubImage(glPtr.Get(), optionalParams.MipLevel,
                               range.MinCorner.x, range.MinCorner.y, face._to_index(),
                               range.Size.x, range.Size.y, 1,
                               GetOglInputFormat<T>(),
                               GetOglChannels(GetComponents<L>(bgrOrdering)),
                               glm::value_ptr(value));
        }


        //Note that pixel data in OpenGL is ordered from left to right,
        //    then from bottom to top, then from back to front.
        //In other words, rows are contiguous and then grouped vertically.

        //Sets the given face to use the given data.
        //Note that the upload to the GPU will be slower if
        //    the data doesn't exactly match the texture's pixel format.
        //It's also not recommended to use this to upload to a compressed texture format,
        //    because GPU drivers may not have good-quality compression algorithms.
        template<typename T>
        void SetData(CubeFaces face, const T* data, ComponentData components,
                     SetDataParams optionalParams = { })
        {
            SetData((const void*)data, face,
                    GetOglChannels(components), GetOglInputFormat<T>(),
                    optionalParams);
        }

        //Sets the given face to use the given data.
        //The number of components is determined by the size of the vector type --
        //    1D is Red/Depth, 2D is Red-Green, 3D is RGB, 4D is RGBA.
        //If "bgrOrdering" is true, then the incoming RGB(A) data is actually in BGR(A) order.
        //BGR order is often more efficient to give to the GPU.
        template<glm::length_t L, typename T>
        void SetData(CubeFaces face, const glm::vec<L, T>* data,
                     bool bgrOrdering = false,
                     SetDataParams optionalParams = { })
        {
            SetData(glm::value_ptr(*data), face,
                    GetOglChannels(GetComponents<L>(bgrOrdering)),
                    GetOglInputFormat<T>(),
                    optionalParams);
        }

        //Directly sets block-compressed data for the texture,
        //    based on its current format.
        //The input data should be "GetByteCount()/6" bytes large.
        //This is highly recommended over the other "SetData()" forms for compressed textures;
        //    while the GPU driver should support doing the compression under the hood for you,
        //    the results vary widely and are often bad.
        //Note that, because Block-Compression works in square "blocks" of pixels,
        //    the destination rectangle is in units of blocks, not individual pixels.
        //Additionally, mipmaps cannot be regenerated automatically.
        void SetData_Compressed(CubeFaces face, const std::byte* compressedData,
                                uint_mipLevel_t mipLevel = 0,
                                Math::Box2Du destBlockRange = { });

        #pragma endregion
        
        #pragma region Getting data

        //Optional parameters when downloading texture data.
        struct GetDataParams
        {
            //The subset of the texture to set.
            //A size-0 box represents the full texture.
            Math::Box2Du Range = Math::Box2Du::MakeCenterSize(glm::uvec2{ 0 }, glm::uvec2{ 0 });
            //The mip level. 0 is the original texture, higher values are smaller mips.
            uint_mipLevel_t MipLevel = 0;

            GetDataParams() { }
            GetDataParams(const Math::Box2Du& range)
                : Range(range) { }
            GetDataParams(uint_mipLevel_t mipLevel)
                : MipLevel(mipLevel) { }
            GetDataParams(const Math::Box2Du& range,
                          uint_mipLevel_t mipLevel)
                : Range(range), MipLevel(mipLevel) { }
        };

        template<typename T>
        void GetData(CubeFaces face, T* data, ComponentData components,
                     GetDataParams optionalParams = { }) const
        {
            GetData((void*)data, face,
                    GetOglChannels(components), GetOglInputFormat<T>(),
                    optionalParams);
        }

        //Gets the given face's pixels and writes them into the given buffer.
        //1D data is interpreted as R channel (or depth values),
        //    2D as RG, 3D as RGB, and 4D as RGBA.
        //If "bgrOrdering" is true, then the incoming RGB(A) data
        //    is actually written out in BGR(A) order, which is faster on most platforms
        //    because the GPU natively stores it that way.
        template<glm::length_t L, typename T>
        void GetData(CubeFaces face,
                     glm::vec<L, T>* data, bool bgrOrdering = false,
                     GetDataParams optionalParams = { }) const
        {
            GetData<T>(glm::value_ptr(*data), face,
                       GetComponents<L>(bgrOrdering),
                       optionalParams);
        }

        //Directly reads block-compressed data from the texture,
        //    based on its current format.
        //This is a fast, direct copy of the byte data stored in the texture.
        //Note that, because Block-Compression works in square groups of pixels,
        //    the "range" rectangle is in units of blocks, not individual pixels.
        void GetData_Compressed(CubeFaces face, std::byte* compressedData,
                                Math::Box2Du blockRange = { },
                                uint_mipLevel_t mipLevel = 0) const;

        #pragma endregion

    protected:

        TextureMinFilters minFilter;
        TextureMagFilters magFilter;

        glm::uvec2 size;

        
        void SetData(const void* data, CubeFaces face,
                     GLenum dataFormat, GLenum dataType,
                     const SetDataParams& params);
        void GetData(void* data, CubeFaces face,
                     GLenum dataFormat, GLenum dataType,
                     const GetDataParams& params) const;
    };
}