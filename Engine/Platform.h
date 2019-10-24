#pragma once

//Defines either OS_WINDOWS, OS_UNIX, or OS_OTHER.
//Also includes various important OS headers.
//Finally, defines some macros for DLL export/import.


#if defined(_WIN32) || defined(WIN32)
    #define OS_WINDOWS
    #define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
    #define NOMINMAX                // Stop conflicts with "min" and "max" macro names
    #include <windows.h>

#elif defined(__unix__)
    #define OS_UNIX
    #pragma warning Include Unix OS files here.
#else
    #define OS_OTHER
    #pragma warning Unknown OS.
#endif


//Define some compiler-specific tokens, BP_COMPILER_EXPORT and BP_COMPILER_IMPORT.
//You shouldn't use these directly.
#if defined(_MSC_VER)
    //  Microsoft 
    #define BP_COMPILER_EXPORT __declspec(dllexport)
    #define BP_COMPILER_IMPORT __declspec(dllimport)
#elif defined(__GNUC__)
    //  GCC
    #define BP_COMPILER_EXPORT __attribute__((visibility("default")))
    #define BP_COMPILER_IMPORT
#else
    //  do nothing and hope for the best?
    #define BP_COMPILER_EXPORT
    #define BP_COMPILER_IMPORT
    #pragma warning Unknown dynamic link import/export semantics.
#endif

//BP_API: Goes before any class/function/global var that should be visible to users of the DLL.
//BP_C_API: Same as BP_API, but also contains 'extern "C"' so it will be exported as plain C.
//BP_IMPORT: Defined as "extern" if importing this as a DLL, or nothing otherwise.
//Note that you should define BP_STATIC if building as a static library.
#if BP_STATIC
	#define BP_API
    #define BP_IMPORT
#elif BP_EXPORT_DLL
	#define BP_API BP_COMPILER_EXPORT
    #define BP_IMPORT
#else
	#define BP_API BP_COMPILER_IMPORT
    #define BP_IMPORT extern
#endif

#define BP_C_API extern "C" BP_API