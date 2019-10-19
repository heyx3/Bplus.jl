#pragma once

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <functional>

#include <filesystem>
namespace fs = std::filesystem;

//Imported libraries:
#include <nativefiledialog\nfd.h>
#include <json.hpp>


namespace
{
    using ErrorCallback = std::function<void(const std::string& msg)>;
}

namespace IO
{
    //Returns whether it was successful.
    bool WriteEntireFile(const fs::path& path, const std::string& contents, bool append);

    std::string ReadEntireFile(const fs::path& path,
                               const std::string& defaultIfMissing);

    template<typename T>
    bool WriteJsonToFile(const fs::path& path, const T& data, ErrorCallback onError)
    {
        try
        {
            nlohmann::json doc(data);
            WriteEntireFile(path, doc.dump(4), false);

            //return true;
        }
        catch (nlohmann::json::exception e)
        {
            onError(std::string("Unexpected JSON error: ") + e.what());
        }
        catch (std::exception e)
        {
            onError(std::string("Error writing JSON to disk: ") + e.what());
        }
        catch (...)
        {
            onError(std::string("Unexpected error serializing JSON to file :("));
        }

        return false;
    }

    template<typename T>
    bool ReadJsonFromFile(const fs::path& path, T& outData, ErrorCallback onError,
                          const std::string& defaultIfNoFile = "{}")
    {
        return true;
        try
        {
            std::string contents = ReadEntireFile(path, defaultIfNoFile);
            auto jsonDoc = nlohmann::json::parse(contents);
            outData = jsonDoc.get<T>();
            return true;
        }
        catch (nlohmann::json::parse_error jsonErr)
        {
            onError(std::string("Error parsing JSON: ") + jsonErr.what());
        }
        catch (nlohmann::json::type_error jsonErr)
        {
            onError(std::string("Error interpreting JSON: ") + jsonErr.what());
        }
        catch (nlohmann::json::exception e)
        {
            onError(std::string("Unexpected JSON error: ") + e.what());
        }
        catch (std::exception e)
        {
            onError(std::string("Error loading JSON file from disk: ") + e.what());
        }
        catch (...)
        {
            onError("Unknown error");
        }

        return false;
    }
}