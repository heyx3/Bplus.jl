#pragma once

//Provides TOML reading/writing for the various rendering-related data structures.
//Like other IO functions, 

#include "../TomlIO.h"
#include "Data.h"

namespace Bplus::IO
{
    //Value_t can be any of the enums or structs in "Engine/Renderer/Data.h".
    template<typename Value_t>
    Value_t FromToml(const toml::Value& value)
    {
        if constexpr (std::is_same_v<Value_t, GL::VsyncModes> ||
                      std::is_same_v<Value_t, GL::FaceCullModes> ||
                      std::is_same_v<Value_t, GL::ValueTests> ||
                      std::is_same_v<Value_t, GL::StencilOps> ||
                      std::is_same_v<Value_t, GL::BlendFactors> ||
                      std::is_same_v<Value_t, GL::BlendOps>)
        {
            if (value.type() != toml::Value::Type::STRING_TYPE)
            {
                throw new IO::Exception(std::string("Expected an enum string such as '") +
                                          Value_t::_from_index(0)._to_string() + "', but got " +
                                          toml::Value::typeToString(value.type()));
            }

            return IO::EnumFromString<Value_t>(value.as<std::string>());
        }
        else if constexpr (std::is_same_v<Value_t, GL::BlendStateAlpha>)
        {
            if (value.type() != toml)
        }
        else
        {
            static_assert(false, "Unexpected type T in Bplus::IO::FromToml<T>()");
        }
    }
}