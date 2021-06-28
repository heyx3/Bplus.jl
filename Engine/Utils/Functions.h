#pragma once

#include "../Platform.h"

#include <string>
#include <vector>


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
    //TODO: List the return value type first for convenience.
    template<typename A, typename B>
    constexpr B Reinterpret(const A& a)
    {
        static_assert(sizeof(A) >= sizeof(B),
                      "Can't Reinterpret() these two types; destination is larger than source");

        B b;
        std::memcpy(&b, &a, sizeof(B));

        return b;
    }

    //Swaps the bytes in some data between little-endian and big-endian.
    //TODO: Take "src" as a reference instead of a pointer.
    template<typename T>
    void SwapByteOrder(const T* src, std::byte* dest)
    {
        //For small data, we can do this with simple bitwise operations.
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
        //For any other byte-size data, fall back to a loop.
        else
        {
            const std::byte* srcBytes = (const std::byte*)src;
            for (uint_fast32_t i = 0; i < sizeof(T); ++i)
                dest[i] = srcBytes[sizeof(T) - i - 1];
        }
    }


    //Makes a static array, filled with the given value.
    template<typename T, size_t Size>
    std::array<T, Size> MakeArray(const T& fillValue)
    {
        std::array<T, Size> arr;
        arr.fill(fillValue);
        return arr;
    }

    //Takes a group of iterators (i.e. things with "begin()" and "end()" methods)
    //    and combines them together into one vector.
    template<typename T, typename ... Inputs>
    std::vector<T> Concatenate(const Inputs& ... inputs)
    {
        std::vector<T> output;
        (output.insert(output.end(), inputs.begin(), inputs.end()), ...);
        return output;
    }

    //TODO: "Enumerate()", a decorator for iterators, that pairs each element with its index.

    // "overload", a C++17 trick that makes it easier to work with std::variant and std::visit.
    //Example usage:
    /*
        std::variant<...> myVar = ...;
        std::visit(overload(
            [](int i) { cout << "Value is an int: " << i; },
            [](std::string) { cout << "Value is some string"; },
            [](auto&&) { cout << "value is something else"; }
        ), myVar);
    */
    #pragma region overload

    //Source: https://arne-mertz.de/2018/05/overload-build-a-variant-visitor-on-the-fly/

    template <class ...Fs>
    struct overload : Fs... {
        template <class ...Ts>
        overload(Ts&& ...ts) : Fs{ std::forward<Ts>(ts) }...
        {}

        using Fs::operator()...;
    };

    template <class ...Ts>
    overload(Ts&&...)->overload<std::remove_reference_t<Ts>...>;

    #pragma endregion


    //Allows you to invoke the "glCreate[X]()" functions as a simple expression,
    //    instead of having to create a variable to be written into.
    template<typename GL_t>
    GL_t GlCreate(void (*glFunc)(int, GL_t*))
    {
        GL_t ptr;
        (*glFunc)(1, &ptr);
        return ptr;
    }
}