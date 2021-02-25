#include "IO.h"

#include <cctype>

using namespace Bplus;


bool IO::LoadEntireFile(const fs::path& path, std::string& output)
{
    try
    {
        std::ifstream stream(path);
        if (stream.is_open())
        {
            std::stringstream buffer;
            buffer << stream.rdbuf();
            output += buffer.str();
            return true;
        }
    }
    catch (...) { }

    return false;
}
bool IO::LoadEntireFile(const fs::path& path, std::vector<std::byte>& output)
{
    try
    {
        std::error_code err;
        auto fileSize = fs::file_size(path, err);

        //If we couldn't even read the file size, give up.
        if (err != std::error_code{})
            return false;

        //If the file is too big to fit into a vector or stream, give up.
        
        constexpr auto maxSize = std::numeric_limits<std::streamsize>::max();
        static_assert((uintmax_t)std::numeric_limits<std::streamsize>::max() < (uintmax_t)std::numeric_limits<size_t>::max(),
                      "The limit on the file read buffer size should come from size_t, not std::streamsize");

        auto newVectorSize = Math::SafeAdd(fileSize, output.size());
        if (!newVectorSize.has_value() || *newVectorSize > maxSize)
            return false;

        //Make space for the new data.
        auto offsetI = output.size();
        output.resize(*newVectorSize);

        //Read the data.
        std::ifstream stream(path, std::ios::binary);
        if (stream.is_open())
        {
            stream.read((char*)(&output[offsetI]),
                        (std::streamsize)(fileSize));
            return true;
        }
    }
    catch (...) { }

    return false;
}


std::string IO::ReadEntireFile(const fs::path& path, const std::string& defaultIfMissing)
{
    std::string value;
    if (LoadEntireFile(path, value))
        return value;
    else
        return defaultIfMissing;
}

//Returns whether it was successful.
bool IO::WriteEntireFile(const fs::path& path, const std::string& contents, bool append)
{
    try
    {
        std::ofstream stream = append ?
            std::ofstream(path, std::ios::app) :
            std::ofstream(path, std::ios::trunc);
        if (!stream.is_open())
            return false;

        stream << contents;
        return true;
    }
    catch (...)
    {
        return false;
    }
}
bool IO::WriteEntireFile(const fs::path& path, const std::byte* data, size_t dataSize, bool append)
{
    try
    {
        auto flags = std::ios::binary |
                     (append ? std::ios::app : std::ios::trunc);
        std::ofstream stream = std::ofstream(path, flags);

        if (!stream.is_open())
            return false;

        stream.write((const char*)data, (std::streamsize)dataSize);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

void IO::ToLowercase(std::string& str)
{
    for (auto& c : str)
        c = std::tolower(c);
}
std::string IO::ToLowercase(const char* str)
{
    std::string strClass = str;
    ToLowercase(strClass);
    return strClass;
}

void Bplus::IO::Remove(std::string& str, char c)
{
    str.erase(std::remove_if(str.begin(), str.end(),
                             [c](char c2) { return c == c2; }),
              str.end());
}
void Bplus::IO::RemoveAll(std::string& str, const char* chars, size_t count)
{
    str.erase(std::remove_if(str.begin(), str.end(),
                             [chars, count](char c)
                             {
                                 for (size_t i = 0; i < count; ++i)
                                     if (chars[i] == c)
                                         return true;
                                 return false;
                             }),
              str.end());
}