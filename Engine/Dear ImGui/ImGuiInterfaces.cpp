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
    io.DisplaySize = ImVec2((float)windowSize.x, (float)windowSize.y);
    io.DisplayFramebufferScale = ImVec2(windowDisplayScale.x, windowDisplayScale.y);


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
    : ImGuiOpenGLInterface((glslVersion == nullptr) ? GL::Context::GLSLVersion() : glslVersion)
{
    //Set back-end capabilities flags.
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "bplus_opengl";
    //We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

    //Backup GL state before creating any objects.
    //Restore it when we're done.
    GLint lastTexture, lastArrayBuffer;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &lastTexture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &lastArrayBuffer);
    GLint lastVertexArray;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &lastVertexArray);

    //Write and compile the shaders.
    auto CheckShader = [&](GLuint handle, const char* description) -> bool
    {
        //Check whether it compiled.
        GLint status = 0;
        glGetShaderiv(handle, GL_COMPILE_STATUS, &status);

        //Get information about it.
        GLint logLength = 0;
        std::string infoLog;
        glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &logLength);
        infoLog.resize(logLength + 1, '\0');
        if (logLength >= 1)
            glGetShaderInfoLog(handle, logLength, nullptr, (GLchar*)infoLog.data());

        if ((GLboolean)status == GL_FALSE)
        {
            outErrorMsg = std::string("Failed to compile ") + description +
                ":\n\t" + infoLog;
            return false;
        }
        else
        {
            fprintf(stderr, "ImGUI %s compiled successfully. %s\n",
                    description, infoLog.c_str());
            return true;
        }
    };
    //Vertex:
    const GLchar* vertexShaderStr = "\n\
layout (location = 0) in vec2 Position;    \n\
layout (location = 1) in vec2 UV;       \n\
layout (location = 2) in vec4 Color;        \n\
uniform mat4 ProjMtx;               \n\
out vec2 Frag_UV;                   \n\
out vec4 Frag_Color;                \n\
void main()                 \n\
{                           \n\
    Frag_UV = UV;           \n\
    Frag_Color = Color;     \n\
    gl_Position = ProjMtx * vec4(Position.xy, 0, 1);   \n\
}\n";
    const GLchar* vertexShaderFull[2] = { GetGlslVersion().c_str(),
                                          vertexShaderStr };
    handle_vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(handle_vertShader, 2, vertexShaderFull, nullptr);
    glCompileShader(handle_vertShader);
    if (!CheckShader(handle_vertShader, "vertex shader"))
        return;
    //Fragment:
    const GLchar* fragmentShaderStr = "\n\
in vec2 Frag_UV;      \n\
in vec4 Frag_Color;   \n\
uniform sampler2D Texture;  \n\
layout (location = 0) out vec4 Out_Color;  \n\
void main()     \n\
{    \n\
    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);    \n\
}\n";
    const GLchar* fragmentShaderFull[2] = { GetGlslVersion().c_str(),
                                            fragmentShaderStr };
    handle_fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(handle_fragShader, 2, fragmentShaderFull, nullptr);
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
    glGetShaderiv(handle_shaderProgram, GL_INFO_LOG_LENGTH, &programLogLength);
    infoLog.resize(programLogLength + 1, '\0');
    if (programLogLength >= 1)
        glGetShaderInfoLog(handle_shaderProgram, programLogLength, nullptr, (GLchar*)infoLog.data());
    if ((GLboolean)programStatus == GL_FALSE)
    {
        outErrorMsg = "Unable to link vertex and fragment shader:\n\t";
        outErrorMsg += infoLog;
        return;
    }
    else
    {
        fprintf(stderr, "ImGUI shaders linked successfully. %s\n",
                infoLog);
    }

    //Get attribute/uniform locations.
    glUseProgram(handle_shaderProgram);
    glUseProgram(0);
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
    glBindTexture(GL_TEXTURE_2D, lastTexture);
    glBindBuffer(GL_ARRAY_BUFFER, lastArrayBuffer);
    glBindVertexArray(lastVertexArray);
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

