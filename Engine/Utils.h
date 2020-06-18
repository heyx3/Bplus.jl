#pragma once

#include "Platform.h"

#include <vector>

//TODO: The functions should probably be moved into the Bplus namespace.


//The BETTER_ENUM() macro, to define an enum
//    with added string conversions and iteration.
//There is one downside: to use a literal value, it must be prepended with a '+',
//    e.x. "if (mode == +VsyncModes::Off)".
//#define BETTER_ENUMS_API BP_API
//NOTE: The #define above was removed because it screws up usage from outside this library,
//    and I'm pretty sure it's not even needed in the first place.
//
//Before including the file, I'm adding a custom modification to enable
//    default constructors for these enum values:
#define BETTER_ENUMS_DEFAULT_CONSTRUCTOR(Enum) public: Enum() = default;
//
#include <better_enums.h>


//The name of the folder where all engine and app content goes.
//This folder is automatically copied to the output directory when building the engine.
#define BPLUS_CONTENT_FOLDER "content"

//The relative path to the engine's own content folder.
//Apps should not put their own stuff in this folder.
#define BPLUS_ENGINE_CONTENT_FOLDER (BPLUS_CONTENT_FOLDER "/engine")


//Not defined in the standard before C++20...
constexpr bool IsPlatformLittleEndian()
{
    //Reference: https://stackoverflow.com/a/1001328
    static_assert(sizeof(char) < sizeof(int),
                  "Only works if int is larger than char");

    int i = 1;
    char* iBytes = (char*)(&i);
    return (*iBytes) == 1;
}


//How does C++ not have a modern "itoa" already?
template<typename Int_t>
std::string ToStringInBase(Int_t value, int base,
                           const char* prefix = nullptr)
{
    char buffer[32];
    itoa(value, buffer, base);
    return buffer;
}


//Safe type-punning: reinterprets input A's byte-data as an instance of B.
template<typename A, typename B>
B Reinterpret(const A& a)
{
    static_assert(sizeof(A) == sizeof(B),
                  "Can't Reinterpret() these two types; they're not the same size");

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


//A modern C++17 way to hash any number of hashable types together.
//The types must specialize std::hash<T>().
template<typename... Items>
size_t MultiHash(const Items&... items)
{
    size_t seed = 0;
    MultiHash_(seed, items);
}

//Helper function for MultiHash().
template<typename T, typename... Rest>
void MultiHash_(std::size_t& seed, const T& v, const Rest&... rest)
{
    //Taken from: https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
    seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    (MultiHash(seed, rest), ...);
}


//TODO: StackTrace utility? Then add it to the default BPAssert function.

//Custom assert macro that can be configured by users of this engine.
//Doesn't do anything in release builds.
//TODO: Switch from plain C-style string to a std::string.
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

//Adds a "Null" value, given its actual value as an integer.
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