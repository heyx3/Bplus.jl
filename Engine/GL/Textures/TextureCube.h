#pragma once

#include "Texture.h"

namespace Bplus::GL::Textures
{
    //The six faces of a cube, defined to match the OpenGL cubemap texture faces.
    //Note that they are ordered in the same way that OpenGL orders them in memory.
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

                default: BP_ASSERT(false, fromEnum._to_string()); Dir = 1;
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

                default: BP_ASSERT(false, fromEnum._to_string()); Axis = 1;
            }
        }

        CubeFaces ToFaceEnum() const
        {
            switch (Axis)
            {
                case 0: return Dir ? CubeFaces::PosX : CubeFaces::NegX;
                case 1: return Dir ? CubeFaces::PosY : CubeFaces::NegY;
                case 2: return Dir ? CubeFaces::PosZ : CubeFaces::NegZ;
                default: BP_ASSERT(false, "Unknown axis"); return CubeFaces::NegX;
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

    
    #pragma region Cube face orientation data

    //Defines, for each face of a cube-map texture, how it is oriented in 3D space.
    //The faces are ordered in their GPU memory order.
    struct CubeFaceOrientation
    {
        CubeFaces Face;
        //The 'Min Corner' maps the first pixel of the 2D texture face
        //    to its 3D corner in the cube-map (from -1 to +1).
        //The 'Max Corner' does the same for the last pixel.
        glm::i8vec3 MinCorner, MaxCorner;
        //These values describe the 3D axis for each of the texture face's axes.
        //0=X, 1=Y, 2=Z
        glm::u8 HorzAxis, VertAxis;

        //Converts a UV coordinate on this face to a 3D cubemap vector.
        glm::fvec3 GetDir(glm::fvec2 uv) const
        {
            glm::fvec3 dir3D(MinCorner);
            dir3D[HorzAxis] = glm::mix((float)MinCorner[HorzAxis],
                                       (float)MaxCorner[HorzAxis],
                                       uv.x);
            dir3D[VertAxis] = glm::mix((float)MinCorner[VertAxis],
                                       (float)MaxCorner[VertAxis],
                                       uv.y);
            return dir3D;
        }
    };
    //Gets the memory layout for each face of a cubemap texture.
    inline std::array<CubeFaceOrientation, 6> GetFacesOrientation()
    {
        return std::array{
            CubeFaceOrientation{CubeFaces::PosX, { 1, 1, 1 }, { 1, -1, -1 }, 2, 1 },
            CubeFaceOrientation{CubeFaces::NegX, { -1, 1, -1 }, { -1, -1, 1 }, 2, 1 },
            CubeFaceOrientation{CubeFaces::PosY, { -1, 1, -1 }, { 1, 1, 1 }, 0, 2 },
            CubeFaceOrientation{CubeFaces::NegY, { -1, -1, 1 }, { 1, -1, -1 }, 0, 2 },
            CubeFaceOrientation{CubeFaces::PosZ, { -1, 1, 1 }, { 1, -1, 1 }, 0, 1 },
            CubeFaceOrientation{CubeFaces::NegZ, { 1, 1, -1 }, { -1, -1, -1 }, 0, 1 },
        };
    }

    #pragma endregion

    #pragma region Get- and SetDataCubeParams

    struct SetDataCubeParams : public SetData2DParams
    {
        //No value means all faces will be changed.
        std::optional<CubeFaces> Face = std::nullopt;

        SetDataCubeParams(std::optional<CubeFaces> face = std::nullopt,
                          bool recomputeMips = true)
            : SetData2DParams(recomputeMips), Face(face) { }
        SetDataCubeParams(std::optional<CubeFaces> face,
                          const uBox_t& destRange,
                          bool recomputeMips = true)
            : SetData2DParams(destRange, recomputeMips), Face(face) { }
        SetDataCubeParams(std::optional<CubeFaces> face,
                          uint_mipLevel_t mipLevel,
                          bool recomputeMips = false)
            : SetData2DParams(mipLevel, recomputeMips), Face(face) { }
        SetDataCubeParams(std::optional<CubeFaces> face,
                          const uBox_t& destRange,
                          uint_mipLevel_t mipLevel,
                          bool recomputeMips = false)
            : SetData2DParams(destRange, mipLevel, recomputeMips), Face(face) { }

        //OpenGL often treats cube-maps as 3D textures, where each Z-slice is a separate face.
        //This function adds the Z position/size to a 2D range, based on the Face field.
        Math::Box3Du ToRange3D(const Math::Box2Du& range2D) const
        {
            auto range = range2D.ChangeDimensions<3>();

            //If changing a specific face, set the start Z to that face and leave the Z size at 1.
            //Otherwise, if changing all faces, leave the start Z at 0 and set the Z size to 6.
            if (Face.has_value())
                range.MinCorner.z = (glm::u32)Face.value()._to_index();
            else
                range.Size.z = CubeFaces::_size_constant;

            return range;
        }
    };

    struct GetDataCubeParams : public GetData2DParams
    {
        //No value means all faces will be gotten, in order.
        std::optional<CubeFaces> Face = std::nullopt;
        
        GetDataCubeParams(std::optional<CubeFaces> face = std::nullopt) : Face(face) { }
        GetDataCubeParams(std::optional<CubeFaces> face, const uBox_t& range)
            : GetData2DParams(range), Face(face) { }
        GetDataCubeParams(std::optional<CubeFaces> face, uint_mipLevel_t mipLevel)
            : GetData2DParams(mipLevel), Face(face) { }
        GetDataCubeParams(std::optional<CubeFaces> face,
                          const uBox_t& range, uint_mipLevel_t mipLevel)
            : GetData2DParams(range, mipLevel), Face(face) { }

        //OpenGL often treats cube-maps as 3D textures, where each Z-slice is a separate face.
        //This function adds the Z position/size to a 2D range, based on the Face field.
        Math::Box3Du ToRange3D(const Math::Box2Du& range2D) const
        {
            auto range = range2D.ChangeDimensions<3>();

            //If reading a specific face, set the start Z to that face and leave the Z size at 1.
            //Otherwise, if reading all faces, leave the start Z at 0 and set the Z size to 6.
            if (Face.has_value())
                range.MinCorner.z = (glm::u32)Face.value()._to_index();
            else
                range.Size.z = CubeFaces::_size_constant;

            return range;
        }
    };

    #pragma endregion


    //A "Cubemap" texture, which has 6 2D textures for faces.
    class BP_API TextureCube : public Texture
    {
    public:
        static constexpr Types GetClassType() { return Types::Cubemap; }

        using Sampler_t = Sampler<2>;


        //Creates a new cube-map with the given width/height.
        //Pass "1" for nMipLevels to not use mip-maps.
        //Pass "0" for nMipLevels to generate full mip-maps down to a single pixel.
        //Pass anything else to generate a fixed amount of mip levels.
        TextureCube(uint32_t size, Format format,
                    uint_mipLevel_t nMipLevels = 0,
                    Sampler_t sampler = { },
                    SwizzleRGBA swizzling = DefaultSwizzling(),
                    std::optional<DepthStencilSources> depthStencilMode = std::nullopt);

        //Note that the copy constructor/operator is automatically deleted via the parent class.

        TextureCube(TextureCube&& src);
        TextureCube& operator=(TextureCube&& src);


        uint32_t GetSize(uint_mipLevel_t mipLevel = 0) const;
        glm::uvec2 GetSize2D(uint_mipLevel_t mipLevel = 0) const
        {
            auto s = GetSize(mipLevel);
            return { s, s };
        }

        //Gets the number of bytes needed to store this texture in its native format.
        //This includes all six faces; divide the result by 6 to get the byte-size per face.
        size_t GetByteSize(uint_mipLevel_t mipLevel = 0) const override { return 6 * GetFormat().GetByteSize(GetSize2D(mipLevel)); }

        //Gets (or creates) a view of this texture with the given sampler.
        TexView GetView(std::optional<Sampler<2>> customSampler = std::nullopt) const
        {
            return GetViewFull(customSampler.has_value() ?
                                  std::make_optional(customSampler.value().ChangeDimensions<3>()) :
                                  std::nullopt);
        }
        
        Sampler<2> GetSampler() const { return GetSamplerFull().ChangeDimensions<2>(); }


        #pragma region Clearing data

        //Clears part or all of this color texture to the given value.
        //Not allowed for compressed-format textures.
        template<glm::length_t L, typename T>
        void Clear_Color(const glm::vec<L, T>& value,
                         SetDataCubeParams optionalParams = { },
                         bool bgrOrdering = false)
        {
            BP_ASSERT(!GetFormat().IsCompressed(), "Can't clear a compressed texture!");
            BP_ASSERT(!GetFormat().IsDepthStencil(), "Can't clear a depth/stencil texture with `Clear_Color()`!");
            if constexpr (!std::is_integral_v<T>)
                BP_ASSERT(!GetFormat().IsInteger(), "Can't clear an integer texture to a non-integer value");

            ClearData(glm::value_ptr(value),
                      GetPixelIOChannels<L>(bgrOrdering),
                      GetPixelIOType<T>(),
                      optionalParams);
        }

        //Note that you can't clear compressed textures, though you can set their pixels with Set_Compressed.

        //Clears part or all of this depth texture to the given value.
        template<typename T>
        void Clear_Depth(T depth, SetDataCubeParams optionalParams = { })
        {
            BP_ASSERT(GetFormat().IsDepthOnly(),
                     "Trying to clear depth value in a color, stencil, or depth-stencil texture");

            ClearData(&depth, GL_DEPTH_COMPONENT, GetPixelIOType<T>(),
                      optionalParams);
        }

        //Clears part or all of this stencil texture to the given value.
        void Clear_Stencil(uint8_t stencil, SetDataCubeParams optionalParams = { })
        {
            BP_ASSERT(GetFormat().IsStencilOnly(),
                     "Trying to clear the stencil value in a color, depth, or depth-stencil texture");

            ClearData(&stencil,
                      GL_STENCIL_INDEX, (GLenum)PixelIOTypes::UInt8,
                      optionalParams);
        }

        //Clears part or all of this depth/stencil hybrid texture.
        //Must use the format Depth24U_Stencil8.
        void Clear_DepthStencil(Unpacked_Depth24uStencil8u value,
                                SetDataCubeParams optionalParams = { })
        {
            BP_ASSERT(GetFormat() == +DepthStencilFormats::Depth24U_Stencil8,
                     "Trying to clear depth/stencil texture with 24U depth, but it doesn't have 24U depth");
            
            auto packed = Pack_DepthStencil(value);
            ClearData(&packed,
                      GL_STENCIL_INDEX, GL_UNSIGNED_INT_24_8,
                      optionalParams);
        }
        //Clears part of all of this depth/stencil hybrid texture.
        //Must use the format Depth32F_Stencil8.
        void Clear_DepthStencil(float depth, uint8_t stencil,
                                SetDataCubeParams optionalParams = { })
        {
            BP_ASSERT(GetFormat() == +DepthStencilFormats::Depth32F_Stencil8,
                     "Trying to clear depth/stencil texture with 32F depth, but it doesn't have 32F depth");

            auto packed = Pack_DepthStencil(Unpacked_Depth32fStencil8u{ depth, stencil });
            ClearData(&packed,
                      GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV,
                      optionalParams);
        }

        //The implementation for clearing any kind of data:
    private:
        void ClearData(void* clearValue, GLenum valueFormat, GLenum valueType,
                       const SetDataCubeParams& params);
    public:

        #pragma endregion


        //Note that pixel data in OpenGL is ordered from left to right,
        //    then from bottom to top, then from back to front.
        //In other words, rows are contiguous and then grouped vertically.

        #pragma region Setting data

        //Sets this color texture with the given data.
        //Not allowed for compressed-format textures.
        //If the texture's format is an Integer-type, then the input data must be as well.
        //Note that the upload to the GPU will be slower if
        //    the data doesn't exactly match the texture's pixel format.
        template<typename T>
        void Set_Color(const T* data, PixelIOChannels components,
                       SetDataCubeParams optionalParams = { })
        {
            if constexpr (!std::is_integral_v<T>)
                BP_ASSERT(!GetFormat().IsInteger(), "Can't set an integer texture with non-integer data");

            Set_Color((const std::byte*)data, components, GetPixelIOType<T>(), optionalParams);
        }

        //Sets any kind of color texture with the given data.
        //Not allowed for compressed-format textures.
        //If the texture's format is an Integer-type, then the input data must be as well.
        //The number of components is determined by the size of the vector type --
        //    1D is Red/Depth, 2D is Red-Green, 3D is RGB, 4D is RGBA.
        //If "bgrOrdering" is true, then the incoming RGB(A) data is actually in BGR(A) order.
        //BGR order is often more efficient to give to the GPU.
        template<glm::length_t L, typename T>
        void Set_Color(const glm::vec<L, T>* pixels,
                       bool bgrOrdering = false,
                       SetDataCubeParams optionalParams = { })
        {
            Set_Color<T>(glm::value_ptr(*pixels),
                         GetPixelIOChannels<L>(bgrOrdering),
                         optionalParams);
        }

        //Sets this color texture with the given data.
        //Not allowed for compressed-format textures.
        //If the texture's format is an Integer-type, then the input data must be as well.
        //Note that the upload to the GPU will be slower if
        //    the data doesn't exactly match the texture's pixel format.
        void Set_Color(const std::byte* data,
                       PixelIOChannels components, PixelIOTypes componentType,
                       SetDataCubeParams optionalParams = { })
        {
            //Technically, the OpenGL standard does allow you to set compressed textures
            //    with normal RGBA values.
            //However, the implementation qualities vary widely so we prohibit it.
            BP_ASSERT(!GetFormat().IsCompressed(),
                     "Can't set a compressed texture with Set_Color()! Use Set_Compressed()");

            BP_ASSERT(!GetFormat().IsDepthStencil(),
                     "Can't set a depth/stencil texture with Set_Color()!");

            SetData((const void*)data,
                    GetOglChannels(components), (GLenum)componentType,
                    optionalParams);
        }

        //Directly sets block-compressed data for the texture, on one or all faces,
        //    based on its current format.
        //The input data should have as many bytes as "GetFormat().GetByteSize(pixelsRange)"
        //    times the number of faces affected (either 1 or 6).
        //Note that, because Block-Compression works in square "blocks" of pixels,
        //    the destination rectangle is in units of blocks, not individual pixels.
        //Additionally, mipmaps cannot be regenerated automatically.
        void Set_Compressed(const std::byte* compressedData,
                            std::optional<CubeFaces> face = std::nullopt,
                            Math::Box2Du destBlockRange = { },
                            uint_mipLevel_t mipLevel = 0);

        //Sets part or all of this depth texture to the given value.
        template<typename T>
        void Set_Depth(const T* pixels, SetDataCubeParams optionalParams = { })
        {
            Set_Depth((const std::byte*)pixels, GetPixelIOType<T>(), optionalParams);
        }
        //Sets part or all of this depth texture to the given value.
        void Set_Depth(const std::byte* pixelData, PixelIOTypes dataType,
                       SetDataCubeParams optionalParams = { })
        {
            BP_ASSERT(GetFormat().IsDepthOnly(),
                     "Trying to set depth data for a non-depth texture");

            SetData((const void*)pixelData, GL_DEPTH_COMPONENT, (GLenum)dataType, optionalParams);
        }

        //Sets part or all of this stencil texture to the given value.
        void Set_Stencil(const uint8_t* pixels, SetDataCubeParams optionalParams = { })
        {
            BP_ASSERT(GetFormat().IsStencilOnly(),
                     "Trying to set the stencil values in a color, depth, or depth-stencil texture");

            SetData((const void*)pixels,
                    GL_STENCIL_INDEX, (GLenum)PixelIOTypes::UInt8,
                    optionalParams);
        }

        
        //Sets part or all of this depth/stencil hybrid texture.
        //Must be using the format Depth24U_Stencil8.
        void Set_DepthStencil(const uint32_t* packedPixels_Depth24uStencil8u,
                              SetDataCubeParams optionalParams = { })
        {
            BP_ASSERT(GetFormat() == +DepthStencilFormats::Depth24U_Stencil8,
                     "Trying to set depth/stencil texture with a 24U depth, but it doesn't use 24U depth");
            
            SetData((const void*)packedPixels_Depth24uStencil8u,
                    GL_STENCIL_INDEX, GL_UNSIGNED_INT_24_8,
                    optionalParams);
        }
        //Sets part of all of this depth/stencil hybrid texture.
        //Must be using the format Depth32F_Stencil8.
        void Set_DepthStencil(const uint64_t* packedPixels_Depth32fStencil8u,
                              SetDataCubeParams optionalParams = { })
        {
            BP_ASSERT(GetFormat() == +DepthStencilFormats::Depth32F_Stencil8,
                     "Trying to set depth/stencil texture with a 32F depth, but it doesn't use 32F depth");

            SetData((const void*)packedPixels_Depth32fStencil8u,
                    GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV,
                    optionalParams);
        }


        //The implementation for setting any kind of data:
    private:
        void SetData(const void* data,
                     GLenum dataChannels, GLenum dataType,
                     const SetDataCubeParams& params);
    public:

        #pragma endregion
        
        #pragma region Getting data

        //Gets any kind of color texture data, writing it into the given buffer.
        //Not allowed for compressed-format textures.
        //If the texture's format is an Integer-type, then the data buffer must be as well.
        //Note that the download from the GPU will be slower if
        //    the data type doesn't exactly match the texture's pixel format.
        template<typename T>
        void Get_Color(T* data, PixelIOChannels components,
                       GetDataCubeParams optionalParams = { }) const
        {
            if constexpr (!std::is_integral_v<T>)
                BP_ASSERT(!GetFormat().IsInteger(), "Can't read an integer texture as non-integer data");

            Get_Color((std::byte*)data, sizeof(decltype(data[0])),
                      components, GetPixelIOType<T>(),
                      optionalParams);
        }

        //Gets any kind of color texture data, writing it into the given buffer.
        //Not allowed for compressed-format textures.
        //If the texture's format is an Integer-type, then the input data must be as well.
        //The number of components is determined by the size of the vector type --
        //    1D is Red/Depth, 2D is Red-Green, 3D is RGB, 4D is RGBA.
        //If "bgrOrdering" is true, then the incoming RGB(A) data is actually in BGR(A) order.
        //BGR order is often more efficient to give to the GPU.
        template<glm::length_t L, typename T>
        void Get_Color(glm::vec<L, T>* pixels, bool bgrOrdering = false,
                       GetDataCubeParams optionalParams = { }) const
        {
            Get_Color<T>(glm::value_ptr(*pixels),
                         GetComponents<L>(bgrOrdering),
                         optionalParams);
        }

        //Gets any kind of color texture data, writing it into the given buffer.
        void Get_Color(std::byte* data, size_t pixelByteSize,
                       PixelIOChannels components, PixelIOTypes componentType,
                       GetDataCubeParams optionalParams = { }) const
        {
            BP_ASSERT(!GetFormat().IsDepthStencil(),
                     "Can't read a depth/stencil texture with Get_Color()!");

            GetData((void*)data,
                    pixelByteSize * GetNChannels(components),
                    GetOglChannels(components), componentType,
                    optionalParams);
        }

        //Directly reads block-compressed data from the texture, on one or all faces,
        //    based on its current format.
        //This is a fast, direct copy of the byte data stored in the texture.
        //The input data should have as many bytes as "GetFormat().GetByteSize(pixelsRange)"
        //Note that, because Block-Compression works in square groups of pixels,
        //    the "range" rectangle is in units of blocks, not individual pixels.
        void Get_Compressed(std::byte* compressedData,
                            std::optional<CubeFaces> face = std::nullopt,
                            Math::Box2Du blockRange = { },
                            uint_mipLevel_t mipLevel = 0) const;
        
        //Gets part or all of this depth texture, writing it into the given buffer.
        template<typename T>
        void Get_Depth(T* pixels, GetDataCubeParams optionalParams = { })
        {
            Get_Depth((std::byte*)pixels,
                      GetPixelIOType<T>(), sizeof(decltype(pixels[0])),
                      optionalParams);
        }
        //Gets part or all of this depth texture, writing it into the given buffer.
        void Get_Depth(std::byte* pixelData,
                       PixelIOTypes dataType, size_t pixelByteSize,
                       GetDataCubeParams optionalParams = { })
        {
            BP_ASSERT(GetFormat().IsDepthOnly(),
                     "Trying to get depth data for a non-depth texture");

            GetData((void*)pixelData, pixelByteSize,
                    GL_DEPTH_COMPONENT, (GLenum)dataType,
                    optionalParams);
        }

        //Gets part or all of this stencil texture to the given value.
        void Get_Stencil(uint8_t* pixels, GetDataCubeParams optionalParams = { })
        {
            BP_ASSERT(GetFormat().IsStencilOnly(),
                     "Trying to get the stencil values in a color, depth, or depth-stencil texture");

            GetData((void*)pixels, sizeof(decltype(pixels[0])),
                    GL_STENCIL_INDEX, (GLenum)PixelIOTypes::UInt8,
                    optionalParams);
        }

        
        //Gets part or all of this depth/stencil hybrid texture, writing it into the given buffer.
        //Must be using the format Depth24U_Stencil8.
        void Get_DepthStencil(uint32_t* packedPixels_Depth24uStencil8u,
                              GetDataCubeParams optionalParams = { })
        {
            BP_ASSERT(GetFormat() == +DepthStencilFormats::Depth24U_Stencil8,
                     "Trying to set depth/stencil texture with a 24U depth, but it doesn't use 24U depth");
            
            GetData((void*)packedPixels_Depth24uStencil8u,
                    sizeof(decltype(packedPixels_Depth24uStencil8u[0])),
                    GL_STENCIL_INDEX, GL_UNSIGNED_INT_24_8,
                    optionalParams);
        }
        //Sets part of all of this depth/stencil hybrid texture, writing it into the given buffer.
        //Must be using the format Depth32F_Stencil8.
        void Get_DepthStencil(uint64_t* packedPixels_Depth32fStencil8u,
                              GetDataCubeParams optionalParams = { })
        {
            BP_ASSERT(GetFormat() == +DepthStencilFormats::Depth32F_Stencil8,
                     "Trying to get depth/stencil texture with a 32F depth, but it doesn't use 32F depth");

            GetData((void*)packedPixels_Depth32fStencil8u,
                    sizeof(decltype(packedPixels_Depth32fStencil8u[0])),
                    GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV,
                    optionalParams);
        }


        //The implementation for getting any kind of data:
    private:
        void GetData(void* data, size_t pixelByteSize,
                     GLenum dataChannels, GLenum dataType,
                     const GetDataCubeParams& params) const;
    public:

        #pragma endregion

    protected:

        uint32_t size;
    };
}