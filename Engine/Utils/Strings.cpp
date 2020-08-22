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
    //Edge-case: the snippet is empty;
    //    we should avoid the infinite-loop in this case.
    if (snippet.empty())
        return;

    //Repeatedly apply the built-in "find()" and "replace()" on the string.
    size_t startPos = 0;
    while ((startPos = str.find(snippet, startPos)) != std::string::npos)
    {
        str.replace(startPos, snippet.length(), replacedWith);
        //Skip over the new text when searching for a snippet.
        startPos += replacedWith.length();
    }
}
std::string Bplus::Strings::ReplaceNew(const std::string& src,
                                       const std::string& snippet,
                                       const std::string& replaceWith)
{
    std::string newVal = src;
    Replace(newVal, snippet, replaceWith);
    return newVal;
}


bool Bplus::Strings::FindDifference(const std::string& a, const std::string b,
                                    size_t& outI, size_t& outCharI, size_t& outLineI)
{
    outCharI = 1;
    outLineI = 1;
    outI = 0;
    while (outI < b.size() && outI < a.size() && b[outI] == a[outI])
    {
        char c = b[outI];
        if (c == '\n' || c == '\r')
        {
            outLineI += 1;
            outCharI = 1;

            //If this is a two-character line break and not the failure point,
            //    pass over the second break character.
            if (outI + 1 < b.size() && outI + 1 < a.size() &&
                b[outI + 1] == a[outI + 1] &&
                ((b[outI] == '\n' && b[outI + 1] == '\r') ||
                    (b[outI] == '\r' && b[outI + 1] == '\n')))
            {
                outI += 1;
            }
        }
        else
        {
            outCharI += 1;
        }

        outI += 1;
    }

    return ((outI < b.size()) | (outI < a.size()));
}