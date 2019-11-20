#pragma once

#include "Platform.h"
#include <functional>

//Custom assert macro that can be configured by users of this engine.
//Only used in debug builds.
#pragma region BPAssert

#ifdef NDEBUG
    #define BPAssert(expr, msg) ((void)0)
#else
    #define BPAssert(expr, msg) Bplus::GetAssertFunc()(expr, msg)
#endif

namespace Bplus
{
    //The function used for asserts.
    using AssertFuncSignature = void(*)(bool, const char*);
    void BP_API SetAssertFunc(AssertFuncSignature f);
    AssertFuncSignature BP_API GetAssertFunc();

    //Runs the standard "assert()" macro,
    //    and also writes to stdout if it fails.
    void BP_API DefaultAssertFunc(bool expr, const char* msg);
}

#pragma endregion


//The BETTER_ENUM() macro, to define an enum
//    with added string conversions and iteration.
#define BETTER_ENUMS_API BP_API
#include <better_enums.h>


//A type-safe form of typedef, that wraps the underlying data into a struct.
//Based on: https://foonathan.net/2016/10/strong-typedefs/
#define strong_typedef(Tag, UnderlyingType, classAttrs) \
    struct classAttrs Tag : _strong_typedef<Tag, UnderlyingType> { }

//This helper macro escapes commas inside other macro calls.
#define BP_COMMA ,


#pragma region _strong_typedef helper struct
namespace Bplus
{
    template <class Tag, typename T>
    struct _strong_typedef
    {
        _strong_typedef() = default; //Normally deleted thanks to the below constructors

        explicit _strong_typedef(const T& value) : value_(value) { }
        explicit _strong_typedef(T&& value)
          noexcept(std::is_nothrow_move_constructible<T>::value)
            : value_(std::move(value)) { }

        explicit operator T&() noexcept { return value_; }
        explicit operator const T&() const noexcept { return value_; }

        friend void swap(_strong_typedef& a, _strong_typedef& b) noexcept
        {
            using std::swap;
            swap(static_cast<T&>(a), static_cast<T&>(b));
        }

    private:
        T value_;
    };
}
#pragma endregion