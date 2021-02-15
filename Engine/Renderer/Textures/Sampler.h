#pragma once

#include "../../Math/Box.hpp"
#include "../Data.h"

namespace Bplus::GL::Textures
{
    //The behaviors of a texture when you sample past its boundaries.
    BETTER_ENUM(WrapModes, GLenum,
        //Repeat the texture indefinitely, creating a tiling effect.
        Repeat = GL_REPEAT,
        //Repeat the texture indefinitely, but mirror it across each edge.
        MirroredRepeat = GL_MIRRORED_REPEAT,
        //Clamp the coordinates so that the texture outputs its last edge pixels
        //    when going past its border.
        Clamp = GL_CLAMP_TO_EDGE,
        //Outputs a custom color when outside the texture.
        //TODO: It seems I forgot to actually use border color sampling. Add a field to Sampler for the border color, and NOTE that bindless textures have a very limited set of allowed border colors.
        CustomBorder = GL_CLAMP_TO_BORDER
    );

    //The filtering mode for a texture's pixels,
    //    within a single mip level.
    BETTER_ENUM(PixelFilters, GLenum,
        //Individual pixels are visible.
        //Often referred to as "nearest" sampling.
        Rough = GL_NEAREST,
        //Blends the nearest 4 pixels together.
        //Often referred to as "linear" sampling.
        Smooth = GL_LINEAR
    );

    //How to blend a texture's mip levels together.
    BETTER_ENUM(MipFilters, GLenum,
        //Mipmaps are not used (i.e. only the lowest mip level is sampled).
        Off = 0,
        //A single mip level is picked and sampled from.
        Rough = (GLenum)PixelFilters::Rough,
        //Blends the two closest mip levels together.
        Smooth = (GLenum)PixelFilters::Smooth
    );

    inline GLenum ToMagFilter(PixelFilters pixelFilter) { return (GLenum)pixelFilter; }
    inline GLenum ToMinFilter(PixelFilters pixelFilter, MipFilters mipFilter)
    {
        switch (pixelFilter)
        {
            case PixelFilters::Rough: switch (mipFilter) {
                case MipFilters::Off: return GL_NEAREST;
                case MipFilters::Rough: return GL_NEAREST_MIPMAP_NEAREST;
                case MipFilters::Smooth: return GL_NEAREST_MIPMAP_LINEAR;
                default: BPAssert(false, "Unknown mip filter mode"); return GL_NONE;
            }

            case PixelFilters::Smooth: switch (mipFilter) {
                case MipFilters::Off: return GL_LINEAR;
                case MipFilters::Rough: return GL_LINEAR_MIPMAP_NEAREST;
                case MipFilters::Smooth: return GL_LINEAR_MIPMAP_LINEAR;
                default: BPAssert(false, "Unknown mip filter mode"); return GL_NONE;
            }

            default: BPAssert(false, "Unknown pixel filter mode"); return GL_NONE;
        }
    }
    

    //The different sources a color texture can pull from during sampling.
    //Note that swizzling is set per-texture, not per-sampler.
    BETTER_ENUM(SwizzleSources, GLenum,
        //The texture's Red component.
        Red = GL_RED,
        //The texture's Green component.
        Green = GL_GREEN,
        //The texture's Blue component.
        Blue = GL_BLUE,
        //The texture's Alpha component.
        Alpha = GL_ALPHA,
        //A constant value of 0.
        Zero = GL_ZERO,
        //A constant value of 1.
        One = GL_ONE
    );
    using SwizzleRGBA = std::array<SwizzleSources, 4>;


    //The different ways a depth/stencil hybrid texture can be sampled.
    //Note that this setting is per-texture, not per-sampler.
    BETTER_ENUM(DepthStencilSources, GLenum,
        //The texture will sample its depth and output floats (generally between 0-1).
        Depth = GL_DEPTH_COMPONENT,
        //The texture will sample its stencil and output unsigned integers.
        Stencil = GL_STENCIL_INDEX
    );


    //Information about a sampler for a D-dimensional texture.
    template<uint8_t D>
    struct Sampler
    {
        static_assert(D > 0);

        std::array<WrapModes, D> Wrapping;
        PixelFilters PixelFilter;
        MipFilters MipFilter;

        //Offsets the mip level calculation for mipmapped textures.
        //For example, a value of 1 essentially forces all samples to go up one mip level.
        float MipOffset = 0.0f;
        //Sets the boundaries of the mip level calculation for mipmapped textures.
        //According to the OpenGL standard, it defaults to {-1000, 1000}.
        Math::IntervalF MipClampRange = Math::IntervalF::MakeMinMax(glm::fvec1{-1000}, glm::fvec1{1000});

        //If this is a depth (or depth-stencil) texture,
        //    this setting makes it a "shadow" sampler,
        std::optional<ValueTests> DepthComparisonMode = std::nullopt;


        Sampler(WrapModes wrapping = WrapModes::Clamp,
                PixelFilters filter = PixelFilters::Smooth,
                std::optional<ValueTests> depthComparisonMode = std::nullopt,
                float mipOffset = 0.0f,
                Math::IntervalF mipClampRange = Math::IntervalF::MakeMinMax(glm::fvec1{-1000}, glm::fvec1{1000}))
            : Sampler(MakeArray<WrapModes, D>(wrapping),
                      filter,
                      MipFilters::_from_integral(filter._to_integral()),
                      depthComparisonMode,
                      mipOffset, mipClampRange) { }

