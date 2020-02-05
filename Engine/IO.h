#pragma once

//Provides string and IO helpler functions in the Bplus::IO namespace.
//Also defines a standard exception type, Bplus::IO::Exception.

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <functional>

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
        //Makes it easy to "nest" these exceptions, so each level can attach information
        //    about where that part of the parser went wrong.
        class Exception
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

        std::string BP_API ReadEntireFile(const fs::path& path,
                                          const std::string& defaultIfMissing);

        void BP_API ToLowercase(std::string& str);
        std::string BP_API ToLowercase(const char* str);

        void BP_API Remove(std::string& str, char c);
        void BP_API RemoveAll(std::string& str, const char* charsToRemove, size_t nChars);

        
        //BetterEnum_t should be an enum type created with the BETTER_ENUM() macro.
        template <typename BetterEnum_t>
        //Case-insensitive and space/underscore-insensitive parsing of enums
        //    that were defined with the BETTER_ENUM macro.
        //If the enum requires case-sensitivity or underscore-sensitivity
        //    to distinguish between its elements,
        //    this function is smart enough to notice and provide that.
        BetterEnum_t BP_API EnumFromString(const std::string& _str)
        {
            //For each type of enum, we need to know if it is case-insensitive
            //    and underscore-insensitive.
            static std::optional<bool> preserveCase, preserveUnderscores;
            static std::unordered_map<std::string, BetterEnum_t> agnosticLookup;
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
                    Bplus::Strings::ToLowercase(eStr_Case);
                    Bplus::Strings::ToLowercase(eStr_All);
                    //See if it's unique among the enum strings.
                    if (values_case.find(eStr_Case) == values_case.end())
                        values_case.insert(eStr_Case);
                    else
                        preserveCase = true;

                    //Remove underscores.
                    Bplus::Strings::Remove(eStr_Underscores, '_');
                    Bplus::Strings::Remove(eStr_All, '_');
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

                #pragma region Generate 'agnosticLookup' values

                BPAssert(agnosticLookup.size() == 0, "Uninitialized but it has values??");

                std::string eStr;
                for (size_t i = 0; i < BetterEnum_t::_size(); ++i)
                {
                    auto eVal = BetterEnum_t::_from_index(i);
                    eStr = eVal._to_string();

                    if (!preserveCase.value())
                        Strings::ToLowercase(eStr);
                    if (!preserveUnderscores.value())
                        Strings::Remove(eStr, '_');

                    agnosticLookup[eStr] = eVal;
                }

                #pragma endregion
            }

            //Apply any allowed filters to the string.
            //Keep the original for generating an error message.
            auto str = _str;
            if (!preserveCase.value())
                Strings::ToLowercase(str);
            if (!preserveUnderscores.value())
                Strings::Remove(str, '_');
            Strings::Remove(str, ' ');

            //Look it up in the cached dictionary.
            auto found = agnosticLookup.find(str);
            if (found == agnosticLookup.end())
            {
                throw IO::Exception(std::string("Couldn't parse '") + _str +
                                      "' into a " + BetterEnum_t::_name());
            }
            else
            {
                return found->second;
            }
        }
    }
}