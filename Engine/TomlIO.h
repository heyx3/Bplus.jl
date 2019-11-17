#pragma once

//Provides helper functions for serializing/deserializing TOML data.
//The functions all follow the IO namespace's standard
//    of using IO::Exception if something goes wrong.

#include "IO.h"

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <toml.h>


namespace Bplus::IO
{
    std::string BP_API ToTomlString(const toml::Value& tomlData,
                                    toml::FormatFlag flags = toml::FormatFlag::FORMAT_INDENT);

    #pragma region Get/TryGet for tables

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

    #pragma region Get/TryGet for arrays

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

    #pragma region Converters between TOML-supported numbers and all int/float/bool

    //TOML has limited support for the variety of number types.
    //This helper function converts floats to double, ints to int64,
    //    and leaves booleans alone (included for GLM).
    template<typename T>
    auto ToTomlNumber(T rawValue)
    {
        if constexpr (std::is_floating_point_v<T>)
            return (double)rawValue;
        else if constexpr (std::is_integral_v<T>)
            return (int64_t)rawValue;
        else //Assume it's a boolean.
            return (bool)rawValue;
    }

    //TOML has limited support for the variety of number types.
    //This helper function reads a TOML value as one of the supported types,
    //    and converts it to the desired float, integer, or bool type.
    template<typename T>
    auto FromTomlNumber(const toml::Value& value)
    {
        if constexpr (std::is_floating_point_v<T>)
            return (T)value.as<double>();
        else if constexpr (std::is_integral_v<T>)
            return (T)value.as<int64_t>();
        else
            return value.as<bool>();
    }

    #pragma endregion

    #pragma region Enum value from TOML string

    //Enum_t should be an enum created with the better_enums library.
    template<typename Enum_t>
    //Converts the given toml table's field from a string to an enum.
    Enum_t EnumFromString(const toml::Value& toml, const char* key,
                          bool caseSensitive = false)
    {
        try
        {
            const auto& value = toml.get<std::string>(key);
            if (caseSensitive)
                return Enum_t::_from_string(value.c_str());
            else
                return Enum_t::_from_string_nocase(value.c_str());
        }
        catch (const std::runtime_error& e)
        {
            throw IO::Exception(std::string("TOML field '") + key + "' unable to be\
 parsed as a string. " + e.what());
        }
    }

    #pragma endregion

    #pragma region Converters for GLM vectors

    template<glm::length_t L, typename T, glm::qualifier Q = glm::packed_highp>
    toml::Value ToToml(const glm::vec<L, T, Q>& v)
    {
        toml::Value outToml;
        
        //If it's just one value, don't bother with an array.
        if constexpr (L == 1)
            outToml = v[0];
        else for (glm::length_t i = 0; i < L; ++i)
            outToml.push(v[(size_t)i]);

        return outToml;
    }

    template<glm::length_t L, typename T, glm::qualifier Q = glm::packed_highp>
    glm::vec<L, T, Q> FromToml(const toml::Value& inToml)
    {
        glm::vec<L, T, Q> v;

        //Note that, if it's a 1D vector, it doesn't have to be an array of values --
        //    it can just be a lone naked value.

        if (inToml.type() != toml::Value::ARRAY_TYPE)
            if constexpr (L == 1)
                return glm::vec<L, T, Q>(FromTomlNumber<T>(inToml));
            else
                throw IO::Exception("Vector value isn't a TOML array");

        if (inToml.size() != L)
            throw IO::Exception(std::string("Vector has ") + std::to_string(inToml.size()) +
                                    " elements instead of the expected " + std::to_string(L));

        for (glm::length_t i = 0; i < L; ++i)
            v[i] = FromTomlNumber<T>(*inToml.find(i));

        return v;
    }

    #pragma endregion
}