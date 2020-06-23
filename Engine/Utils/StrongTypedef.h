#pragma once

#include "../Platform.h"


//A type-safe form of typedef, that wraps the underlying data into a struct.
//Based on: https://foonathan.net/2016/10/strong-typedefs/

#define strong_typedef_start(Tag, UnderlyingType, classAttrs) \
    struct classAttrs Tag : Bplus::_strong_typedef<Tag, UnderlyingType> { \
        using Data_t = UnderlyingType; \
        using Me_t = Tag; \
        using _strong_typedef::_strong_typedef; /* Make the constructors available */

#define strong_typedef_end \
    };


//Defines '==' and '!=' operators for the type,
//    assuming the underlying type has them too.
//NOTE that this MUST be placed between 'strong_typedef_start' and 'strong_typedef_end'!
#define strong_typedef_equatable() \
    bool operator==(const Me_t& t) const { return (Data_t)t == (Data_t)(*this); } \
    bool operator!=(const Me_t& t) const { return (Data_t)t != (Data_t)(*this); } \
    bool operator==(const Data_t& t) const { return t == (Data_t)(*this); } \
    bool operator!=(const Data_t& t) const { return t != (Data_t)(*this); }


//Adds a default constructor initializing to the given value.
//NOTE that this MUST be placed between 'strong_typedef_start' and 'strong_typedef_end'!
#define strong_typedef_defaultConstructor(Tag, defaultVal) \
    Tag() : Tag(defaultVal) { }


//Adds a "Null" value, given its actual integer value.
//NOTE that this MUST be placed between 'strong_typedef_start' and 'strong_typedef_end'!
#define strong_typedef_null(intValue) \
    static const Data_t null = intValue; \
    static const Me_t Null() { return Me_t{null}; } \
    bool IsNull() const { return Get() == null; }


//Defines a default hash implementation for the type,
//    assuming the underlying type has a default hash implementation.
//NOTE that this MUST be placed in the global namespace!
#define strong_typedef_hashable(Tag, classAttrs) \
    namespace std { template<> struct classAttrs hash<Tag> { \
        std::size_t operator()(const Tag& key) const \
        { \
            using std::hash; \
            return hash<Tag::Data_t>()((Tag::Data_t)key); \
        } }; }


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
        
        const T& Get() const { return value_; }
        T& Get() { return value_; }

        friend void swap(_strong_typedef& a, _strong_typedef& b) noexcept
        {
            using std::swap;
            swap(static_cast<T&>(a), static_cast<T&>(b));
        }

    private:
        T value_;
    };
}