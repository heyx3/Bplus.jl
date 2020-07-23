#pragma once

//Provides string and IO helpler functions in the Bplus::IO namespace.
//Also defines a standard exception type, Bplus::IO::Exception.

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <functional>
#include <unordered_set>

#include <filesystem>
namespace fs = std::filesystem;

#include "Utils.h"


namespace Bplus
{
    //TODO: Refactor into a more robust logging system.
    using ErrorCallback = std::function<void(const std::string& msg)>;

    namespace IO
    {
        //The exception for something that goes bad during IO work, such as parsing.
        class BP_API Exception
        {
        public:
            const std::string Message;
            Exception(const char* msg) : Message(msg) { }
            Exception(const std::string& msg) : Message(msg) { }
            Exception(const Exception& inner,
                      const std::string& prefix, const std::string& suffix = "")
                : Exception(prefix + inner.Message + suffix) { }
        };


        //Returns whether it was successful.
        bool BP_API WriteEntireFile(const fs::path& path, const std::string& contents, bool append);

        //Reads the contents of the given text file
        //    and writes them into the given output string,
        //    returning whether or not it was successful.
        bool BP_API LoadEntireFile(const fs::path& path, std::string& output);
        //Reads the contents of the given text file and returns them as a string,
        //    or returns the given fallback if the file couldn't be read.
        std::string BP_API ReadEntireFile(const fs::path& path,
                                          const std::string& defaultIfMissing);

        void BP_API ToLowercase(std::string& str);
        std::string BP_API ToLowercase(const char* str);

        template<typename Int_t>
        std::string ToHex(Int_t i)
        {
            std::stringstream ss;
            ss << "0x" << std::hex << i;
            return ss.str();
        }

        void BP_API Remove(std::string& str, char c);
        void BP_API RemoveAll(std::string& str, const char* charsToRemove, size_t nChars);

        
        //BetterEnum_t should be an enum type created with the BETTER_ENUM() macro.
        template <typename BetterEnum_t>
        //Case-insensitive and space/underscore-insensitive parsing of enums
        //    that were defined with the BETTER_ENUM macro.
        //If the enum requires case-sensitivity or underscore-sensitivity
        //    to distinguish between its elements,
        //    this function is smart enough to notice and provide that.
        BetterEnum_t EnumFromString(const std::string& _str)
        {
            //For each type of enum, we need to know if it is case-insensitive
            //    and underscore-insensitive.
            //NOTE: I'm getting compile errors when I use a BETTER_ENUM enum
            //    as the values in an unordered_map, so I'm going to store them by index instead.
            static std::optional<bool> preserveCase, preserveUnderscores;
            static std::unordered_map<std::string, size_t> agnosticLookup;
            if (!preserveCase.has_value())
            {
                BPAssert(!preserveUnderscores.has_value(), "One but not the other??");

                preserveCase = false;
                preserveUnderscores = false;
            
                #pragma region Determine what needs to be preserved

                std::unordered_set<std::string> values_case, values_underscores,
                                                values_all;
                std::string eStr;
                for (size_t i = 0; i < BetterEnum_t::_size(); ++i)
                {
                    auto eVal = BetterEnum_t::_from_index(i);
                    eStr = eVal._to_string();

                    auto eStr_Case = eStr,
                         eStr_Underscores = eStr,
                         eStr_All = eStr;

                    //Make lowercase.
                    Bplus::IO::ToLowercase(eStr_Case);
                    Bplus::IO::ToLowercase(eStr_All);
                    //See if it's unique among the enum strings.
                    if (values_case.find(eStr_Case) == values_case.end())
                        values_case.insert(eStr_Case);
                    else
                        preserveCase = true;

                    //Remove underscores.
                    Bplus::IO::Remove(eStr_Underscores, '_');
                    Bplus::IO::Remove(eStr_All, '_');
                    //See if it's unique among the enum  strings.
                    if (values_underscores.find(eStr_Underscores) == values_underscores.end())
                        values_underscores.insert(eStr_Underscores);
                    else
                        preserveUnderscores = true;

                    //Finally, try the combined string.
                    if (values_all.find(eStr_All) == values_all.end())
                        values_all.insert(eStr_All);
                    else
                    {
                        //Pick a feature to give up.
                        //Symbol agnostic isn't as important as case-agnostic.
                        if (preserveCase.value() && preserveUnderscores.value())
                            preserveUnderscores = false;
                    }
                }

                #pragma endregion

                #pragma region Generate agnosticLookup

                BPAssert(agnosticLookup.size() == 0, "Uninitialized but it has values??");
                for (size_t i = 0; i < BetterEnum_t::_size(); ++i)
                {
                    auto eVal = BetterEnum_t::_from_index(i);
                    eStr = eVal._to_string();

                    if (!preserveCase.value())
                        IO::ToLowercase(eStr);
                    if (!preserveUnderscores.value())
                        IO::Remove(eStr, '_');

                    agnosticLookup[eStr] = i;
                }

                #pragma endregion
            }

            //Apply any allowed filters to the string.
            //Keep the original for generating an error message.
            auto str = _str;
            if (!preserveCase.value())
                IO::ToLowercase(str);
            if (!preserveUnderscores.value())
                IO::Remove(str, '_');
            IO::Remove(str, ' ');

            //Look it up in the cached dictionary.
            auto found = agnosticLookup.find(str);
            if (found == agnosticLookup.end())
            {
                throw IO::Exception(std::string("Couldn't parse '") + _str +
                                      "' into a " + BetterEnum_t::_name());
            }
            else
            {
                return BetterEnum_t::_from_index(found->second);
            }
        }
    }
}