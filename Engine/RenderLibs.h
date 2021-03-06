#pragma once

//TODO: Rename to "Dependencies.h"

//Basic B+ platform/macro stuff:
#include "Platform.h"
#include "Utils/BPAssert.h"


//Dear ImGUI, an immediate-mode GUI library.
#include "Dear ImGui\ImGuiInterfaces.h"
#include "Dear ImGui\ImGuiAddons.h"

//SDL, for input and window management.
#define SDL_MAIN_HANDLED 1
#include <sdl\SDL.h>

//OpenGL.
#include <gl/glew.h>
#include <sdl/SDL_opengl.h>

//GLM, a popular math library for OpenGL.
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/matrix.hpp>


//EnTT, an Entity-Component System.
#define ENTT_ASSERT(condition) BP_ASSERT(condition, "[internal EnTT assert]")
#include <entt.hpp>
//To avoid compiler warnings, the use of EnTT "hashed strings" must go through the below macro.
#if !defined(COMPILER_VS)
    #define ENTT_SYMBOL(str) entt::hashed_string{str}
#else
    #define ENTT_SYMBOL(str) __pragma(warning(suppress:4307)) entt::hashed_string::value(str)
#endif


//Simple Iterator Template, which makes it much easier to construct custom STL iterators.
#include <iterator_tpl.h>

//zlib, a popular compression algorithm.
#define ZLIB_WINAPI
#define ZLIB_DLL
#include <zlib/zlib.h>

//Various image libraries.
#include <turboJPEG/jpeglib.h>
//TODO: Set up a custom error handler for TurboJPEG that uses BP_ASSERT
#define PNG_USE_DLL 1
#include <libPNG/png.h>

//TODO: Libraries for Block-Compression of textures (see below):
//        * https://github.com/richgel999/bc7enc16
//        * https://github.com/GameTechDev/ISPCTextureCompressor

//AssImp, the 3D model file saving/loading library.
#include <assimp/scene.h>