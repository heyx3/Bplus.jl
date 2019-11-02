#include "ImGuiInterfaces.h"
#include "../Renderer/Context.h"

#include <vector>


//This file is a custom rewrite of Dear ImGUI's built-in example SDL and OpenGL3 integrations.

#define SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE    SDL_VERSION_ATLEAST(2,0,4)

using namespace Bplus;

using SDLImpl = ImGuiSDLInterface_Default;
using OGLImpl = ImGuiOpenGLInterface_Default;

#pragma region Singleton management/Interface constructors

namespace
{
    thread_local ImGuiSDLInterface* currentSDLInterface = nullptr;
    thread_local ImGuiOpenGLInterface* currentOGLInterface = nullptr;
}

ImGuiSDLInterface* ImGuiSDLInterface::GetThreadInstance()
{
    return currentSDLInterface;
}
ImGuiSDLInterface::ImGuiSDLInterface(SDL_Window* _mainWindow, SDL_GLContext _glContext)
    : mainWindow(_mainWindow), glContext(_glContext)
{
    assert(currentSDLInterface == nullptr);
    currentSDLInterface = this;
}
ImGuiSDLInterface::~ImGuiSDLInterface()
{
    assert(currentSDLInterface == this);
    currentSDLInterface = nullptr;
}

ImGuiOpenGLInterface* ImGuiOpenGLInterface::GetThreadInstance()
{
    return currentOGLInterface;
}
ImGuiOpenGLInterface::ImGuiOpenGLInterface(const char* _glslVersion)
    : glslVersion(_glslVersion)
{
    assert(currentOGLInterface == nullptr);
    currentOGLInterface = this;
}
ImGuiOpenGLInterface::~ImGuiOpenGLInterface()
{
    assert(currentOGLInterface == this);
    currentOGLInterface = nullptr;
}

#pragma endregion

#pragma region Default SDL Interface

