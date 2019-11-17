#pragma once

//A file that is included in Platform.h, essentially making this file's contents
//    available everywhere in the project.


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

namespace Bplus
{
    #pragma region _strong_typedef definition
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
    #pragma endregion
}