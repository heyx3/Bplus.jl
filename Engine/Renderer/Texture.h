#pragma once

#include <array>

#include "Data.h"

namespace Bplus::GL
{
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


    //An OpenGL object representing a grid of pixels that can be "sampled" in shaders.
    class BP_API Texture
    {
    public:

        //TODO: Finish: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexParameter.xhtml

        #pragma region Metadata getters

        //Gets the dimensionality of this texture.
        //Returns 0 if it hasn't been set yet.
        uint8_t GetDimensions() const { return dimensionality; }

        //Gets the default sampler associated with this texture.
        template<uint8_t D>
        const Sampler<D>& GetCurrentSampler() const
        {
            BPAssert(dimensionality == 0 || dimensionality == D,
                     "Dimensionality mismatch");

            Sampler<D> s;
            s.MinFilter = samplerFull.MinFilter;
            s.MagFilter = samplerFull.MagFilter;
            std::copy(samplerFull.Wrapping.begin(),
                      samplerFull.Wrapping.end(),
                      s.Wrapping.begin());

            return s;
        }

        #pragma endregion

        #pragma region Metadata setters

        //Sets the default sampler associated with this texture.
        template<uint8_t D>
        void SetCurrentSampler(const Sampler<D> s)
        {
            BPAssert(dimensionality == 0 || dimensionality == D,
                     "Dimensionality mismatch");
            static_assert(D < 4, "No 4D+ textures");

            //TODO: Push the rest into a non-template function.

            //Set the fields representing the sampler.
            samplerFull.MinFilter = s.MinFilter;
            samplerFull.MagFilter = s.MagFilter;
            std::copy(s.Wrapping.begin(), s.Wrapping.end(),
                      samplerFull.Wrapping.begin());
            
            //Set the OpenGL sampler data.
            glTextureParameteri(glPtr.Get(), GL_TEXTURE_MIN_FILTER,
                                (GLint)samplerFull.MinFilter);
            glTextureParameteri(glPtr.Get(), GL_TEXTURE_MAG_FILTER,
                                (GLint)samplerFull.MagFilter);
            glTextureParameteri(glPtr.Get(), GL_TEXTURE_WRAP_S,
                                (GLint)samplerFull.Wrapping[0]);
            if (D > 1)
                glTextureParameteri(glPtr.Get(), GL_TEXTURE_WRAP_T,
                                    (GLint)samplerFull.Wrapping[1]);
            if (D > 2)
                glTextureParameteri(glPtr.Get(), GL_TEXTURE_WRAP_R,
                                    (GLint)samplerFull.Wrapping[2]);
        }

        #pragma endregion


    private:

        OglPtr::Texture glPtr;

        uint8_t dimensionality;

        //Stored as a full 3D sampler, and casted down to the correct size as needed.
        Sampler<3> samplerFull;
    };


    //TODO: 'Handle' class.
}