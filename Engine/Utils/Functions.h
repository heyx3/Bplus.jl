#pragma once

#include "../Platform.h"

#include <string>


//Ad-hoc/uncategorized helper functions.

namespace Bplus
{
    //A helper type that does nothing but raise a lambda in the destructor.
    //Cannot be moved or copied.
    template<typename FuncOnStackClose>
    struct TieToStack
    {
        FuncOnStackClose Func;
        TieToStack(FuncOnStackClose func) : Func(func) { }
        ~TieToStack() { Func(); }

        TieToStack(const TieToStack<FuncOnStackClose>&) = delete;
        TieToStack(TieToStack<FuncOnStackClose>&&) = delete;
    };


    //Not defined in the standard before C++20...
    constexpr bool IsPlatformLittleEndian()
    {
        //Reference: https://stackoverflow.com/a/1001328

        uint32_t i = 1;
        unsigned char* iBytes = (unsigned char*)(&i);
        return iBytes[0] == 1;
    }


    //Safe type-punning: reinterprets input A's byte-data as an instance of B
    //    by making a copy on the stack.
    template<typename A, typename B>
    B Reinterpret(const A& a)
    {
        static_assert(sizeof(A) >= sizeof(B),
                      "Can't Reinterpret() these two types; destination is larger than source");

        B b;
        std::memcpy(&b, &a, sizeof(B));

        return b;
    }

    template<typename T>
    void SwapByteOrder(const T* src, std::byte* dest)
    {
        if constexpr (sizeof(T) == 1)
        {
            std::memcpy(dest, src, 1);
        }
        else if constexpr (sizeof(T) == 2)
        {
            auto asInt = Reinterpret<T, uint16_t>(*src);
            asInt = ((asInt << 8) & 0xff00) |
                    ((asInt >> 8) & 0x00ff);
            std::memcpy(dest, &asInt, 2);
        }
        else if constexpr (sizeof(T) == 4)
        {
            auto asInt = Reinterpret<T, uint32_t>(*src);
            asInt = (((asInt & 0x000000FF) << 24) |
                     ((asInt & 0x0000FF00) << 8)  |
                     ((asInt & 0x00FF0000) >> 8)  |
                     ((asInt & 0xFF000000) >> 24));
            std::memcpy(dest, &asInt, 4);
        }
        else
        {
            const std::byte* srcBytes = (const std::byte*)src;
            for (uint_fast32_t i = 0; i < sizeof(T); ++i)
                dest[i] = srcBytes[sizeof(T) - i - 1];
        }
    }


    template<typename T, size_t Size>
    std::array<T, Size> MakeArray(const T& fillValue)
    {
        std::array<T, Size> arr;
        arr.fill(fillValue);
        return arr;
    }


    //A helper function that turns the "glCreateX()" functions into a simple expression.
    template<typename GL_t>
    GL_t GlCreate(void (*glFunc)(int, GL_t*))
    {
        GL_t ptr;
        (*glFunc)(1, &ptr);
        return ptr;
    }
}