#pragma once

#include <array>

#include "Format.h"


namespace Bplus::GL::Textures
{
    //The different kinds of textures in OpenGL.
    BETTER_ENUM(TextureTypes, GLenum,
        OneD = GL_TEXTURE_1D,
        TwoD = GL_TEXTURE_2D,
        ThreeD = GL_TEXTURE_3D,
        Cubemap = GL_TEXTURE_CUBE_MAP,

        //A special kind of 2D texture that supports MSAA.
        TwoD_Multisample = GL_TEXTURE_2D_MULTISAMPLE
    );


    //The behaviors of a texture when you sample past its boundaries.
    BETTER_ENUM(TextureWrapping, GLenum,
        //Repeat the texture indefinitely, creating a tiling effect.
        Repeat = GL_REPEAT,
        //Repeat the texture indefinitely, but mirror it across each edge.
        MirroredRepeat = GL_MIRRORED_REPEAT,
        //Clamp the coordinates so that the texture outputs its last edge pixels
        //    when going past its border.
        Clamp = GL_CLAMP_TO_EDGE,
        //Outputs a custom color when outside the texture.
        CustomBorder = GL_CLAMP_TO_BORDER
    );

    //The filtering mode for a texture when shrinking it on the screen.
    BETTER_ENUM(TextureMinFilters, GLenum,
        //Rough (or "nearest") sampling for both the pixels and the mipmaps.
        Rough = GL_NEAREST_MIPMAP_NEAREST,
        //Smooth (or "linear") sampling for both the pixels and the mipmaps.
        Smooth = GL_NEAREST_MIPMAP_NEAREST,

        //Smooth sampling for the pixels, and rough sampling for the mipmaps.
        SmoothPixels_RoughMips = GL_LINEAR_MIPMAP_NEAREST,
        //Rough sampling for the pixels, and smooth sampling for the mipmaps.
        RoughPixels_SmoothMips = GL_NEAREST_MIPMAP_LINEAR,

        //Rough sampling for the pixels, and no mip-maps (always use the largest mip).
        RoughPixels_NoMips = GL_NEAREST,
        //Smooth sammpling for the pixels, and no mip-maps (always use the largest mip).
        SmoothPixels_NoMips = GL_LINEAR
    );
    //The filtering mode for a texture when enlarging it on the screen.
    BETTER_ENUM(TextureMagFilters, GLenum,
        //Rough, or "nearest" sampling.
        Rough = GL_NEAREST,
        //Smooth, or "linear" sampling.
        Smooth = GL_LINEAR
    );


    //Information about a sampler for a texture of some number of dimensions.
    template<uint8_t D>
    struct Sampler
    {
        static_assert(D > 0);

        TextureMinFilters MinFilter;
        TextureMagFilters MagFilter;
        std::array<TextureWrapping, D> Wrapping;


        Sampler(TextureWrapping wrapping = TextureWrapping::Clamp)
            : Sampler(TextureMinFilters::Smooth, TextureMagFilters::Smooth, wrapping) { }
        Sampler(TextureMinFilters min, TextureMagFilters mag, TextureWrapping wrapping)
            : MinFilter(min), MagFilter(mag)
        {
            SetWrapping(wrapping);
        }


        void SetWrapping(TextureWrapping w)
        {
            for (uint_fast8_t d = 0; d < D; ++d)
                Wrapping[d] = w;
        }

        //Get this sampler's wrapping mode,
        //    assuming all axes use the same wrapping.
        TextureWrapping GetWrapping() const
        {
            BPAssert(std::all_of(Wrapping.begin(), Wrapping.end(),
                                 [&](TextureWrapping w) { return w == Wrapping[0]; }),
                     "Sampler's axes have different wrap modes");

            return Wrapping[0];
        }
    };


    //An OpenGL object representing a 2D grid of pixels that can be "sampled" in shaders.
    //May also be an array of textures.
    class Texture2D
    {
    public:

        //TODO: Finish: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexParameter.xhtml

        Texture2D(bool isArray);

        virtual ~Texture2D();


        //No copying.
        Texture2D(const Texture2D& cpy) = delete;
        Texture2D& operator=(const Texture2D& cpy) = delete;

        //Move operators.
        Texture2D(Texture2D&& src);
        Texture2D& operator=(Texture2D&& src);


        TextureTypes GetType() const { return type; }
        bool IsArray() const { return isArray; }

        Sampler<2> GetSampler() const { return sampler; }
        void SetSampler(const Sampler<2>& s);

        //void SetData(const glm::uvec2& size, )


    private:

        OglPtr::Texture glPtr;

        TextureTypes type;
        bool isArray;
        glm::uvec2 size = { 0, 0 };
        Format format;

        Sampler<2> sampler;
    };


    //TODO: 'Handle' class.
}