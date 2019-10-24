#pragma once

#include "Platform.h"

//Dear ImGUI : the GUI library.
#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include <Dear ImGui\imgui.h>
#include <Dear ImGui\imgui_impl_sdl.h>
#include <Dear ImGui\imgui_impl_opengl3.h>

//SDL.
#define SDL_MAIN_HANDLED 1
#include <sdl\SDL.h>

//OpenGL, GLU, GLEW
#include <gl/glew.h>
#include <sdl/SDL_opengl.h>

//GLM, the GL math library.
#include <glm/glm.hpp>
#include <glm/ext.hpp>

//DevIL, the image loading/saving library.
#include <DevIL/il.h>
#include <DevIL/ilu.h>
#include <DevIL/ilut.h>

//AssImp, the model file saving/loading library.
#include <assimp/scene.h>

//B+ rendering helper functions.
#include "Renderer/Helpers.h"