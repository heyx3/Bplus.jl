#pragma once

#include "Platform.h"

//The BETTER_ENUM() macro, to define an enum
//    with added string conversions and iteration.
//There is one downside: to use a literal value, it must be prepended with a '+',
//    e.x. "if (mode == +VsyncModes::Off)".
#define BETTER_ENUMS_API BP_API
#include <better_enums.h>


//Custom assert macro that can be configured by users of this engine.
//Doesn't do anything in release builds.
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


#pragma region Bool struct, for making a sane vector<Bool>
//Mimics the standard bool type, except that it can be used in a vector<>
//    without becoming a bitfield.
struct BP_API Bool
{
    Bool() : data(false) { }
    Bool(bool b) : data(b) { }

    operator const bool&() const { return data; }
    operator       bool&()       { return data; }

    Bool& operator=(bool b) { data = b; return *this; }

    bool operator!() const { return !data; }
    bool operator|(bool b) const { return data | b; }
    bool operator&(bool b) const { return data & b; }

    bool operator==(bool b) const { return b == data; }
    bool operator!=(bool b) const { return b != data; }

private:
    bool data;
};

//Provide hashing for Bool.
namespace std
{
    template<> struct BP_API hash<Bool>
    {
        std::size_t operator()(Bool key) const
        {
            using std::hash;
            return hash<bool>()(key);
        }
    };
}

#pragma endregion

#pragma region strong_typedef

//A type-safe form of typedef, that wraps the underlying data into a struct.
//Based on: https://foonathan.net/2016/10/strong-typedefs/
#define strong_typedef_start(Tag, UnderlyingType, classAttrs) \
    struct classAttrs Tag : _strong_typedef<Tag, UnderlyingType> { \
        using Data_t = UnderlyingType; \
        using Me_t = Tag; \
        using _strong_typedef::_strong_typedef; /* Make the constructors available */
#define strong_typedef_end \
    }
#define strong_typedef(Tag, UnderlyingType, classAttrs) \
    strong_typedef_start(Tag, UnderlyingType, classAttrs) \
    strong_typedef_end

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

#pragma endregion

//This helper macro escapes commas inside other macro calls.
#define BP_COMMA ,

#pragma region Lazy<> struct, for lazy instantiation of a class
namespace Bplus
{
    //An object that isn't actually constructed until it is needed.
    template<typename T>
    struct Lazy
    {
    public:
        using Inner_t = T;


        bool IsCreated() const { return isCreated; }

        template<typename... Args>
        void Create(Args&&... args)
        {
            BPAssert(!isCreated, "Already created this!");
            isCreated = true;
            new (&itemBytes[0]) T(std::forward<Args>(args)...);
        }

        
        //Gets the underlying data, creating it with the default constructor if necessary.
        const T& Get() const
        {
            //If not created already, the instance should be created now.
            //However, this operation is supposed to be a const op that returns a const instance,
            //    so we can't avoid some const casting.
            if (!isCreated)
                const_cast<Lazy<T>*>(this)->Create();

            return Cast();
        }
        T& Get()
        {
            return const_cast<T&>(const_cast<const Lazy<T>*>(this)->Get());
        }

        //Gets the underlying data, assuming it's already been created.
        const T& Cast() const
        {
            BPAssert(isCreated, "Accessed before creation");
            return *(const T*)(&itemBytes[0]);
        }
        T& Cast()
        {
            return const_cast<T&>(const_cast<const Lazy<T>*>(this)->Cast());
        }

        //Makes this object un-instantiated again.
        //If it was never instantiated in the first place, nothing happens.
        void Clear()
        {
            if (isCreated)
            {
                isCreated = false;
                Cast().~T();
            }
        }
        

        #pragma region Constructor, destructor, copy, move

        Lazy() { }
        ~Lazy()
        {
            if (isCreated)
                Cast().~T();
            isCreated = false;
        }

        Lazy(const T& cpy) : isCreated(true) { Create(cpy); }
        Lazy(const Lazy<T>& cpy)
            : isCreated(cpy.isCreated)
        {
            if (cpy.isCreated)
                Create(cpy.Cast());
        }
        Lazy& operator=(const Lazy<T>& cpy)
        {
            //Destroy or copy the "cpy" object as necessary.
            if (isCreated && !cpy.isCreated)
                Cast().~T();
            else if (cpy.isCreated)
                if (isCreated)
                    Cast() = cpy.Cast();
                else
                    new(&Cast()) T(cpy.Cast());

            isCreated = cpy.isCreated;
            return *this;
        }
        Lazy& operator=(const T& cpy)
        {
            if (isCreated)
                Cast() = cpy;
            else
                new(&Cast()) T(cpy);

            isCreated = true;
            return *this;
        }
        
        Lazy(T&& src) : isCreated(true) { Create(std::move(src)); }
        Lazy(Lazy<T>&& src)
            : isCreated(src.isCreated)
        {
            if (src.isCreated)
                Create(std::move(src.Cast()));
        }
        Lazy& operator=(Lazy<T>&& src)
        {
            //Destroy or move the "src" object as necessary.
            if (isCreated && !src.isCreated)
                Cast().~T();
            else if (src.isCreated)
                if (isCreated)
                    Cast() = std::move(src.Cast());
                else
                    new(&Cast()) T(std::move(src.Cast()));

            isCreated = src.isCreated;
            return *this;
        }
        Lazy& operator=(T&& cpy)
        {
            if (isCreated)
                Cast() = std::move(cpy);
            else
                new(&Cast()) T(std::move(cpy));

            isCreated = true;
            return *this;
        }

        #pragma endregion


    private:
        mutable bool isCreated = false;
        uint8_t itemBytes[sizeof(T)];
    };
}
#pragma endregion


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
#pragma endregion