#include "TomlIO.h"

#include <sstream>

using namespace Bplus::IO;

std::string ToTomlString(const toml::Value& tomlData, toml::FormatFlag flags)
{
    std::stringstream ss;
    tomlData.writeFormatted(&ss, flags);
    return ss.str();
}