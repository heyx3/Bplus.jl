#pragma once

#include "../Platform.h"

#include <istream>
#include <ostream>


//Defines new types of istream and ostream,
//    such as ones that work with existing memory.

namespace Bplus
{
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
    struct InputMemoryStream : virtual InputMemoryBuffer, std::istream
    {
        InputMemoryStream(const std::byte* data, size_t size)
            : InputMemoryBuffer(data, size),
              std::istream(static_cast<std::streambuf*>(this))
        {
        }
    };


    struct OutputMemoryBuffer : std::streambuf
    {
        OutputMemoryBuffer(std::byte* data, size_t size)
        {
            auto ptr = (char*)data;
            setp(ptr, ptr + size);
        }
    };
    struct OutputMemoryStream : virtual OutputMemoryBuffer, std::ostream
    {
        OutputMemoryStream(std::byte* data, size_t size)
            : OutputMemoryBuffer(data, size),
            std::ostream(static_cast<std::streambuf*>(this))
        {
        }
    };
}