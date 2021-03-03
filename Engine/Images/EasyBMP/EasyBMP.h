/*************************************************
*                                                *
*  EasyBMP Cross-Platform Windows Bitmap Library * 
*                                                *
*  Author: Paul Macklin                          *
*   email: macklin01@users.sourceforge.net       *
* support: http://easybmp.sourceforge.net        *
*                                                *
*          file: EasyBMP.h                       * 
*    date added: 01-31-2005                      *
* date modified: 12-01-2006                      *
*       version: 1.06                            *
*                                                *
*   License: BSD (revised/modified)              *
* Copyright: 2005-6 by the EasyBMP Project       * 
*                                                *
* description: Main include file                 *
*                                                *
*************************************************/

//Modified for B-Plus for the following:
//   * Moved inside a namespace
//   * Support DLL export
//   * Use more unique macro names
//   * Support reading/writing to memory, not just a file
//   * Disable std::cout warnings by default

#define EASY_BMP_NAMESPACE ebmp

//Define this macro to change how functions are exported, e.x. "__declspec(dllexport)"
#ifndef EASY_BMP_API
#define EASY_BMP_API
#endif


#ifdef _MSC_VER 
// MS Visual Studio gives warnings when using 
// fopen. But fopen_s is not going to work well 
// with most compilers, and fopen_s uses different 
// syntax than fopen. (i.e., a macro won't work) 
// So, we'lll use this:
#define _CRT_SECURE_NO_DEPRECATE
#endif

#include <iostream>
#include <cmath>
#include <cctype>
#include <cstring>

#ifndef EasyBMP
#define EasyBMP

#ifdef __BCPLUSPLUS__ 
// The Borland compiler must use this because something
// is wrong with their cstdio file. 
#include <stdio.h>
#else 
#include <cstdio>
#endif

#ifdef __GNUC__
// If g++ specific code is ever required, this is 
// where it goes. 
#endif

#ifdef __INTEL_COMPILER
// If Intel specific code is ever required, this is 
// where it goes. 
#endif

#ifndef _EBMP_DefaultXPelsPerMeter_
#define _EBMP_DefaultXPelsPerMeter_
#define EBMP_DefaultXPelsPerMeter 3780
// set to a default of 96 dpi 
#endif

#ifndef _DefaultYPelsPerMeter_
#define _DefaultYPelsPerMeter_
#define DefaultYPelsPerMeter 3780
// set to a default of 96 dpi
#endif

#include "EasyBMP_DataStructures.h"
#include "EasyBMP_BMP.h"
#include "EasyBMP_VariousBMPutilities.h"

#ifndef _EasyBMP_Version_
#define _EasyBMP_Version_ 1.06
#define _EasyBMP_Version_Integer_ 106
#define _EasyBMP_Version_String_ "1.06"
#endif

#ifndef _EasyBMPwarnings_
#define _EasyBMPwarnings_
#endif

namespace EASY_BMP_NAMESPACE {
    EASY_BMP_API void SetEasyBMPwarningsOff( void );
    EASY_BMP_API void SetEasyBMPwarningsOn( void );
    EASY_BMP_API bool GetEasyBMPwarningState( void );
}

#endif
