#include "TomlIO.h"

#include <sstream>

using namespace Bplus;
using namespace Bplus::IO;


std::string TomlToString(const toml::Value& tomlData, toml::FormatFlag flags)
{
    std::stringstream ss;
    tomlData.writeFormatted(&ss, flags);
    return ss.str();
}