SDLImpl::ImGuiSDLInterface_Default(SDL_Window* mainWindow, SDL_GLContext glContext)
    : ImGuiSDLInterface(mainWindow, glContext)
{
    //Setup back-end capabilities flags
    ImGuiIO& io = ImGui::GetIO();
    //We can provide mouse cursor data and the ability to set the mouse position.
    //Note that some other backend flags are managed elsewhere --
    //    during updates, or in the renderer interface.
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
    io.BackendPlatformName = "bplus_sdl";

    //Set keyboard mappings to match SDL.
    io.KeyMap[ImGuiKey_Tab] = SDL_SCANCODE_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
    io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
    io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
    io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
    io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
    io.KeyMap[ImGuiKey_Insert] = SDL_SCANCODE_INSERT;
    io.KeyMap[ImGuiKey_Delete] = SDL_SCANCODE_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = SDL_SCANCODE_BACKSPACE;
    io.KeyMap[ImGuiKey_Space] = SDL_SCANCODE_SPACE;
    io.KeyMap[ImGuiKey_Enter] = SDL_SCANCODE_RETURN;
    io.KeyMap[ImGuiKey_Escape] = SDL_SCANCODE_ESCAPE;
    io.KeyMap[ImGuiKey_KeyPadEnter] = SDL_SCANCODE_RETURN2;
    io.KeyMap[ImGuiKey_A] = SDL_SCANCODE_A;
    io.KeyMap[ImGuiKey_C] = SDL_SCANCODE_C;
    io.KeyMap[ImGuiKey_V] = SDL_SCANCODE_V;
    io.KeyMap[ImGuiKey_X] = SDL_SCANCODE_X;
    io.KeyMap[ImGuiKey_Y] = SDL_SCANCODE_Y;
    io.KeyMap[ImGuiKey_Z] = SDL_SCANCODE_Z;

    //Set up the clipboard.
    io.ClipboardUserData = this;
    io.GetClipboardTextFn = [](void* i) { return ((SDLImpl*)i)->GetSDLClipboardText(); };
    io.SetClipboardTextFn = [](void* i, const char* text) { ((SDLImpl*)i)->SetSDLClipboardText(text); };

    //Set SDL mouse cursors to system defaults.
    mouseCursors[ImGuiMouseCursor_Arrow] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    mouseCursors[ImGuiMouseCursor_TextInput] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
    mouseCursors[ImGuiMouseCursor_ResizeAll] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
    mouseCursors[ImGuiMouseCursor_ResizeNS] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
    mouseCursors[ImGuiMouseCursor_ResizeEW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
    mouseCursors[ImGuiMouseCursor_ResizeNESW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
    mouseCursors[ImGuiMouseCursor_ResizeNWSE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
    mouseCursors[ImGuiMouseCursor_Hand] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
}
SDLImpl::~ImGuiSDLInterface_Default()
{
    //Release this instance's management of the ImGUI clipboard.
    ImGuiIO& io = ImGui::GetIO();
    io.ClipboardUserData = nullptr;
    io.GetClipboardTextFn = nullptr;
    io.SetClipboardTextFn = nullptr;

    //Clean up SDL mouse cursors.
    for (auto& cursor : mouseCursors)
        SDL_FreeCursor(cursor);
}

const char* SDLImpl::GetSDLClipboardText()
{
    char* rawClipboard = SDL_GetClipboardText();
    clipboard = rawClipboard;
    SDL_free(rawClipboard);

    return clipboard.c_str();
}
void SDLImpl::SetSDLClipboardText(const char* text)
{
    clipboard = text;
    SDL_SetClipboardText(text);
}

void SDLImpl::ProcessEvent(const SDL_Event& event)
{
    ImGuiIO& io = ImGui::GetIO();
    switch (event.type)
    {
        case SDL_MOUSEWHEEL: {
            if (event.wheel.x > 0)
                io.MouseWheelH += 1;
            if (event.wheel.x < 0)
                io.MouseWheelH -= 1;
            if (event.wheel.y > 0)
                io.MouseWheel += 1;
            if (event.wheel.y < 0)
                io.MouseWheel -= 1;
        } break;

        case SDL_MOUSEBUTTONDOWN: {
            if (event.button.button == SDL_BUTTON_LEFT)
                mousePressed[0] = true;
            if (event.button.button == SDL_BUTTON_RIGHT)
                mousePressed[1] = true;
            if (event.button.button == SDL_BUTTON_MIDDLE)
                mousePressed[2] = true;
        } break;

        case SDL_TEXTINPUT: {
            io.AddInputCharactersUTF8(event.text.text);
        } break;

        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            int key = event.key.keysym.scancode;
            IM_ASSERT(key >= 0 && key < IM_ARRAYSIZE(io.KeysDown));
            io.KeysDown[key] = (event.type == SDL_KEYDOWN);
            io.KeyShift = ((SDL_GetModState() & KMOD_SHIFT) != 0);
            io.KeyCtrl = ((SDL_GetModState() & KMOD_CTRL) != 0);
            io.KeyAlt = ((SDL_GetModState() & KMOD_ALT) != 0);
            io.KeySuper = ((SDL_GetModState() & KMOD_GUI) != 0);
        } break;

        default: break;
    }
}

std::array<bool, 3> SDLImpl::RefreshMouseData(glm::ivec2& outPos)
{
    auto sdlMouseButtonFlags = SDL_GetMouseState(&outPos.y, &outPos.x);

    std::array<bool, 3> output;
    // If a mouse press event came, always pass it as "mouse held this frame",
    //    so we don't miss click-release events that are shorter than 1 frame.
    #define IMPL_BUTTON(imGuiIndex, sdlIndex) \
        output[imGuiIndex] = mousePressed[imGuiIndex] || \
                             ((sdlMouseButtonFlags & SDL_BUTTON(sdlIndex))) != 0
    IMPL_BUTTON(0, SDL_BUTTON_LEFT);
    IMPL_BUTTON(1, SDL_BUTTON_RIGHT);
    IMPL_BUTTON(2, SDL_BUTTON_MIDDLE);
    #undef IMPL_BUTTON

    //Now that we're reporting the mouse buttons,
    //    reset the field that tracks button presses over a frame.
    mousePressed.fill(false);
    return output;
}
void SDLImpl::GetWindowDisplayScale(glm::ivec2& windowSize,
                                    glm::fvec2& windowFramebufferScale)
{
    SDL_GetWindowSize(GetWindow(), &windowSize.x, &windowSize.y);

    glm::ivec2 displaySize;
    SDL_GL_GetDrawableSize(GetWindow(), &displaySize.x, &displaySize.y);
    windowFramebufferScale = (windowSize.x > 0 && windowSize.y > 0) ?
                                 ((glm::fvec2)displaySize / (glm::fvec2)windowSize) :
                                 glm::fvec2{ 1, 1 };
}
void SDLImpl::ProcessGamepadInput(ImGuiIO& io)
{
    //TODO: Support multiple controllers at once.
    auto* gameController = SDL_GameControllerOpen(0);
    if (gameController == nullptr)
    {
        io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
        return;
    }

    #define MAP_BUTTON(NAV_NO, BUTTON_NO) { \
        io.NavInputs[NAV_NO] = (SDL_GameControllerGetButton(gameController, BUTTON_NO) != 0) ? \
                                   1.0f : 0.0f; \
    }
    #define MAP_ANALOG(NAV_NO, AXIS_NO, V0, V1) { \
        float vn = (float)(SDL_GameControllerGetAxis(gameController, AXIS_NO) - V0) \
                       / (float)(V1 - V0); \
        if (vn > 1.0f) vn = 1.0f; \
        if (vn > 0.0f && io.NavInputs[NAV_NO] < vn) io.NavInputs[NAV_NO] = vn; \
    }

    const int thumb_dead_zone = 8000; // SDL_gamecontroller.h suggests using this value.

    MAP_BUTTON(ImGuiNavInput_Activate,      SDL_CONTROLLER_BUTTON_A);               // Cross / A
    MAP_BUTTON(ImGuiNavInput_Cancel,        SDL_CONTROLLER_BUTTON_B);               // Circle / B
    MAP_BUTTON(ImGuiNavInput_Menu,          SDL_CONTROLLER_BUTTON_X);               // Square / X
    MAP_BUTTON(ImGuiNavInput_Input,         SDL_CONTROLLER_BUTTON_Y);               // Triangle / Y
    MAP_BUTTON(ImGuiNavInput_DpadLeft,      SDL_CONTROLLER_BUTTON_DPAD_LEFT);       // D-Pad Left
    MAP_BUTTON(ImGuiNavInput_DpadRight,     SDL_CONTROLLER_BUTTON_DPAD_RIGHT);      // D-Pad Right
    MAP_BUTTON(ImGuiNavInput_DpadUp,        SDL_CONTROLLER_BUTTON_DPAD_UP);         // D-Pad Up
    MAP_BUTTON(ImGuiNavInput_DpadDown,      SDL_CONTROLLER_BUTTON_DPAD_DOWN);       // D-Pad Down
    MAP_BUTTON(ImGuiNavInput_FocusPrev,     SDL_CONTROLLER_BUTTON_LEFTSHOULDER);    // L1 / LB
    MAP_BUTTON(ImGuiNavInput_FocusNext,     SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);   // R1 / RB
    MAP_BUTTON(ImGuiNavInput_TweakSlow,     SDL_CONTROLLER_BUTTON_LEFTSHOULDER);    // L1 / LB
    MAP_BUTTON(ImGuiNavInput_TweakFast,     SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);   // R1 / RB
    MAP_ANALOG(ImGuiNavInput_LStickLeft,    SDL_CONTROLLER_AXIS_LEFTX, -thumb_dead_zone, -32768);
    MAP_ANALOG(ImGuiNavInput_LStickRight,   SDL_CONTROLLER_AXIS_LEFTX, +thumb_dead_zone, +32767);
    MAP_ANALOG(ImGuiNavInput_LStickUp,      SDL_CONTROLLER_AXIS_LEFTY, -thumb_dead_zone, -32767);
    MAP_ANALOG(ImGuiNavInput_LStickDown,    SDL_CONTROLLER_AXIS_LEFTY, +thumb_dead_zone, +32767);

    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    #undef MAP_BUTTON
    #undef MAP_ANALOG
}

