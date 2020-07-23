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