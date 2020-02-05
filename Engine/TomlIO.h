#pragma once

#include "IO.h"

//Provides helper functions for serializing/deserializing TOML data.
//The functions all throw Bplus::IO::Exception if something goes wrong.


#include <string>
#include <functional>
#include <optional>
#include <variant>
#include <algorithm>
#include <unordered_set>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_access.hpp>


#include <toml.h>


namespace Bplus::IO
{
    std::string BP_API ToTomlString(const toml::Value& tomlData,
                                    toml::FormatFlag flags = toml::FormatFlag::FORMAT_INDENT);


    #pragma region Get/TryGet for TOML tables

    template<typename T>
    //Gets the TOML field with the given name if it exists,
    //    or a "default" result if it doesn't.
    T TomlTryGet(const toml::Value& object, const char* key,
                 const T& defaultIfMissing = {})
    {
        const auto* found = object.find(key);
        if (found == nullptr)
            return defaultIfMissing;

        try
        {
            return found->as<T>();
        }
        catch (const std::runtime_error& e)
        {
            throw IO::Exception(std::string("TOML field '") + key +
                                    "' exists, but is the wrong type. " + e.what());
        }
    }

    template<typename T>
    //Gets the TOML field with the given name if it exists.
    //Throws an IO::Exception if it doesn't.
    T TomlGet(const toml::Value& object, const char* key)
    {
        const auto* found = object.find(key);
        if (found == nullptr)
            throw IO::Exception(std::string("Unable to find TOML field '") +
                key + "'");

        return TomlTryGet<T>(object, key);
    }

    #pragma endregion

    #pragma region TomlGet/TryGet for TOML arrays

    template<typename T>
    //Gets the TOML array element at the given index if it exists,
    //    or a "default" result if it doesn't.
    T TomlTryGet(const toml::Value& object, size_t index,
                 const T& defaultIfMissing = default)
    {
        const auto* found = object.find(index);
        if (found == nullptr)
            return defaultIfMissing;
        else
            return found->as<T>();
    }

    template<typename T>
    //Gets the TOML array element at the given index if it exists.
    //Throws an IO::Exception if it doesn't.
    T TomlGet(const toml::Value& object, size_t index)
    {
        const auto* found = object.find(index);
        if (found == nullptr)
            throw IO::Exception(std::string("Unable to find TOML array element a[") +
                                  std::to_string(index) + "]");

        return TomlTryGet<T>(object, index);
    }
    
    #pragma endregion
}


//The TOML_MAKE_PARSEABLE macro allows you to extend the "tiny toml" library
//   to add partial native support for parsing custom data types from TOML.
//It allows you to use the built-in Value::as<T>() to convert your own type "T" out of a toml::Value.
//It requires the following arguments:
//    T : the type
//    checkTypeOfV : an expression that outputs whether the Value "V" can be interpreted as the type T
//    getValueOfV : gets the value from V in a form that can be explicitly cast to "T".
//    typeNameStr : the name of the new type as a C-style string (i.e. const char*).
//    templateData : any custom template parameters for specialization
#define TOML_MAKE_PARSEABLE(T, checkTypeOfV, getValueOfV, typeNameStr, templateData) \
    namespace toml { \
        namespace internal { \
            template<templateData> inline const char* type_name<T>() { return typeNameStr; } \
        } \
        template<templateData> struct call_traits<T> : public internal::call_traits_value<T> {}; \
        template<templateData> struct Value::ValueConverter<T> \
        { \
            bool is(const Value& V) {                    return checkTypeOfV; } \
               T to(const Value& V) { V.assureType<T>(); return (T)(getValueOfV); } \
        }; \
    }


#pragma region Extend TOML Value.as<T>() for all integer/float types, and struct Bool