void SDLImpl::BeginFrame(float deltaTime)
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.Fonts->IsBuilt() &&
              "Font atlas not built! It should be built by the renderer back-end.\
 Did you call ImGuiOpenGLInterface.BeginFrame()?");

    io.DeltaTime = deltaTime;

    //Set up display size. We're doing this every frame to accomodate window-resizing.
    glm::ivec2 windowSize;
    glm::fvec2 windowDisplayScale;
    GetWindowDisplayScale(windowSize, windowDisplayScale);
    io.DisplaySize = { windowSize.x, windowSize.y };
    io.DisplayFramebufferScale = { windowDisplayScale.x, windowDisplayScale.y };


    //Handle mouse events.

    // Set OS mouse position if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
    if (io.WantSetMousePos)
        SDL_WarpMouseInWindow(GetWindow(), (int)io.MousePos.x, (int)io.MousePos.y);
    else
        io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);

    //Update mouse down events.
    glm::ivec2 mousePos;
    auto mouseButtons = RefreshMouseData(mousePos);
    for (uint8_t i = 0; i < mouseButtons.size(); ++i)
        io.MouseDown[i] = mouseButtons[i];

    //SDL_GetMouseState() gives mouse position seemingly based on
    //    the last window entered/focused?
    //Both SDL_CaptureMouse() and the creation of new windows at runtime seem to
    //    severely mess with that, so we retrieve that position globally.
    //Additionally, SDL_CaptureMouse() lets the OS know that dragging outside the SDL window
    //    shouldn't trigger the OS window resize cursor, or other similar events.
    //The functionality is only supported starting SDL 2.0.4 (released Jan 2016)
