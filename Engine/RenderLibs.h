#pragma once

//TODO: Rename to "Dependencies.h"

//Basic B+ platform/macro stuff:
#include "Platform.h"
#include "Utils/BPAssert.h"


//Dear ImGUI.
#include "Dear ImGui\ImGuiInterfaces.h"
#include "Dear ImGui\ImGuiAddons.h"

//SDL.
#define SDL_MAIN_HANDLED 1
#include <sdl\SDL.h>

//OpenGL, GLU, GLEW
#include <gl/glew.h>
#include <sdl/SDL_opengl.h>

//GLM, the OpenGL math library.
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/matrix.hpp>


//EnTT, the Entity-Component System.
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


//Image libraries:
#include <turboJPEG/jpeglib.h>
//Custom error handler for TurboJPEG that uses BP_ASSERT:
namespace Bplus {
    TurboJPEGErrorHandler
}
#define PNG_USE_DLL 1
#include <libPNG/png.h>
//TODO: Replace EasyBMP with my own BMP library; this one has a loooot of issues
#define EASY_BMP_API BP_API
#include "Images/EasyBMP/EasyBMP.h"

//TODO: Libraries for Block-Compression of textures (see below):
//        * https://github.com/richgel999/bc7enc16
//        * https://github.com/GameTechDev/ISPCTextureCompressor

//AssImp, the model file saving/loading library.
#include <assimp/scene.h>