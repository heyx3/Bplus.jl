#pragma once

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_access.hpp>

#include "../../Helpers/GuiData.hpp"
#include "../Textures/TexturesData.h"

//"Static" uniform data is data which is constant at shader compile-time.
//As opposed to proper shader uniforms, which can be changed without recompiling the shader.

namespace Bplus::GL::Uniforms
{
    //TODO: Should statics have a prefix too?

    #pragma region Static Uniform Definitions

    //Implies one of a set of #define statements in the shader code
    //   (e.x. '#define USE_PARALLAX_On 1' )
    struct StaticEnum
    {
        //Different suffixes for the #define-d token.
        //E.x. the static uniform "USE_PARALLAX" with value "on"
        //    leads to "#define USE_PARALLAX_on 1".
        std::vector<std::string> Values = { "On", "Off" };

        //The default value, as an index into the "Values" field.
        size_t DefaultValueIdx = 0;
    };
    //Implies a #define statement setting a specific token to some integer value
    //   (e.x. '#define QUALITY_MODE 3' ).
    struct StaticInt
    {
        int64_t Min = 0,
                Max = std::numeric_limits<int64_t>::max();
        int64_t DefaultValue = 0;
    };

    #pragma endregion

    //A static uniform definition.
    using StaticUniformDef_t = std::variant<StaticEnum, StaticInt>;

    //A set of shader-compile-time parameters.
    struct StaticUniformDefs
    {
        std::unordered_map<std::string, StaticUniformDef_t> Definitions;

        //Provides a deterministic ordering for the uniforms,
        //    which helps with hashing/equality.
        std::vector<std::string> Ordering;
    };

    //Stores the values of shader-compile-time parameters.
    //Is hashable.
    struct BP_API StaticUniformValues
    {
        //Needed for hashing.
        StaticUniformDefs Definitions;

        //The values, either integer or enum, for each uniform.
        std::unordered_map<std::string, std::variant<int64_t, std::string>>
            Values;
    };

    #pragma region Static value expressions

    //TODO: Implement these int and bool expressions so we can have custom rules for rendering states in each CompiledShader
    
    #if false

    #pragma region Static Uniform Integer Expressions

    //A constant integer value.
    strong_typedef_start(StaticIntExpr_ConstInt, int64_t, BP_API)
    strong_typedef_end

    //A constant enum value.
    //Can be interpreted as an int by getting the index of this value in the enum definition.
    strong_typedef_start(StaticIntExpr_ConstEnum, std::string, BP_API)
    strong_typedef_end

    //A reference to an integer uniform.
    struct StaticIntExpr_VarInt
    {
        std::string VarName;
        int64_t VarValue;
    };

    //A reference to an enum uniform.
    //Can be interpreted as an int by getting the index of the value from the enum definition.
    struct StaticIntExpr_VarEnum
    {
        std::string VarName, VarValue;
    };

    //TODO: Ternary expession, arithmetic/bitwise expressions.

    #pragma endregion

    //An expression which evaluates to an integer value.
    //This could be a constant, a static uniform lookup, or something else.
    using StaticIntExpr = std::variant<StaticIntExpr_ConstInt,
                                       StaticIntExpr_ConstEnum,
                                       StaticIntExpr_VarInt,
                                       StaticIntExpr_VarEnum>;

    
    #pragma region Static Uniform Bool Expressions

    //The different types of boolean operations that can be performed
    //    on static uniforms.
    BETTER_ENUM(StaticBoolExprTypes, uint_fast16_t,
        Equal, NotEqual,
        Less, LessOrEqual,
        Greater, GreaterOrEqual,
        Or, And, Xor
    );

    //Different kinds of boolean expressions for evaluating static uniforms.
    struct BP_API _StaticBoolExpr
    {
        StaticBoolExprTypes Type = StaticBoolExprTypes::And;

        //The parameter(s) for this expression.
        //Could be some integer arguments to an operation (e.x. a == b),
        //    or a list of inner bool expressions (e.x. a AND b AND c AND ...).
        using Data_t = std::variant<std::vector<StaticIntExpr>,
                                    std::vector<StaticBoolExpr>>;
        Data_t Params = std::vector<StaticBoolExpr>{};

        //Evaluates this expression.
        bool Evaluate(const StaticUniformDefs& uniforms,
                      const StaticUniformValues& values) const;
        //Generates the inverse of this expression.
        StaticBoolExpr Invert() const;
    };

    #pragma endregion

    //An expression which evaluates to a boolean.
    using StaticBoolExpr = _StaticBoolExpr;

    #endif

    #pragma endregion
}

//Make the "static uniform values" struct hashable.
BP_HASHABLE_START(Bplus::GL::Uniforms::StaticUniformValues)
    size_t hashed = 1234567890;
    for (const auto& sName : d.Definitions.Ordering)
        hashed = Bplus::MultiHash(hashed, d.Values.at(sName));
    return hashed;
BP_HASHABLE_END