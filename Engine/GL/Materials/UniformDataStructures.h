#pragma once

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_access.hpp>

#include "../Textures/Sampler.h"
#include "../Textures/Format.h"
#include "../../Helpers/GuiData.hpp"

namespace Bplus::GL::Materials::Uniforms
{
    //The allowable dimensionality of OpenGL vectors.
    BETTER_ENUM(VectorSizes, glm::length_t,
        One = 1,
        Two = 2,
        Three = 3,
        Four = 4
    );
    //The allowable dimensionality of OpenGL matrices.
    BETTER_ENUM(MatrixSizes, glm::length_t,
        //A matrix with 1 row or 1 column is not allowed in OpenGL; just use a vector.
        Two = 2,
        Three = 3,
        Four = 4
    );

    //The scalar types available in GLSL shaders.
    BETTER_ENUM(ScalarTypes, uint8_t,
        Float, Double,
        Int, UInt,
        Bool
    );

    //The types of texture samplers available.
    BETTER_ENUM(SamplerTypes, uint8_t,
        //Texture data comes out as a float. This is the norm.
        Float,
        //Texture data comes out as an integer/unsigned integer.
        Int, UInt,
        //Texture data comes out as a comparison against a particular "depth" value.
        Shadow
    );


    //Scalar/vector uniform data.
    struct Vector
    {
        VectorSizes D = VectorSizes::Four;
        ScalarTypes Type = ScalarTypes::Float;

        //All value/range data is stored as the largest, safest type in its category --
        //    double or int64.

        std::variant<glm::dvec4, glm::i64vec4>
            DefaultValue = glm::dvec4{ 0, 0, 0, 1 };

        std::variant<GuiData::VectorDataRange<double>,
                     GuiData::VectorDataRange<int64_t>>
            Range = GuiData::VectorChannelDataRange<double>{ std::nullopt };
    };

    //A float or double matrix, from 2x2 to 4x4 in size.
    struct Matrix
    {
        MatrixSizes Rows = MatrixSizes::Four,
                    Columns = MatrixSizes::Four;

        //If true, this is a matrix of 64-bit doubles instead of 32-bit floats.
        bool IsDouble = false;
    };

    //Color data (greyscale, RG, RGB, or RGBA).
    struct Color
    {
        Textures::SimpleFormatComponents Channels = Textures::SimpleFormatComponents::RGBA;
        glm::fvec4 Default = { 1, 0, 1, 1 };
        bool IsHDR = false;
    };

    struct Gradient : public GuiData::Curve<4, float>
    {
        Textures::SimpleFormatComponents Channels = Textures::SimpleFormatComponents::RGBA;
        bool IsHDR = false;
        uint16_t Resolution = 128;
    };

    //A texture/sampler.
    struct TexSampler
    {
        //The expected texture type.
        Textures::Types Type = Textures::Types::OneD;
        //Hard-coded sampler settings (otherwise, the texture's default sampler will be used).
        //The full 3-dimensional sampler settings are stored here,
        //    even if the texture is less than 3-dimensional.
        std::optional<Textures::Sampler<3>> FullSampler;

        SamplerTypes SamplingType = SamplerTypes::Float;
    };

    //A reference to a struct, by its type-name.
    strong_typedef_start(StructInstance, std::string, BP_API)
        strong_typedef_equatable
        strong_typedef_defaultConstructor(StructInstance, std::string())
    strong_typedef_end


    //The main definition for a uniform.
    struct UniformType
    {
        //If this uniform is an array,
        //    this field provides its size.
        //Otherwise, this value is 0.
        uint32_t ArrayCount = 0;

        std::variant<Vector, Matrix, Color, TexSampler, StructInstance> ElementType;

        bool IsArray() const { return ArrayCount == 0; }
    };
    
    //A definition of a struct, which can then be used as a uniform type.
    struct UniformStructDef
    {
        std::string Name;
        //A list instead of a dictionary, so that they're ordered.
        std::vector<std::pair<std::string, UniformType>> Fields;
    };
}

//TODO: Toml IO helpers

//Add hashes for some of the data structures:
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Materials::Uniforms::VectorSizes);
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Materials::Uniforms::MatrixSizes);
strong_typedef_hashable(Bplus::GL::Materials::Uniforms::StructInstance);