void OGLImpl::ResetRenderState(ImDrawData& drawData, glm::ivec2 framebufferSize,
                               GLuint vao)
{
    //Setup render state: alpha-blending enabled, no face culling, no depth testing,
    //    scissor enabled, polygon fill
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    //Setup the viewport and orthographic projection matrix.
    //Our visible ImGUI space lies from draw_data->DisplayPos (top left)
    //    to draw_data->DisplayPos+data_data->DisplaySize (bottom right).
    //DisplayPos is (0,0) for single viewport apps.
    glViewport(0, 0, (GLsizei)framebufferSize.x, (GLsizei)framebufferSize.y);
    float L = drawData.DisplayPos.x;
    float R = drawData.DisplayPos.x + drawData.DisplaySize.x;
    float T = drawData.DisplayPos.y;
    float B = drawData.DisplayPos.y + drawData.DisplaySize.y;
    const float ortho_projection[4][4] =
    {
        { 2.0f/(R-L),   0.0f,         0.0f,   0.0f },
        { 0.0f,         2.0f/(T-B),   0.0f,   0.0f },
        { 0.0f,         0.0f,        -1.0f,   0.0f },
        { (R+L)/(L-R),  (T+B)/(B-T),  0.0f,   1.0f },
    };

    glUseProgram(handle_shaderProgram);
    glUniformMatrix4fv(uniform_projectionMatrix, 1, GL_FALSE, &ortho_projection[0][0]);

    //Set the texture.
    glUniform1i(uniform_tex, 0);
    //We use combined texture/sampler state.
    //Otherwise, applications using GL 3.3 may mess with it.
    glBindSampler(0, 0);

    //Set up the vertex data.
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, handle_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle_elements);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0,   2, GL_FLOAT,          GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, pos));
    glVertexAttribPointer(1,   2, GL_FLOAT,          GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, uv));
    glVertexAttribPointer(2,   4, GL_UNSIGNED_BYTE,  GL_TRUE,  sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, col));
}
void OGLImpl::RenderCommandList(ImDrawData& drawData, const ImDrawList& cmdList,
                                glm::ivec2 framebufferSize,
                                glm::fvec2 clipOffset, glm::fvec2 clipScale,
                                bool clipOriginIsLowerLeft,
                                GLuint vao)
{
    //Upload vertex/index buffers.
    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizeiptr)cmdList.VtxBuffer.Size * sizeof(ImDrawVert),
                 (const GLvoid*)cmdList.VtxBuffer.Data, GL_STREAM_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 (GLsizeiptr)cmdList.IdxBuffer.Size * sizeof(ImDrawIdx),
                 (const GLvoid*)cmdList.IdxBuffer.Data, GL_STREAM_DRAW);

    for (int bufferI = 0; bufferI < cmdList.CmdBuffer.Size; ++bufferI)
    {
        const auto& drawCmd = cmdList.CmdBuffer[bufferI];

        //If this command is actually a custom user callback, run that instead.
        if (drawCmd.UserCallback == ImDrawCallback_ResetRenderState)
            ResetRenderState(drawData, framebufferSize, vao);
        else if (drawCmd.UserCallback != nullptr)
            drawCmd.UserCallback(&cmdList, &drawCmd);
        //Otherwise, it's a regular draw command.
        else
        {
            //Project scissor/clipping rectangles into framebuffer space.
            glm::vec4 clipRect;
            clipRect.x = (drawCmd.ClipRect.x - clipOffset.x) * clipScale.x;
            clipRect.y = (drawCmd.ClipRect.y - clipOffset.y) * clipScale.y;
            clipRect.z = (drawCmd.ClipRect.z - clipOffset.x) * clipScale.x;
            clipRect.w = (drawCmd.ClipRect.w - clipOffset.y) * clipScale.y;

            //Only bother drawing if it's inside the frame-buffer.
            if (clipRect.x < framebufferSize.x && clipRect.y < framebufferSize.y &&
                clipRect.z >= 0.0f && clipRect.w >= 0.0f)
            {
                //Apply scissor/clipping rectangle.
                if (clipOriginIsLowerLeft)
                {
                    glScissor((int)clipRect.x,
                                (int)(framebufferSize.y - clipRect.w),
                                (int)(clipRect.z - clipRect.x),
                                (int)(clipRect.w - clipRect.y));
                }
                else
                {
                    glScissor((int)clipRect.x, (int)clipRect.y,
                                (int)clipRect.z, (int)clipRect.w);
                }

                //Bind texture and draw.
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)drawCmd.TextureId);
                glDrawElementsBaseVertex(GL_TRIANGLES, (GLsizei)drawCmd.ElemCount,
                                            (sizeof(ImDrawIdx) == 2) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                                            (void*)(intptr_t)(drawCmd.IdxOffset * sizeof(ImDrawIdx)),
                                            (GLint)drawCmd.VtxOffset);
            }
        }
    }
}

