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

void Bplus::Strings::Replace(std::string& str,
                             const std::string& snippet, const std::string& replacedWith)
{
    if (str.empty())
        return;

    size_t startPos;
    while ((startPos = str.find(snippet, startPos)) != std::string::npos)
    {
        str.replace(startPos, snippet.length(), replacedWith);
        startPos += replacedWith.length();
    }
}