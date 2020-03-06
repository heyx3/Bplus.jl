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
    std::string BP_API TomlToString(const toml::Value& tomlData,
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

    #pragma region ToToml, an extendable way to convert custom types to TOML

    template<typename T, typename Traits = void>
    //Specialize this struct to support custom types for ToToml<T>.
    //This fallback implementation just uses a toml::Value constructor.
    struct _ToToml
    {
        static toml::Value Convert(const T& t)
        {
            return { t };
        }
    };


    template<typename T>
    //Converts B+ data to TOML.
    //Out of the box, supports all built-in TOML types,
    //    number types, Bool, GLM vectors and matrices,
    //    and all BETTER_ENUM enums.
    //Support new data types by specializing the Bplus::IO::_ToToml<> struct.
    toml::Value ToToml(const T& t) { return _ToToml<T>::Convert(t); }


    //Numbers/bools:
#define MAKE_TO_TOML_CAST(baseType, tomlType) \
        template<> struct _ToToml<baseType> { \
            static toml::Value Convert(baseType n) { \
                return (tomlType)n; \
            } \
        }
    MAKE_TO_TOML_CAST(int8_t, int64_t);
    MAKE_TO_TOML_CAST(int16_t, int64_t);
    MAKE_TO_TOML_CAST(int32_t, int64_t);
    MAKE_TO_TOML_CAST(uint8_t, int64_t);
    MAKE_TO_TOML_CAST(uint16_t, int64_t);
    MAKE_TO_TOML_CAST(uint32_t, int64_t);
    MAKE_TO_TOML_CAST(uint64_t, int64_t);
    MAKE_TO_TOML_CAST(float, double);
    MAKE_TO_TOML_CAST(Bool, bool);
#undef MAKE_TO_TOML_CAST

    //BETTER_ENUMs:
    template<typename Enum_t>
    struct _ToToml<Enum_t,
                   std::enable_if_t<is_better_enum_v<Enum_t>, void>>
    {
        static toml::Value Convert(Enum_t e)
        {
            return e._to_string();
        }
    };

    //GLM vectors:
    template<glm::length_t L, typename T>
    struct _ToToml<glm::vec<L, T>>
    {
        using vec_t = glm::vec<L, T>;
        static toml::Value Convert(const vec_t& v)
        {
            auto result = toml::Array();
            result.resize(L);

            for (size_t i = 0; i < L; ++i)
                result[i] = v[i];

            return result;
        }
    };

    //GLM matrices:
    template<glm::length_t C, glm::length_t R, typename T>
    struct _ToToml<glm::mat<C, R, T>>
    {
        using mat_t = glm::mat<C, R, T>;
        static toml::Value Convert(const mat_t& m)
        {
            auto result = toml::Array();
            result.resize(R);

            for (glm::length_t r = 0; r < R; ++r)
                result.push_back(ToToml<mat_t::row_type>(glm::row(m, r)));
        }
    };

    #pragma endregion

    #pragma region TomlWrap/TomlUnwrap

    //Wraps a single value into a valid TOML file.
    template<typename T>
    toml::Value TomlWrap(const T& t)
    {
        toml::Value tVal;
        tVal["t"] = ToToml(t);
        return tVal;
    }

    //Unwraps a value that was wrapped by TomlWrap().
    template<typename T>
    T TomlUnwrap(const toml::Value& v)
    {
        return v.get<T>("t");
    }

    #pragma endregion
}

#pragma region Extend TOML Value.as<T>() for more types

//The TOML_MAKE_PARSEABLE macro allows you to extend the "tiny toml" library
//   to add partial native support for parsing custom data types from TOML.
//It allows you to use the built-in Value::as<T>() to convert your own type "T" out of a toml::Value.
//It requires the following arguments:
//    T : the type
//    templateOuter : the template arguments for the specialization
//    templateInner : the template instantiation for the specialization
//    checkTypeOfV : an expression that outputs whether the Value "V" can be interpreted as the type T
//    getValueOfV : gets the value from V in a form that can be explicitly cast to "T".
//    typeNameStr : the name of the new type as a C-style string (i.e. const char*).
//    value_or_ref : "value" if the result is passed by value; "ref" if the result is passed by const reference.
#define TOML_MAKE_PARSEABLE(T, templateOuter, templateInner, checkTypeOfV, getValueOfV, typeNameStr, value_or_ref) \
    namespace toml { \
        namespace internal { \
            template<templateOuter> struct type_name<templateInner> \
            { \
                static const char* N() { return typeNameStr; } \
            }; \
        } \
        template<templateOuter> struct call_traits<templateInner> \
            : public internal::call_traits_##value_or_ref<T> {}; \
        template<templateOuter> struct Value::ValueConverter<templateInner> \
        { \
            bool is(const Value& V) {                    return checkTypeOfV; } \
               T to(const Value& V) { V.assureType<T>(); return (T)(getValueOfV); } \
        }; \
    }


