#pragma once

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <functional>

#include <filesystem>
namespace fs = std::filesystem;

//Imported libraries:
#include "NativeFileDialog\nfd.h"

#include "Platform.h"


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
    }
}