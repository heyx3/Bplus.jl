#pragma once

#include "../../Math/Box.hpp"
#include "Format.h"

namespace Bplus::GL::Textures
{
    //The different sources a color texture sampler can pull from.
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
    BETTER_ENUM(DepthStencilSources, GLenum,
        //The texture will sample its depth and output floats (generally between 0-1).
        Depth = GL_DEPTH_COMPONENT,
        //The texture will sample its stencil and output unsigned integers.
        Stencil = GL_STENCIL_INDEX
    );


    //A hybrid of the different ways a texture can read from its pixels:
    //    * Unmodified (plain RGBA or Depth).
    //    * Picking between depth or stencil in a hybrid depth-stencil texture.
    //    * Comparing a depth texture's pixels to a "test" value,
    //          outputting 1 if the test passes and 0 if it fails.
    //          Outputs greyscale values if using Smooth filtering.
    struct BP_API SamplerDataSource
    {
        //The texture samples its data like normal.
        //If it's a hybrid depth-stencil, it will sample only the stencil.
        SamplerDataSource() : data(std::byte{ 0 }) { }
        //The texture is a depth-stencil hybrid and samples from either depth or stencil.
        SamplerDataSource(DepthStencilSources component) : data(component) { }
        //The texture is depth or depth-stencil, and compares its depth samples to a "test" value.
        SamplerDataSource(ValueTests test) : data(test) { }


        bool IsUnmodified()      const { return std::holds_alternative<std::byte>(data); }
        bool IsDepthOrStencil()  const { return std::holds_alternative<DepthStencilSources>(data); }
        bool IsDepthComparison() const { return std::holds_alternative<ValueTests>(data); }

        DepthStencilSources AsDepthOrStencil()  const { return std::get<DepthStencilSources>(data); }
        ValueTests          AsDepthComparison() const { return std::get<ValueTests>(data); }


        bool operator==(const SamplerDataSource& other) const { return data == other.data; }
        bool operator!=(const SamplerDataSource& other) const { return !operator==(other); }

        bool operator==(DepthStencilSources source) const { return IsDepthOrStencil() && AsDepthOrStencil() == source; }
        bool operator!=(DepthStencilSources source) const { return !operator==(source); }

        bool operator==(ValueTests test) const { return IsDepthComparison() && AsDepthComparison() == test; }
        bool operator!=(ValueTests test) const { return !operator==(test); }


    private:

        using TypeUnion = std::variant<std::byte,
                                       DepthStencilSources,
                                       ValueTests>;
        TypeUnion data;
    };
}

//Allow enums and structs in this file to be hashed, for use in STL collections.
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Textures::SwizzleSources);
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Textures::DepthStencilSources);
namespace std
{
    //SamplerDataSource:
    template<>
    struct hash<Bplus::GL::Textures::SamplerDataSource> {
        size_t operator()(const Bplus::GL::Textures::SamplerDataSource& value) {
            if (value.IsUnmodified())
                return 0;
            else if (value.IsDepthOrStencil())
                return value.AsDepthOrStencil()._to_integral();
            else if (value.IsDepthComparison())
                return value.AsDepthComparison()._to_integral();
            else
            {
                BPAssert(false, "Unknown state of SamplerDataSource");
                return 0;
            }
        }
    };
}