#pragma region Integer/float types and struct Bool

TOML_MAKE_PARSEABLE(int8_t,   , int8_t,   V.type() == Value::INT_TYPE, V.int_, "int8_t",   value)
TOML_MAKE_PARSEABLE(int16_t,  , int16_t,  V.type() == Value::INT_TYPE, V.int_, "int16_t",  value)
TOML_MAKE_PARSEABLE(uint8_t,  , uint8_t,  V.type() == Value::INT_TYPE, V.int_, "uint8_t",  value)
TOML_MAKE_PARSEABLE(uint16_t, , uint16_t, V.type() == Value::INT_TYPE, V.int_, "uint16_t", value)
TOML_MAKE_PARSEABLE(uint32_t, , uint32_t, V.type() == Value::INT_TYPE, V.int_, "uint32_t", value)
TOML_MAKE_PARSEABLE(uint64_t, , uint64_t, V.type() == Value::INT_TYPE, V.int_, "uint64_t", value)

TOML_MAKE_PARSEABLE(float, , float, V.type() == Value::DOUBLE_TYPE, V.double_, "float", value)

TOML_MAKE_PARSEABLE(Bool, , Bool, V.type() == Value::BOOL_TYPE, V.bool_, "Bool", value)

#pragma endregion

#pragma region BETTER_ENUMs

TOML_MAKE_PARSEABLE(Enum_t, typename Enum_t,
                    Enum_t BP_COMMA std::enable_if_t<is_better_enum_v<Enum_t> BP_COMMA void>,
                    (V.type() == toml::Value::Type::INT_TYPE) ||
                      (V.type() == toml::Value::Type::STRING_TYPE),
                    (V.type() == toml::Value::Type::INT_TYPE) ?
                        Enum_t::_from_index(V.as<size_t>()) :
                        Bplus::IO::EnumFromString<Enum_t>(V.as<std::string>()),
                    Enum_t::_name(),
                    value);

#pragma endregion

#pragma region GLM vectors

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

    template<glm::length_t L, typename T>
    glm::vec<L, T> glmVectorFromToml(const toml::Value& v)
    {
        using glmVec_t = glm::vec<L, T>;

        //Note that we're assuming the value is valid for this type,
        //    checked via the above "glmVectorCheckToml()".
        BPAssert(v.size() == L,
                 "Expected size to match up");

        if (v.type() == toml::Value::ARRAY_TYPE)
        {
            glmVec_t result;
            for (glm::length_t i = 0; i < result.length(); ++i)
                result[i] = v.get<T>(i);
            return result;
        }
        else if (v.type() == toml::Value::TABLE_TYPE)
        {
            glmVec_t result;

            for (glm::length_t i = 0; i < result.length(); ++i)
            {
                T c;

                static const char* modes[] = { "xyzw", "XYZW", "rgba", "RGBA" };
                for (uint_fast8_t m = 0; m < 4; ++m)
                {
                    auto str = std::string() + modes[m][i];

                    if (v.has(str))
                    {
                        c = v.get<T>(str);
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
            return glmVec_t(v.as<T>());
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

        return name.value().c_str();
    }
}

TOML_MAKE_PARSEABLE(glm::vec<L BP_COMMA T>,
                    glm::length_t L BP_COMMA typename T,
                    glm::vec<L BP_COMMA T>,
                    Bplus::IO::internal::glmVectorCheckToml<glm::vec<L BP_COMMA T>>(V),
                    Bplus::IO::internal::glmVectorFromToml<L BP_COMMA T>(V),
                    Bplus::IO::internal::glmVectorTypeName<L BP_COMMA T>(),
                    value);

#pragma endregion

#pragma region GLM matrices

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
                    glm::length_t C BP_COMMA glm::length_t R BP_COMMA typename T,
                    glm::mat<C BP_COMMA R BP_COMMA T>,
                    Bplus::IO::internal::glmMatrixCheckToml<C BP_COMMA R BP_COMMA T>(V),
                    Bplus::IO::internal::glmMatrixFromToml<C BP_COMMA R BP_COMMA T>(V),
                    Bplus::IO::internal::glmMatrixTypeName<C BP_COMMA R BP_COMMA T>(),
                    value);

#pragma endregion

#pragma endregion