#pragma once

#include "../Platform.h"

#include <istream>
#include <ostream>


//Defines new types of istream and ostream,
//    such as ones that work with existing memory.

namespace Bplus::IO
{
    //Used by InputMemoryStream.
    struct InputMemoryBuffer : std::streambuf
    {
        InputMemoryBuffer(const std::byte* data, size_t size)
        {
            //Apparently I have to do a const-cast here?
            //Why does the standard library take non-const pointers to read-only data?
            auto ptr = (char*)data;
            setg(ptr, ptr, ptr + size);
        }
    };
    //An std::istream that reads its data from memory.
    struct InputMemoryStream : virtual InputMemoryBuffer, std::istream
    {
        InputMemoryStream(const std::byte* data, size_t size)
            : InputMemoryBuffer(data, size),
              std::istream(static_cast<std::streambuf*>(this))
        {
        }
    };

    //Used by OutputMemoryStream.
    struct OutputMemoryBuffer : std::streambuf
    {
        OutputMemoryBuffer(std::byte* data, size_t size)
        {
            auto ptr = (char*)data;
            setp(ptr, ptr + size);
        }
    };
    //An std::ostream that writes data to memory.
    struct OutputMemoryStream : virtual OutputMemoryBuffer, std::ostream
    {
        OutputMemoryStream(std::byte* data, size_t size)
            : OutputMemoryBuffer(data, size),
            std::ostream(static_cast<std::streambuf*>(this))
        {
        }
    };
}