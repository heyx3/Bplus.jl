#pragma once

//Os/platform defines.
#include "Platform.h"

//Dear ImGUI.
#include "Dear ImGui\ImGuiInterfaces.h"

//SDL.
#define SDL_MAIN_HANDLED 1
#include <sdl\SDL.h>

//OpenGL, GLU, GLEW
#include <gl/glew.h>
#include <sdl/SDL_opengl.h>

//GLM, the OpenGL math library.
#include <glm/glm.hpp>
#include <glm/ext.hpp>

//DevIL, the image loading/saving library.
#include <DevIL/il.h>
#include <DevIL/ilu.h>
#include <DevIL/ilut.h>

//AssImp, the model file saving/loading library.
#include <assimp/scene.h>