void OGLImpl::RenderFrame()
{
    auto* drawData = ImGui::GetDrawData();

    //Scale coordinates for retina displays.
    auto framebufferSize = (glm::ivec2)
                              (glm::fvec2{ drawData->DisplaySize.x, drawData->DisplaySize.y } *
                               glm::fvec2{ drawData->FramebufferScale.x, drawData->FramebufferScale.y });

    //Avoid rendering when minimized.
    if (framebufferSize.x <= 0 || framebufferSize.y <= 0)
        return;

    //Backup GL state, and then restore it at the end.
    //That way nobody outside ImGui has to worry about what OpenGL state is changing.
    GLenum lastActiveTexture; glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&lastActiveTexture);
    glActiveTexture(GL_TEXTURE0);
    GLint lastProgram; glGetIntegerv(GL_CURRENT_PROGRAM, &lastProgram);
    GLint lastTexture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &lastTexture);
    GLint lastSampler; glGetIntegerv(GL_SAMPLER_BINDING, &lastSampler);
    GLint lastArrayBuffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &lastArrayBuffer);
    GLint lastVAO; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &lastVAO);
    GLint lastPolygonMode[2]; glGetIntegerv(GL_POLYGON_MODE, lastPolygonMode);
    GLint lastViewport[4]; glGetIntegerv(GL_VIEWPORT, lastViewport);
    GLint lastScissorBox[4]; glGetIntegerv(GL_SCISSOR_BOX, lastScissorBox);
    GLenum lastBlendSrcRGB; glGetIntegerv(GL_BLEND_SRC_RGB, (GLint*)&lastBlendSrcRGB);
    GLenum lastBlendDestRGB; glGetIntegerv(GL_BLEND_DST_RGB, (GLint*)&lastBlendDestRGB);
    GLenum lastBlendSrcAlpha; glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint*)&lastBlendSrcAlpha);
    GLenum lastBlendDestAlpha; glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint*)&lastBlendDestAlpha);
    GLenum lastBlendEquationRGB; glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint*)&lastBlendEquationRGB);
    GLenum lastBlendEquationAlpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint*)&lastBlendEquationAlpha);
    GLboolean lastBlendEnabled = glIsEnabled(GL_BLEND);
    GLboolean lastCullFaceEnabled = glIsEnabled(GL_CULL_FACE);
    GLboolean lastDepthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean lastScissorTestEnabled = glIsEnabled(GL_SCISSOR_TEST);
    bool clipOriginIsLowerLeft = true;
#if !defined(OS_APPLE)
    GLenum lastClipOrigin = 0; glGetIntegerv(GL_CLIP_ORIGIN, (GLint*)&lastClipOrigin); // Support for GL 4.5's glClipControl(GL_UPPER_LEFT)
    if (lastClipOrigin == GL_UPPER_LEFT)
        clipOriginIsLowerLeft = false;
#endif

    //Recreate the VAO every frame to more easily allow multiple GL contexts to be rendered to
    //    (VAO are not shared among GL contexts).
    //The renderer would actually work without any VAO bound, but then our VertexAttrib calls
    //    would overwrite the default one currently bound.
    GLuint handle_vao = 0;
    glGenVertexArrays(1, &handle_vao);

    ResetRenderState(*drawData, framebufferSize, handle_vao);
    
    //Project scissor/clipping rectangles into framebuffer space.
    auto clipOffset = drawData->DisplayPos;         // (0, 0) unless using multi-viewports)
    auto clipScale = drawData->FramebufferScale; // (1, 1) unless using retina displays

    //Render ImGUI's command lists.
    for (int i = 0; i < drawData->CmdListsCount; ++i)
    {
        const auto& cmdList = *(drawData->CmdLists[i]);
        RenderCommandList(*drawData, *(drawData->CmdLists[i]),
                          framebufferSize,
                          glm::fvec2{clipOffset.x, clipOffset.y},
                          glm::fvec2{clipScale.x, clipScale.y},
                          clipOriginIsLowerLeft, handle_vao);
    }

    //Clean up the temp VAO.
    glDeleteVertexArrays(1, &handle_vao);;

    //Restore the external GL state.
    glUseProgram(lastProgram);
    glBindTexture(GL_TEXTURE_2D, lastTexture);
    glBindSampler(0, lastSampler);
    glActiveTexture(lastActiveTexture);
    glBindVertexArray(lastVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lastArrayBuffer);
    glBlendEquationSeparate(lastBlendEquationRGB, lastBlendEquationAlpha);
    glBlendFuncSeparate(lastBlendSrcRGB, lastBlendDestRGB, lastBlendSrcAlpha, lastBlendDestAlpha);
    if (lastBlendEnabled) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (lastCullFaceEnabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (lastDepthTestEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (lastScissorTestEnabled) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, (GLenum)lastPolygonMode[0]);
    glViewport(lastViewport[0], lastViewport[1], (GLsizei)lastViewport[2], (GLsizei)lastViewport[3]);
    glScissor(lastScissorBox[0], lastScissorBox[1], (GLsizei)lastScissorBox[2], (GLsizei)lastScissorBox[3]);
}

#pragma endregion