        Sampler(const std::array<WrapModes, D>& wrappingPerAxis,
                PixelFilters pixelFilter, MipFilters mipFilter,
                std::optional<ValueTests> depthComparisonMode = std::nullopt,
                float mipOffset = 0.0f,
                Math::IntervalF mipClampRange = Math::IntervalF::MakeMinMax(glm::fvec1{-1000}, glm::fvec1{1000}))
            : Wrapping(wrappingPerAxis), PixelFilter(pixelFilter), MipFilter(mipFilter),
              MipOffset(mipOffset), MipClampRange(mipClampRange)
        {

        }


        //Sets the wrapping mode for all axes.
        void SetWrapping(WrapModes w)
        {
            for (uint_fast8_t d = 0; d < D; ++d)
                Wrapping[d] = w;
        }
        //Get the wrapping mode for all axes, assuming they're all the same.
        WrapModes GetWrapping() const
        {
            BPAssert(std::all_of(Wrapping.begin(), Wrapping.end(),
                                 [&](WrapModes w) { return w == Wrapping[0]; }),
                     "Sampler's axes have different wrap modes");

            return Wrapping[0];
        }

        void Apply(OglPtr::Texture tex)  const { Apply( tex.Get(), glTextureParameteri, glTextureParameterf); }
        void Apply(OglPtr::Sampler samp) const { Apply(samp.Get(), glSamplerParameteri, glSamplerParameterf); }


        //A helper function to convert the per-axis part of this sampler's data.
        //Some code doesn't want to be templated, so they store all sampler data
        //    in the least-common-denominator form (i.e. 3D).
        template<glm::length_t D2>
        Sampler<D2> ChangeDimensions() const
        {
            //Make a copy.
            static_assert(D > 0,
                          "No support for Sampler<D>::ChangeDimensions<>() with D == 0");
            Sampler<D2> result(MakeArray<WrapModes, D2>(Wrapping.back()),
                               PixelFilter, MipFilter, DepthComparisonMode,
                               MipOffset, MipClampRange);

            //Copy over the Wrapping array's elements.
            size_t maxI = std::min(Wrapping.size(), result.Wrapping.size());
            for (size_t i = 0; i < maxI; ++i)
                result.Wrapping[i] = Wrapping[i];

            return result;
        }


        bool operator==(const Sampler<D>& other) const
        {
            return (Wrapping == other.Wrapping) &
                   (PixelFilter == other.PixelFilter) &
                   (DepthComparisonMode == other.DepthComparisonMode) &
                   (MipFilter == other.MipFilter) &
                   (MipOffset == other.MipOffset) &
                   (MipClampRange == other.MipClampRange);
        }
        bool operator!=(const Sampler<D>& other) const { return !operator==(other); }


    private:

        //Applies this sampler's settings to the given OpenGL object (a texture or sampler),
        //    using the given OpenGL functions.
        void Apply(GLuint targetPtr,
                   void(*glSetFuncI)(GLuint, GLenum, GLint),
                   void(*glSetFuncF)(GLuint, GLenum, GLfloat)) const
        {
            //Set filtering.
            glSetFuncI(targetPtr, GL_TEXTURE_MIN_FILTER,
                       (GLint)ToMinFilter(PixelFilter, MipFilter));
            glSetFuncI(targetPtr, GL_TEXTURE_MAG_FILTER,
                       (GLint)ToMagFilter(PixelFilter));

            //Set mip bias.
            glSetFuncF(targetPtr, GL_TEXTURE_MIN_LOD, MipClampRange.MinCorner.x);
            glSetFuncF(targetPtr, GL_TEXTURE_MAX_LOD, MipClampRange.GetMaxCorner().x);
            glSetFuncF(targetPtr, GL_TEXTURE_LOD_BIAS, MipOffset);

            if (DepthComparisonMode.has_value())
            {
                glSetFuncI(targetPtr, GL_TEXTURE_COMPARE_MODE, (GLint)GL_COMPARE_REF_TO_TEXTURE);
                glSetFuncI(targetPtr, GL_TEXTURE_COMPARE_FUNC, (GLint)*DepthComparisonMode);
            }
            else
            {
                glSetFuncI(targetPtr, GL_TEXTURE_COMPARE_MODE, (GLint)GL_NONE);
            }

            //Note that OpenGL is not bothered by setting this value for dimensions
            //    higher than the texture actually has.
            const std::array<GLenum, 3> wrapEnums = {
                GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R
            };
            for (size_t i = 0; i < D; ++i)
                glSetFuncI(targetPtr, wrapEnums[i], (GLint)Wrapping[i]._to_integral());
        }
    };
}


//Allow enums and structs in this file to be hashed, for use in STL collections.
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Textures::WrapModes);
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Textures::PixelFilters);
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Textures::MipFilters);
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Textures::SwizzleSources);
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Textures::DepthStencilSources);

BP_HASHABLE_SIMPLE_FULL(size_t D, Bplus::GL::Textures::Sampler<D>,
                        d.Wrapping, d.PixelFilter, d.MipFilter,
                        d.MipOffset, d.MipClampRange,
                        d.DepthComparisonMode)