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
        

        #pragma region constructor, destructor, move, copy

        Lazy() { }
        ~Lazy()
        {
            if (isCreated)
                Cast().~T();
            isCreated = false;
        }

        Lazy(const Lazy<T>& cpy)
            : isCreated(cpy.isCreated)
        {
            if (cpy.isCreated)
                Create(cpy.Cast());
        }
        Lazy(Lazy<T>&& src)
            : isCreated(src.isCreated)
        {
            if (src.isCreated)
                Create(std::move(src.Cast()));
        }

        Lazy(const T& cpy) : isCreated(true) { Create(cpy); }
        Lazy(T&& src)      : isCreated(true) { Create(std::move(src)); }

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