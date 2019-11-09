#pragma once

//Provides helper functions for serializing/deserializing TOML data.
//The functions all follow the IO namespace's standard
//    of using IO::Exception if something goes wrong.

#include "IO.h"

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <better_enums.h>
#include <toml.h>


namespace Bplus::IO
{
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


    template<glm::length_t L, class T>
    toml::Value ToToml(const glm::vec<L, T>& v)
    {
        toml::Value outToml;
        for (glm::length_t i = 0; i < L; ++i)
            outToml.push(v[i]);
    }

    template<glm::length_t L, class T>
    glm::vec<L, T> FromToml(const toml::Value& inToml)
    {
        glm::vec<L, T>& v;

        try
        {
            for (glm::length_t i = 0; i < L; ++i)
                v[i] = inToml.get(i);
        }
        catch (const std::runtime_error& e)
        {
            throw IO::Exception(std::string("Unable to parse value as Xvec") +
                                  std::to_string(L) + ": " + e.what());
        }

        return v;
    }

    //For 1D vectors, don't bother with an array of values;
    //    just serialize the value directly.
    template<class T>
    toml::Value ToToml(const glm::vec<1, T>& v)
    {
        toml::Value val;
        val = v[0];
        return val;
    }
    template<class T>
    glm::vec<1, T> FromToml(const toml::Value& inToml)
    {
        try
        {
            T t = inToml.as<T>();
            return glm::vec<1, T>(t);
        }
        catch (const std::runtime_error& e)
        {
            throw IO::Exception(std::string("Unable to parse value as its expected type\
 in the vector: ") + e.what());
        }
    }


    //TOML has limited support for the variety of integer types.
    inline BP_API int32_t ToTomlInt(uint32_t u) { return *((int32_t*)(&u)); }
    inline BP_API uint32_t FromTomlInt(int32_t i) { return *((uint32_t*)(&i)); }
}