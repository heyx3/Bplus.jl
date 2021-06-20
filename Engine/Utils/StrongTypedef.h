#pragma once

#include "../Platform.h"


//A type-safe form of typedef, which wraps the underlying data into a struct.
//Based on: https://foonathan.net/2016/10/strong-typedefs/


#define strong_typedef_start(Tag, UnderlyingType, classAttrs) \
    struct classAttrs Tag : Bplus::_strong_typedef<Tag, UnderlyingType> \
    { \
    public: \
        using Data_t = UnderlyingType; \
        using Me_t = Tag; \
        /* Constructors from a value: */ \
        explicit Tag(const Data_t& value) \
            : Bplus::_strong_typedef<Tag, UnderlyingType>(value) { } \
        explicit Tag(Data_t&& value) \
            : Bplus::_strong_typedef<Tag, UnderlyingType>(std::move(value)) { } \
        /* Constructors from another instance: */ \
        Tag(const Me_t&  value) : Bplus::_strong_typedef<Tag, UnderlyingType>(value.Get()) { } \
        Tag(      Me_t&& value) : Bplus::_strong_typedef<Tag, UnderlyingType>(std::move(value.Get())) { } \
        /* Define assignment operators */ \
        Me_t& operator=(const Me_t&    t) { new (this) Me_t(t.Get());            return *this; } \
        Me_t& operator=(      Me_t&&   t) { new (this) Me_t(std::move(t.Get())); return *this; } \
        Me_t& operator=(const Data_t&  t) { new (this) Me_t(t);                  return *this; } \
        Me_t& operator=(      Data_t&& t) { new (this) Me_t(std::move(t));       return *this; }


#define strong_typedef_end \
    };


//Defines '==' and '!=' operators for the type,
//    assuming the underlying type has them too.
//NOTE that this MUST be placed between 'strong_typedef_start' and 'strong_typedef_end'!
#define strong_typedef_equatable \
    bool operator==(const Me_t& t) const { return (Data_t)t == (Data_t)(*this); } \
    bool operator!=(const Me_t& t) const { return (Data_t)t != (Data_t)(*this); } \
    bool operator==(const Data_t& t) const { return t == (Data_t)(*this); } \
    bool operator!=(const Data_t& t) const { return t != (Data_t)(*this); }


//Adds a default constructor initializing to the given value.
//You should not add this if you've already added the "null" value.
//NOTE that this MUST be placed between 'strong_typedef_start' and 'strong_typedef_end'!
#define strong_typedef_defaultConstructor(Tag, defaultVal) \
    Tag() : Tag(defaultVal) { }


//Adds a "Null" value, given its actual integer value.
//Also adds a default constructor which uses that value.
//NOTE that this MUST be placed between 'strong_typedef_start' and 'strong_typedef_end'!
#define strong_typedef_null(Tag, intValue) \
    static const Data_t null = intValue; \
    static const Me_t Null() { return Me_t{null}; } \
    bool IsNull() const { return Get() == null; } \
    strong_typedef_defaultConstructor(Tag, null)


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
    public:

        _strong_typedef(const T& val) : value_(val) { }
        _strong_typedef(T&& val) : value_(std::move(val)) { }

        //Explicit cast to the underlying data:
        explicit operator T&() noexcept { return value_; }
        explicit operator const T&() const noexcept { return value_; }
        
        const T& Get() const { return value_; }
        T& Get() { return value_; }

        friend void swap(_strong_typedef& a, _strong_typedef& b) noexcept
        {
            using std::swap;
            swap(static_cast<T&>(a), static_cast<T&>(b));
        }

    protected:
        T value_;
    };
}