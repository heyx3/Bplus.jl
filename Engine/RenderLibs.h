#pragma once

//Os/platform defines.
#include "Platform.h"

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

//DevIL, the image loading/saving library.
#include <DevIL/il.h>
#include <DevIL/ilu.h>
#include <DevIL/ilut.h>

//TODO: Libraries for Block-Compression of textures (see below):
//        * https://github.com/richgel999/bc7enc16
//        * https://github.com/GameTechDev/ISPCTextureCompressor

//AssImp, the model file saving/loading library.
#include <assimp/scene.h>