#pragma once

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_access.hpp>

#include "../../Helpers/GuiData.hpp"
#include "../Textures/TexturesData.h"

namespace Bplus::GL::Uniforms
{
    #pragma region Enums

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

    //The types of texture samplers available,
    //    in terms of the data types that come from sampling the texture.
    BETTER_ENUM(SamplerTypes, uint8_t,
        //Texture data comes out as a float. This is the norm.
        Float,
        //Texture data comes out as an integer/unsigned integer.
        Int, UInt,
        //Texture data comes out as a comparison against a particular "depth" value.
        Shadow
    );

    #pragma endregion

    #pragma region Uniform data types

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

    //TODO: SSBO buffer definition. Very similar to a struct definition,
    //    but with additional "memory qualifiers"
    //    and the last element can be a dynamically-sized array.
    //    https://www.khronos.org/opengl/wiki/Shader_Storage_Buffer_Object


    //A reference to a struct, by its type-name.
    strong_typedef_start(StructInstance, std::string, BP_API)
        strong_typedef_equatable
        strong_typedef_defaultConstructor(StructInstance, std::string())
    strong_typedef_end

    #pragma endregion


    //TODO: Define "static" uniform data types, a.k.a. types that decide compile-time parameters.



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
    

    //A struct is defined by its fields.
    //The fields are well-ordered.
    using UniformStructDef = std::vector<std::pair<std::string, UniformType>>;


    //A set of uniform definitions for a shader.
    struct BP_API UniformDefinitions
    {
        std::unordered_map<std::string, UniformStructDef> Structs;
        std::unordered_map<std::string, UniformType> Uniforms;

        //Tries to add the given uniforms/structs to this instance.
        //Returns an error message, or an empty string if everything went fine.
        std::string Import(const UniformDefinitions& newDefs);
    };
}

//TODO: Toml IO helpers
//TODO: ImGui helpers

//Add hashes for some of the data structures:
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Uniforms::VectorSizes);
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Uniforms::MatrixSizes);
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Uniforms::SamplerTypes);
strong_typedef_hashable(Bplus::GL::Uniforms::StructInstance);