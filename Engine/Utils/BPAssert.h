#pragma once

#include "../Platform.h"


#pragma region BPDebug, BPRelease, BPIsDebug

//This boolean is true if in debug mode, and false if in release mode.
constexpr inline bool BPIsDebug =
        #ifdef NDEBUG
            false
        #else
            true
        #endif
    ;
    
//BPDebug passes through the input if in debug mode,
//    or replaces it with nothing in release mode.
#ifdef NDEBUG
    #define BPDebug(...)
#else
    #define BPDebug(...) __VA_ARGS__
#endif

//BPRelease passes through the input if in release mode,
//    or replaces it with nothing in debug mode.
#ifdef NDEBUG
    #define BPRelease(...) __VA_ARGS__
#else
    #define BPRelease(...)
#endif

#pragma endregion


//Custom assert macro that can be configured by users of this engine.
//Doesn't do anything in release builds.
//TODO: Switch from plain C-style string to a std::string.
#define BPAssert(expr, msg) \
    BPRelease((void)0) \
    BPDebug(Bplus::GetAssertFunc()(expr, msg))

namespace Bplus
{
    using AssertFuncSignature = void(*)(bool, const char*);
    void BP_API SetAssertFunc(AssertFuncSignature f);
    AssertFuncSignature BP_API GetAssertFunc();

    //Runs the standard "assert()" macro,
    //    and also writes to stdout if it fails.
    void BP_API DefaultAssertFunc(bool expr, const char* msg);
}