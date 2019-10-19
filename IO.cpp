#include "IO.h"


std::string IO::ReadEntireFile(const fs::path& path, const std::string& defaultIfMissing)
{
    try
    {
        std::ifstream stream(path);
        if (stream.is_open())
        {
            std::stringstream buffer;
            buffer << stream.rdbuf();
            return buffer.str();
        }
    }
    catch (...) { }

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