#if SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE
    if (GetWindow() == SDL_GetKeyboardFocus())
    {
        glm::ivec2 windowPos;
        SDL_GetWindowPosition(GetWindow(), &windowPos.x, &windowPos.y);
        SDL_GetGlobalMouseState(&mousePos.x, &mousePos.y);
        mousePos -= windowPos;
        io.MousePos = { (float)mousePos.x, (float)mousePos.y };
    }

    SDL_CaptureMouse(ImGui::IsAnyMouseDown() ? SDL_TRUE : SDL_FALSE);
#else
    if (SDL_GetWindowFlags(GetWindow()) & SDL_WINDOW_INPUT_FOCUS)
        io.MousePos = { (float)mousePos.x, (float)mousePos.y);
#endif

    //Update mouse cursor.
    if (!(io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange))
    {
        //If ImGui is drawing a cursor manually, hide the OS one.
        auto imGuiCursor = ImGui::GetMouseCursor();
        if (io.MouseDrawCursor || imGuiCursor == ImGuiMouseCursor_None)
        {
            SDL_ShowCursor(SDL_FALSE);
        }
        else
        {
            SDL_SetCursor(mouseCursors[imGuiCursor] ?
                              mouseCursors[imGuiCursor] :
                              mouseCursors[ImGuiMouseCursor_Arrow]);
            SDL_ShowCursor(SDL_TRUE);
        }
    }

    //Update game-pads.
    memset(io.NavInputs, 0, sizeof(io.NavInputs));
    if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) != 0)
        ProcessGamepadInput(io);
}

#pragma endregion


#pragma region Default OpenGL Interface

