#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include <Dear ImGui\imgui.h>
#include <Dear ImGui\imgui_impl_sdl.h>
#include <Dear ImGui\imgui_impl_opengl3.h>

#define SDL_MAIN_HANDLED 1
#include <sdl\SDL.h>

#include <gl/glew.h>
#include <sdl/SDL_opengl.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <DevIL/il.h>
#include <DevIL/ilu.h>
#include <DevIL/ilut.h>

#include <assimp/scene.h>

#include "Renderer/Helpers.h"