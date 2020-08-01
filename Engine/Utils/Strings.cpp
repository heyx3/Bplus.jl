#include "Strings.h"

bool Bplus::Strings::StartsWith(const std::string& str, const std::string& snippet)
{
    return strncmp(str.c_str(), snippet.c_str(), snippet.size()) == 0;
}
bool Bplus::Strings::EndsWith(const std::string& str, const std::string& snippet)
{
    return strncmp(str.c_str() + str.size() - snippet.size(),
                   snippet.c_str(), snippet.size()) == 0;
}