OGLImpl::ImGuiOpenGLInterface_Default(std::string& outErrorMsg,
                                      const char* glslVersion)
    : ImGuiOpenGLInterface(glslVersion)
{
    //Set back-end capabilities flags.
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "bplus_opengl";
    //We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

    //Backup GL state before creating any objects.
    //Restore it when we're done.
    GLint last_texture, last_array_buffer;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    GLint last_vertex_array;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);

    //Write and compile the shaders.
    auto CheckShader = [&](GLuint handle, const char* description) -> bool
    {
        //Check whether it compiled.
        GLint status = 0;
        glGetShaderiv(handle, GL_COMPILE_STATUS, &status);

        //Get information about it.
        GLint logLength = 0;
        std::string infoLog;
        infoLog.resize(logLength + 1, '\0');
        glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 1)
            glGetShaderInfoLog(handle, logLength, nullptr, (GLchar*)infoLog.data());

        if ((GLboolean)status == GL_FALSE)
        {
            outErrorMsg = std::string("Failed to compile ") + description +
                          ":\n\t" + infoLog;
            return false;
        }
        else
        {
            fprintf(stderr, "ImGUI %s compiled successfully:\n%s\n",
                    description, infoLog.c_str());
            return true;
        }
    };
    //Vertex:
    const GLchar* vertex_shader_glsl =
        "layout (location = 0) in vec2 Position;\n"
        "layout (location = 1) in vec2 UV;\n"
        "layout (location = 2) in vec4 Color;\n"
        "uniform mat4 ProjMtx;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "    Frag_UV = UV;\n"
        "    Frag_Color = Color;\n"
        "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n";
    const GLchar* vertex_shader_with_version[2] = { GetGlslVersion().c_str(),
                                                    vertex_shader_glsl };
    handle_vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(handle_vertShader, 2, vertex_shader_with_version, NULL);
    glCompileShader(handle_vertShader);
    if (!CheckShader(handle_vertShader, "vertex shader"))
        return;
    //Fragment:
    const GLchar* fragment_shader_glsl =
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "uniform sampler2D Texture;\n"
        "layout (location = 0) out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
        "}\n";
    const GLchar* fragment_shader_with_version[2] = { GetGlslVersion().c_str(),
                                                      fragment_shader_glsl };
    handle_fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(handle_fragShader, 2, fragment_shader_with_version, NULL);
    glCompileShader(handle_fragShader);
    if (!CheckShader(handle_fragShader, "fragment shader"))
        return;

    //Link the shaders together:
    handle_shaderProgram = glCreateProgram();
    glAttachShader(handle_shaderProgram, handle_vertShader);
    glAttachShader(handle_shaderProgram, handle_fragShader);
    glLinkProgram(handle_shaderProgram);
    //Check whether it worked.
    GLint programStatus;
    glGetProgramiv(handle_shaderProgram, GL_LINK_STATUS, &programStatus);
    GLint programLogLength = 0;
    std::string infoLog;
    infoLog.resize(programLogLength + 1, '\0');
    glGetShaderiv(handle_shaderProgram, GL_INFO_LOG_LENGTH, &programLogLength);
    if (programLogLength > 1)
        glGetShaderInfoLog(handle_shaderProgram, programLogLength, nullptr, (GLchar*)infoLog.data());
    if ((GLboolean)programStatus == GL_FALSE)
    {
        outErrorMsg = "Unable to link vertex and fragment shader:\n\t";
        outErrorMsg += infoLog;
        return;
    }

    //Get attribute/uniform locations.
    attrib_pos = glGetUniformLocation(handle_shaderProgram, "Position");
    attrib_uv = glGetUniformLocation(handle_shaderProgram, "UV");
    attrib_color = glGetUniformLocation(handle_shaderProgram, "Color");
    uniform_tex = glGetUniformLocation(handle_shaderProgram, "Texture");
    uniform_projectionMatrix = glGetUniformLocation(handle_shaderProgram, "ProjMtx");

    //Create mesh buffers.
    glGenBuffers(1, &handle_vbo);
    glGenBuffers(1, &handle_elements);

    //Create the fonts texture.
    {
        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        glGenTextures(1, &handle_fontTexture);
        glBindTexture(GL_TEXTURE_2D, handle_fontTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

        io.Fonts->TexID = (ImTextureID)(intptr_t)handle_fontTexture;
    }

    //Restore the OpenGL state that we modified.
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    glBindVertexArray(last_vertex_array);
}
OGLImpl::~ImGuiOpenGLInterface_Default()
{
    if (handle_vbo)
        glDeleteBuffers(1, &handle_vbo);
    if (handle_elements)
        glDeleteBuffers(1, &handle_elements);

    if (handle_shaderProgram)
    {
        if (handle_vertShader)
            glDetachShader(handle_shaderProgram, handle_vertShader);
        if (handle_fragShader)
            glDetachShader(handle_shaderProgram, handle_fragShader);
    }

    if (handle_vertShader)
        glDeleteShader(handle_vertShader);
    if (handle_fragShader)
        glDeleteShader(handle_fragShader);

    if (handle_shaderProgram)
        glDeleteProgram(handle_shaderProgram);

    if (handle_fontTexture)
    {
        ImGuiIO& io = ImGui::GetIO();
        glDeleteTextures(1, &handle_fontTexture);
        io.Fonts->TexID = 0;
    }
}

void OGLImpl::RenderFrame()
{
    auto* drawData = ImGui::GetDrawData();

    //TODO: Implement.
}

#pragma endregion