TOML_MAKE_PARSEABLE(int8_t, V.type() == Value::INT_TYPE, V.int_, "int8_t")
TOML_MAKE_PARSEABLE(int16_t, V.type() == Value::INT_TYPE, V.int_, "int16_t")
TOML_MAKE_PARSEABLE(int32_t, V.type() == Value::INT_TYPE, V.int_, "int32_t")
TOML_MAKE_PARSEABLE(uint8_t, V.type() == Value::INT_TYPE, V.int_, "uint8_t")
TOML_MAKE_PARSEABLE(uint16_t, V.type() == Value::INT_TYPE, V.int_, "uint16_t")
TOML_MAKE_PARSEABLE(uint32_t, V.type() == Value::INT_TYPE, V.int_, "uint32_t")
TOML_MAKE_PARSEABLE(uint64_t, V.type() == Value::INT_TYPE, V.int_, "uint64_t")

TOML_MAKE_PARSEABLE(float, V.type() == Value::DOUBLE_TYPE, V.double_, "float")

TOML_MAKE_PARSEABLE(Bool, V.type() == Value::BOOL_TYPE, V.bool_, "Bool")

#pragma endregion

#pragma region Extend TOML Value.as<T>() for all GLM vectors, as array or table or single values

//Define helper functions for pulling a vector from a TOML Value.
namespace Bplus::IO::internal
{
    template<typename glmVec_t>
    bool glmVectorCheckToml(const toml::Value& v)
    {
        //"size" is well-defined for all toml data types,
        //  so we can check it immediately.
        if (v.size() != glmVec_t::length())
            return false;

        //Support arrays of numbers.
        if (v.type() == toml::Value::ARRAY_TYPE)
        {
            //Check element types.
            for (glm::length_t i = 0; i < glmVec_t::length(); ++i)
                if (!v.is<glmVec_t::value_type>())
                    return false;

            return true;
        }
        //Support tables of X/Y/Z/W values.
        else if (glmVec_t::length() < 5 && v.type() == toml::Value::TABLE_TYPE)
        {
            //Expect "xyzw", "XYZW", "rgba", or "RGBA".
            return (v.has("x") || v.has("X") || v.has("r") || v.has("R")) &&
                   (glmVec_t::length() < 2 || v.has("y") || v.has("Y") || v.has("g") || v.has("G")) &&
                   (glmVec_t::length() < 3 || v.has("z") || v.has("Z") || v.has("b") || v.has("B")) &&
                   (glmVec_t::length() < 4 || v.has("w") || v.has("W") || v.has("a") || v.has("A"));
        }
        //Support individual values for 1D vectors.
        else if (glmVec_t::length() == 1)
        {
            return v.is<glmVec_t::value_type>();
        }
        else
        {
            return false;
        }
    }

    template<typename glmVec_t>
    glmVec_t glmVectorFromToml(const toml::Value& v)
    {
        //Note that we're assuming the value is valid for this type,
        //    checked via the above "glmVectorCheckToml()".

        BPAssert(v.size() == glmVec_t::length(),
                 "Expected size to match up");

        if (v.type() == toml::Value::ARRAY_TYPE)
        {
            glmVec_t result;
            for (glm::length_t i = 0; i < result.length(); ++i)
                result[i] = v.get<glmVec_t::value_type>(i);
            return result;
        }
        else if (v.type() == toml::Value::TABLE_TYPE)
        {
            glmVec_t result;

            for (glm::length_t i = 0; i < result.length(); ++i)
            {
                glmVec_t::value_type c;

                static const char* modes[] = { "xyzw", "XYZW", "rgba", "RGBA" };
                for (uint_fast8_t m = 0; m < 4; ++m)
                {
                    auto str = std::string() + modes[m][i];

                    if (v.has(str))
                    {
                        c = v.get<glmVec_t::value_type>(str);
                        break;
                    }

                    BPAssert(m != 3, "Couldn't find a component");
                }

                result[i] = c;
            }

            return result;
        }
        else
        {
            BPAssert(v.size() == 1, "Expected 1D vector value");
            return glmVec_t(v.as<glmVec_t::value_type>());
        }
    }

    template<glm::length_t L, typename T>
    const char* glmVectorTypeName()
    {
        static std::optional<std::string> name;
        if (!name.has_value())
        {
            if constexpr (std::is_same_v<T, int32_t>)
                name = "ivec";
            else if constexpr (std::is_same_v<T, uint32_t>)
                name = "uvec";
            else if constexpr (std::is_same_v<T, float>)
                name = "fvec";
            else if constexpr (std::is_same_v<T, double>)
                name = "dvec";
            else if constexpr (std::is_same_v<T, bool>)
                name = "bvec";
            else
                name = "?vec";

            name += std::to_string(L);
        }

        return name.c_str();
    }
}

