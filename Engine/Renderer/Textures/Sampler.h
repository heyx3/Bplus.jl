#pragma once

#include "SamplerDataSource.h"


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
        Math::IntervalF MipClampRange = Math::IntervalF::MakeMinSize(glm::vec1{-1000}, //This is the default value specified by OpenGL.
                                                                     glm::vec1{2000});

        SamplerDataSource DataSource = { };
        SwizzleRGBA DataSwizzle;


        Sampler(WrapModes wrapping = WrapModes::Clamp,
                PixelFilters filter = PixelFilters::Smooth,
                SamplerDataSource dataSource = { },
                SwizzleRGBA dataSwizzle = { SwizzleSources::Red, SwizzleSources::Green,
                                            SwizzleSources::Blue, SwizzleSources::Alpha },
                float mipOffset = 0.0f,
                Math::IntervalF mipClampRange = Math::IntervalF::MakeMinSize(
                    glm::vec1{-1000}, glm::vec1{2000}))
            : Sampler(MakeArray<WrapModes, D>(wrapping),
                      filter,
                      MipFilters::_from_integral(filter._to_integral()),
                      dataSource, dataSwizzle,
                      mipOffset, mipClampRange) { }

        Sampler(const std::array<WrapModes, D>& wrappingPerAxis,
                PixelFilters pixelFilter, MipFilters mipFilter,
                SamplerDataSource dataSource = { },
                SwizzleRGBA dataSwizzle = { SwizzleSources::Red, SwizzleSources::Green,
                                            SwizzleSources::Blue, SwizzleSources::Alpha },
                float mipOffset = 0.0f,
                Math::IntervalF mipClampRange = Math::IntervalF::MakeMinSize(
                    glm::vec1{-1000}, glm::vec1{2000}))
            : Wrapping(wrappingPerAxis), PixelFilter(pixelFilter), MipFilter(mipFilter),
              MipOffset(mipOffset), MipClampRange(mipClampRange),
              DataSource(dataSource), DataSwizzle(dataSwizzle)
        {
            BPAssert(DataSource != +DepthStencilSources::Stencil ||
                         (pixelFilter == +PixelFilters::Rough &&
                          mipFilter == +MipFilters::Rough),
                     "Can't use 'Smooth' filtering on a stencil-sampled texture");
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

        void Apply(OglPtr::Texture tex) const { Apply(tex.Get(), glTextureParameteri, glTextureParameterf); }
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
                               PixelFilter, MipFilter, DataSource, DataSwizzle,
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
                   (MipFilter == other.MipFilter) &
                   (MipOffset == other.MipOffset) &
                   (MipClampRange == other.MipClampRange) &
                   (DataSource == other.DataSource) &
                   (DataSwizzle == other.DataSwizzle);
        }
        bool operator!=(const Sampler<D>& other) const { return !operator==(other); }


    private:

        //Applies this sampler's settings to the given OpenGL object,
        //    using the given OpenGL function.
        void Apply(GLuint targetPtr,
                   void(*glSetFuncI)(GLuint, GLenum, GLint),
                   void(*glSetFuncF)(GLuint, GLenum, GLfloat)) const
        {
            //Set filtering.
            glSetFuncI(targetPtr, GL_TEXTURE_MIN_FILTER,
                       ToMinFilter(PixelFilter, MipFilter));
            glSetFuncI(targetPtr, GL_TEXTURE_MAG_FILTER,
                       ToMagFilter(PixelFilter));

            //Set mip bias.
            glSetFuncF(targetPtr, GL_TEXTURE_MIN_LOD, MipClampRange.MinCorner.x);
            glSetFuncF(targetPtr, GL_TEXTURE_MAX_LOD, MipClampRange.GetMaxCorner().x);
            glSetFuncF(targetPtr, GL_TEXTURE_LOD_BIAS, MipOffset);

            //Set swizzling.
            glSetFuncI(targetPtr, GL_TEXTURE_SWIZZLE_R, (GLint)DataSwizzle[0]);
            glSetFuncI(targetPtr, GL_TEXTURE_SWIZZLE_G, (GLint)DataSwizzle[1]);
            glSetFuncI(targetPtr, GL_TEXTURE_SWIZZLE_B, (GLint)DataSwizzle[2]);
            glSetFuncI(targetPtr, GL_TEXTURE_SWIZZLE_A, (GLint)DataSwizzle[3]);

            //Set the depth/stencil data source.
            if (DataSource.IsDepthComparison())
            {
                glSetFuncI(targetPtr, GL_DEPTH_STENCIL_TEXTURE_MODE,
                           (GLint)DepthStencilSources::Depth);
                glSetFuncI(targetPtr, GL_TEXTURE_COMPARE_MODE,
                           GL_COMPARE_REF_TO_TEXTURE);

                glSetFuncI(targetPtr, GL_TEXTURE_COMPARE_FUNC,
                           (GLint)DataSource.AsDepthComparison);
            }
            else
            {
                //No depth comparison.
                glSetFuncI(targetPtr, GL_TEXTURE_COMPARE_MODE, GL_NONE);

                //If setting depth vs stencil explicitly, do that.
                if (DataSource.IsDepthOrStencil())
                {
                    glSetFuncI(targetPtr, GL_DEPTH_STENCIL_TEXTURE_MODE,
                               (GLint)DataSource.AsDepthOrStencil());
                }
                //Otherwise use the default, Depth.
                else
                {
                    glSetFuncI(targetPtr, GL_DEPTH_STENCIL_TEXTURE_MODE,
                               (GLint)DepthStencilSources::Depth);
                }
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
namespace std
{
    //Sampler<D>:
    template<glm::length_t D>
    struct hash<Bplus::GL::Textures::Sampler<D>> {
        size_t operator()(const Bplus::GL::Textures::Sampler<D>& value) {
            return MultiHash(value.Wrapping, value.PixelFilter, value.MipFilter,
                             value.MipOffset, value.MipClampRange,
                             value.DataSource, value.DataSwizzle);
        }
    };
}