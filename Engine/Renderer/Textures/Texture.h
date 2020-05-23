#pragma once

#include <numeric>

#include "Sampler.h"
#include "../../Math/Box.hpp"


namespace Bplus::GL::Textures
{
    //The different kinds of textures in OpenGL.
    BETTER_ENUM(Types, GLenum,
        OneD = GL_TEXTURE_1D,
        TwoD = GL_TEXTURE_2D,
        ThreeD = GL_TEXTURE_3D,
        Cubemap = GL_TEXTURE_CUBE_MAP,

        //A special kind of 2D texture that supports MSAA.
        TwoD_Multisample = GL_TEXTURE_2D_MULTISAMPLE
    );

    //Subsets of color channels when uploading/downloading pixel data,
    //    in byte order.
    BETTER_ENUM(ComponentData, GLenum,
        Red = GL_RED,
        Green = GL_GREEN,
        Blue = GL_BLUE,
        RG = GL_RG,
        RGB = GL_RGB,
        BGR = GL_BGR,
        RGBA = GL_RGBA,
        BGRA = GL_BGRA
    );


    //The unsigned integer type used to represent mip levels.
    using uint_mipLevel_t = uint_fast16_t;

    //Gets the maximum number of mipmaps for a texture of the given size.
    template<glm::length_t L>
    uint_mipLevel_t GetMaxNumbMipmaps(const glm::vec<L, glm::u32>& texSize)
    {
        auto largestAxis = glm::compMax(texSize);
        return 1 + (uint_mipLevel_t)floor(log2(largestAxis));
    }

    //TODO: Texture2DMSAA class.
    //TODO: Copying from one texture to another (and from framebuffer into texture? it's redundant though).
    //TODO: Memory Barriers. https://www.khronos.org/opengl/wiki/Memory_Model#Texture_barrier

    //An extremely basic wrapper around OpenGL textures.
    //More full-featured wrappers are defined below and inherit from this one.
    class BP_API Texture
    {
    public:
        //Creates a new texture.
        //Pass "1" for nMipLevels to not use mip-maps.
        //Pass "0" for nMipLevels to generate full mip-maps down to a single pixel.
        //Pass anything else to generate a fixed amount of mip levels.
        Texture(Types type, Format format, uint_mipLevel_t nMipLevels);
        virtual ~Texture();
        
        //No copying.
        Texture(const Texture& cpy) = delete;
        Texture& operator=(const Texture& cpy) = delete;


        const Format& GetFormat() const { return format; }

        Types GetType() const { return type; }
        uint_mipLevel_t GetNMipLevels() const { return nMipLevels; }
        OglPtr::Texture GetOglPtr() const { return glPtr; }

        //Updates mipmaps for this texture.
        //Not allowed for compressed-format textures.
        void RecomputeMips();

    protected:

        OglPtr::Texture glPtr;
        Types type;
        uint_mipLevel_t nMipLevels;

        Format format;

        
        //Given a set of components for texture uploading/downloading,
        //    and the data type of this texture's pixels,
        //    finds the corresponding OpenGL enum value.
        GLenum GetOglChannels(ComponentData components) const
        {
            //If the pixel format isn't integer (i.e. it's float or normalized integer),
            //    we can directly use the enum values.
            //Otherwise, we should be sending the "integer" enum values.
            if (!format.IsInteger())
                return (GLenum)components;
            else switch (components)
            {
                case ComponentData::Red: return GL_RED_INTEGER;
                case ComponentData::Green: return GL_GREEN_INTEGER;
                case ComponentData::Blue: return GL_BLUE_INTEGER;
                case ComponentData::RG: return GL_RG_INTEGER;
                case ComponentData::RGB: return GL_RGB_INTEGER;
                case ComponentData::BGR: return GL_BGR_INTEGER;
                case ComponentData::RGBA: return GL_RGBA_INTEGER;
                case ComponentData::BGRA: return GL_BGRA_INTEGER;

                default:
                    std::string msg = "Unexpected data component type: ";
                    msg += components._to_string();
                    BPAssert(false, msg.c_str());
                    return GL_NONE;
            }
        }

        //Given a type T, finds the corresponding GLenum for that type of data.
        //    bool types are interpreted as unsigned integers of the same size.
        template<typename T>
        GLenum GetOglInputFormat() const
        {
            //Deduce the "type" argument for OpenGL, representing
            //    the size of each channel being sent in.
            GLenum type = GL_NONE;
            if constexpr (std::is_same_v<T, bool>)
            {
                if constexpr (sizeof(bool) == 1) {
                    type = GL_UNSIGNED_BYTE;
                } else if constexpr (sizeof(bool) == 2) {
                    type = GL_UNSIGNED_SHORT;
                } else if constexpr (sizeof(bool) == 4) {
                    type = GL_UNSIGNED_INT;
                } else {
                    static_assert(false, "Unexpected value for sizeof(bool)");
                }
            }
            else if constexpr (std::is_same_v<T, glm::u8>) {
                type = GL_UNSIGNED_BYTE;
            } else if constexpr (std::is_same_v<T, glm::u16>) {
                type = GL_UNSIGNED_SHORT;
            } else if constexpr (std::is_same_v<T, glm::u32>) {
                type = GL_UNSIGNED_INT;
            } else if constexpr (std::is_same_v<T, glm::i8>) {
                type = GL_BYTE;
            } else if constexpr (std::is_same_v<T, glm::i16>) {
                type = GL_SHORT;
            } else if constexpr (std::is_same_v<T, glm::i32>) {
                type = GL_INT;
            } else if constexpr (std::is_same_v<T, glm::f32>) {
                type = GL_FLOAT;
            } else {
                static_assert(false, "T is an unexpected type");
            }

            return type;
        }

        //Given a number of dimensions, and a switch for reversed (BGR) ordering,
        //    finds the corresponding enum value representing component ordering
        //    for texture upload/download.
        template<glm::length_t L>
        ComponentData GetComponents(bool bgrOrdering) const
        {
            if constexpr (L == 1) {
                return ComponentData::Greyscale;
            } else if constexpr (L == 2) {
                return ComponentData::RG;
            } else if constexpr (L == 3) {
                return (bgrOrdering ?
                            ComponentData::BGR :
                            ComponentData::RGB);
            } else if constexpr (L == 4) {
                return (bgrOrdering ?
                            ComponentData::BGRA :
                            ComponentData::RGBA);
            } else {
                static_assert(false, "L should be between 1 and 4");
                return ComponentData::Greyscale;
            }
        }


        //Child classes have access to the move operator.
        Texture(Texture&& src);
        Texture& operator=(Texture&& src);
    };

    //A great reference for getting/setting texture data in OpenGL:
    //    https://www.khronos.org/opengl/wiki/Pixel_Transfer


    //TODO: 'Handle' class.

}