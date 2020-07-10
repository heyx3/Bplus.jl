#pragma once

#include "../../Math/Box.hpp"
#include "Format.h"

namespace Bplus::GL::Textures
{
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

        using TypeUnion = std::variant<std::byte, //A dummy type that represents nothing,
                                                  //    except that the texture is sampled normally.
                                       DepthStencilSources,
                                       ValueTests>;
        TypeUnion data;
    };
}


//Provide std::hash for the above types.

BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Textures::DepthStencilSources);

BP_HASHABLE_START(Bplus::GL::Textures::SamplerDataSource)
    if (d.IsUnmodified())
        return 0;
    else if (d.IsDepthOrStencil())
        return d.AsDepthOrStencil()._to_integral();
    else if (d.IsDepthComparison())
        return d.AsDepthComparison()._to_integral();
    else {
        BPAssert(false, "Unknown state of SamplerDataSource");
        return 999999999;
    }
BP_HASHABLE_END