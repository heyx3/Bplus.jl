#pragma once

#include "../Platform.h"


#pragma region BP_DEBUG, BP_RELEASE, BPIsDebug

namespace Bplus
{
    //This boolean is true if in debug mode, and false if in release mode.
    constexpr inline bool BPIsDebug =
            #ifdef NDEBUG
                false
            #else
                true
            #endif
        ;
}
    
//BP_DEBUG passes through the input if in debug mode,
//    or replaces it with nothing in release mode.
#ifdef NDEBUG
    #define BP_DEBUG(...)
#else
    #define BP_DEBUG(...) __VA_ARGS__
#endif

//BP_RELEASE passes through the input if in release mode,
//    or replaces it with nothing in debug mode.
#ifdef NDEBUG
    #define BP_RELEASE(...) __VA_ARGS__
#else
    #define BP_RELEASE(...)
#endif

#pragma endregion


//This macro technically counts as an expression or statement, but does nothing.
//This can prevent compiler warnings about empty code blocks.
#define BP_NOOP (void)0

//Runs the assertion check.
#define BP_CHECK(expr, msg) Bplus::GetAssertFunc()(expr, msg)

//Runs the assertion check in debug builds, but not in release builds.
//Custom assert macro that can be configured by users of this engine.
//Doesn't do anything in release builds.
//TODO: Switch from plain C-style string to a std::string.
#define BP_ASSERT(expr, msg) \
    BP_RELEASE(BP_NOOP) \
    BP_DEBUG(BP_CHECK(expr, msg))

//A variant of BP_ASSERT that constructs a temporary std::string message.
#define BP_ASSERT_STR(expr, msg) { \
    if constexpr (Bplus::BPIsDebug) { \
        if (!(expr)) { \
            std::string msgStr; \
            msgStr += std::string() + msg; \
            BP_ASSERT(false, msgStr.c_str()); \
        } \
    } }

//Begins a block of assertion code that returns "std::nullopt" if everything went fine,
//    or a std::string error message if the assert failed.
//The code you write inside this block will be run within a lambda.
#define BP_ASSERT_BLOCK_BEGIN { \
    if constexpr (Bplus::BPIsDebug) { \
        auto checkRunner = [&]() -> std::optional<std::string> {
#define BP_ASSERT_BLOCK_END \
        }; \
        auto result = checkRunner(); \
        if (result.has_value()) \
            BP_ASSERT_STR(false, *result); \
    } }

//TODO: Add a BP_LOG.

namespace Bplus
{
    using AssertFuncSignature = void(*)(bool, const char*);
    void BP_API SetAssertFunc(AssertFuncSignature f);
    AssertFuncSignature BP_API GetAssertFunc();

    //Runs the standard "assert()" macro,
    //    and also writes to stdout if it fails.
    void BP_API DefaultAssertFunc(bool expr, const char* msg);
}