TOML_MAKE_PARSEABLE(glm::vec<L BP_COMMA T>,
                    Bplus::IO::internal::glmVectorCheckToml<L BP_COMMA T>(V),
                    Bplus::IO::internal::glmVectorFromToml<L BP_COMMA T>(V),
                    Bplus::IO::internal::glmVectorTypeName<L BP_COMMA T>(V),
                    glm::length_t L BP_COMMA typename T);

template<typename glmVec_t>
toml::Value GlmToToml(const glmVec_t& v)
{
    toml::Value outToml;
        
    //If it's just one value, don't bother with an array.
    if constexpr (L == 1)
        outToml = v[0];
    else for (glm::length_t i = 0; i < L; ++i)
        outToml.push(v[(size_t)i]);

    return outToml;
}

#pragma endregion

#pragma region Extend TOML Value.as<T>() for all GLM matrices, as array or table or single values

//Define helper functions for pulling a matrix from a TOML Value.
namespace Bplus::IO::internal
{
    template<glm::length_t C, glm::length_t R, typename T>
    bool glmMatrixCheckToml(const toml::Value& v)
    {
        if (v.type() == toml::Value::Type::ARRAY_TYPE)
        {
            if (v.size() != R)
                return false;

            for (size_t r = 0; r < R; ++r)
            {
                auto row = *v.find(r);
                if (!row.is<glm::mat<C, R, T>::row_type>())
                    return false;
            }

            return true;
        }
        else
        {
            return C == 1 && R == 1 &&
                   v.is<T>();
        }
    }

    template<glm::length_t C, glm::length_t R, typename T>
    glm::mat<C, R, T> glmMatrixFromToml(const toml::Value& v)
    {
        if (v.type() == toml::Value::Type::ARRAY_TYPE)
        {
            glm::mat<C, R, T> result;
            for (size_t r = 0; r < R; ++r)
                glm::row(result, r, v.find(r)->as<glm::mat<C, R, T>::row_type>());
            return result;
        }
        else
        {
            BPAssert(C == 1 && R == 1 && v.is<T>(),
                     "Invalid single-element matrix");
            return glm::mat<C, R, T>(v.as<T>());
        }
    }
    
    template<glm::length_t C, glm::length_t R, typename T>
    const char* glmMatrixTypeName()
    {
        static std::optional<std::string> name;
        if (!name.has_value())
        {
            if constexpr (std::is_same_v<T, int32_t>)
                name = "imat";
            else if constexpr (std::is_same_v<T, uint32_t>)
                name = "umat";
            else if constexpr (std::is_same_v<T, float>)
                name = "fmat";
            else if constexpr (std::is_same_v<T, double>)
                name = "dmat";
            else if constexpr (std::is_same_v<T, bool>)
                name = "bmat";
            else
                name = "?mat";

            name += std::to_string(C) + "x" + std::to_string(R);
        }

        return name.value().c_str();
    }
}

TOML_MAKE_PARSEABLE(glm::mat<C BP_COMMA R BP_COMMA T>,
                    Bplus::IO::internal::glmMatrixCheckToml<C BP_COMMA R BP_COMMA T>(V),
                    Bplus::IO::internal::glmMatrixFromToml<C BP_COMMA R BP_COMMA T>(V),
                    Bplus::IO::internal::glmMatrixTypeName<C BP_COMMA R BP_COMMA T>(V),
                    glm::length_t C BP_COMMA glm::length_t R BP_COMMA typename T);

template<glm::length_t C, glm::length_t R, typename T>
toml::Value GlmToToml(const glm::mat<C, R, T>& m)
{
    toml::Value outToml;

    //If it's just one value, don't bother with an array of arrays.
    if constexpr (C == 1 && R == 1)
        outToml = m[0][0];
    //Otherwise, insert each row into an array.
    else for (glm::length_t r = 0; r < R; ++r)
        outToml.push(GlmToToml<glm::mat<C, R, T>::row_type>(glm::row(m, r)));

    return outToml;
}

#pragma endregion