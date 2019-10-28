#pragma once

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <functional>

#include <filesystem>
namespace fs = std::filesystem;

//Imported libraries:
#include "NativeFileDialog\nfd.h"
#include <toml.h>

#include "Platform.h"


namespace Bplus
{
    //TODO: Refactor into a more robust logging system.
    using ErrorCallback = std::function<void(const std::string& msg)>;

    namespace IO
    {
        //Returns whether it was successful.
        bool BP_API WriteEntireFile(const fs::path& path, const std::string& contents, bool append);

        std::string BP_API ReadEntireFile(const fs::path& path,
                                          const std::string& defaultIfMissing);

        template<typename T>
        T TomlTryGet(const toml::Value& object, const char* key,
            const T& defaultIfMissing = default)
        {
            const auto* found = object.find(key);
            if (found == nullptr)
                return defaultIfMissing;
            else
                return found->as<T>();
        }
        template<typename T>
        T TomlTryGet(const toml::Value& object, size_t index,
                     const T& defaultIfMissing = default)
        {
            const auto* found = object.find(index);
            if (found == nullptr)
                return defaultIfMissing;
            else
                return found->as<T>();
        }

        void ToLowercase(std::string& str);
    }
}