#include "ImGuiImplementations.h"

#include "../GL/Context.h"


//This file is a custom rewrite of Dear ImGUI's built-in example SDL and OpenGL3 integrations.

#define SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE    SDL_VERSION_ATLEAST(2,0,4)

using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Buffers;
using namespace Bplus::GL::Textures;
namespace Vertices = Bplus::GL::Buffers::VertexData;

using SDLImpl = ImGuiSDLInterface_Default;
using OGLImpl_BPlus = ImGuiOpenGLInterface_BPlus;



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

#pragma region Default B+ interface

OGLImpl_BPlus::ImGuiOpenGLInterface_BPlus(std::string& outErrorMsg)
    : ImGuiOpenGLInterface(GL::Context::GLSLVersion())
{
    //Set back-end capabilities flags.
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "bplus_opengl";
    //We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

    ShaderCompileJob compiler;
    compiler.VertexSrc = "\n\
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
    compiler.FragmentSrc = "\n\
in vec2 Frag_UV;      \n\
in vec4 Frag_Color;   \n\
layout(bindless_sampler) uniform sampler2D Texture;  \n\
layout (location = 0) out vec4 Out_Color;  \n\
void main()     \n\
{    \n\
    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);    \n\
}\n";
    compiler.PreProcessIncludes();

    bool _;
    OglPtr::ShaderProgram shaderPtr;
    std::tie(outErrorMsg, _) = compiler.Compile(shaderPtr);
    if (shaderPtr.IsNull())
    {
        outErrorMsg = "Error in shaders: " + outErrorMsg;
        return;
    }
    
    //Configure the render state for this shader:
    RenderState shaderRenderMode;
    shaderRenderMode.DepthTest = ValueTests::Off;
    shaderRenderMode.CullMode = FaceCullModes::Off;
    shaderRenderMode.ColorBlending = BlendStateRGB::Transparent();
    shaderRenderMode.AlphaBlending = BlendStateAlpha::Opaque();
    shader.emplace(shaderRenderMode, shaderPtr,
                   std::vector<std::string>{ "ProjMtx", "Texture" });

    //Create and configure the fonts texture.
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    fontTexture.emplace(glm::uvec2((glm::u32)width, (glm::u32)height),
                        SimpleFormat(FormatTypes::NormalizedUInt,
                                     SimpleFormatComponents::RGBA,
                                     SimpleFormatBitDepths::B8));
    fontTexture->Set_Color(pixels, PixelIOChannels::RGBA);
    fontTextureView = fontTexture->GetView();
    io.Fonts->TexID = (ImTextureID)(&fontTexture.value());
}
OGLImpl_BPlus::~ImGuiOpenGLInterface_BPlus()
{
    //The font texture's view must get cleaned up before the texture itself.
    fontTextureView.reset();
    fontTexture.reset();

    //The mesh data object should get cleaned up before the buffers storing that data.
    meshData.reset();
    verticesBuffer.reset();
    indicesBuffer.reset();

    ImGui::GetIO().Fonts->TexID = 0;
}

void OGLImpl_BPlus::PrepareRenderState(ImDrawData& drawData, glm::ivec2 framebufferSize)
{
    auto& context = *Context::GetCurrentContext();

    //Setup the viewport and orthographic projection matrix.
    //Our visible ImGUI space lies from draw_data->DisplayPos (top left)
    //    to draw_data->DisplayPos+data_data->DisplaySize (bottom right).
    //DisplayPos is (0,0) for single viewport apps.

    context.SetViewport(framebufferSize.x, framebufferSize.y);

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
    bool success = shader->SetUniform("ProjMtx", glm::make_mat4<float>(&ortho_projection[0][0]));
    BP_ASSERT(success, "Failed to set projection matrix for ImGUI renderer");

    //Give the font texture to the shader.
    shader->SetUniform("Texture", fontTextureView.value());
}
void OGLImpl_BPlus::RenderCommandList(ImDrawData& drawData, const ImDrawList& cmdList,
                                      glm::ivec2 framebufferSize,
                                      glm::fvec2 clipOffset, glm::fvec2 clipScale,
                                      bool clipOriginIsLowerLeft)
{
    auto& context = *Context::GetCurrentContext();

    auto verticesDataByteSize = cmdList.VtxBuffer.Size * sizeof(ImDrawVert),
         indicesDataByteSize = cmdList.IdxBuffer.Size * sizeof(ImDrawIdx);
    if ((verticesDataByteSize > 0 && indicesDataByteSize > 0) &&
        (!verticesBuffer.has_value() || verticesBuffer->GetByteSize() < verticesDataByteSize ||
         !indicesBuffer.has_value() || indicesBuffer->GetByteSize() < indicesDataByteSize))
    {
        //TODO: I think it's more efficient to store the buffers in CPU memory, via the last (optional) constructor argument.
        verticesBuffer.emplace(sizeof(ImDrawVert) * cmdList.VtxBuffer.Size, true, (const std::byte*)cmdList.VtxBuffer.Data);
        indicesBuffer.emplace(sizeof(ImDrawIdx) * cmdList.IdxBuffer.Size, true, (const std::byte*)cmdList.IdxBuffer.Data);

        meshData.emplace(PrimitiveTypes::Triangle,
                         MeshDataSource(&indicesBuffer.value(), sizeof(ImDrawIdx)),
                         IndexDataTypes::UInt16,
                         std::vector{ MeshDataSource(&verticesBuffer.value(), sizeof(ImDrawVert), 0) },
                         std::vector{ VertexDataField(0, IM_OFFSETOF(ImDrawVert, pos),
                                                      Vertices::Type::FVector<2>()),
                                      VertexDataField(0, IM_OFFSETOF(ImDrawVert, uv),
                                                      Vertices::Type::FVector<2>()),
                                      VertexDataField(0, IM_OFFSETOF(ImDrawVert, col),
                                                      Vertices::Type::IColor(Vertices::GetIVectorType<glm::u8>())) });
    }
    else
    {
        verticesBuffer->Set(cmdList.VtxBuffer.Data,
                            Bplus::Math::IntervalUL::MakeSize(glm::vec<1, glm::u64>(cmdList.VtxBuffer.Size)));
        indicesBuffer->Set(cmdList.IdxBuffer.Data,
                           Bplus::Math::IntervalUL::MakeSize(glm::vec<1, glm::u64>(cmdList.IdxBuffer.Size)));
    }

    for (int bufferI = 0; bufferI < cmdList.CmdBuffer.Size; ++bufferI)
    {
        const auto& drawCmd = cmdList.CmdBuffer[bufferI];

        //If this command is actually a custom user callback, run that instead.
        if (drawCmd.UserCallback == ImDrawCallback_ResetRenderState)
            PrepareRenderState(drawData, framebufferSize);
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
                clipRect.z >= 0 && clipRect.w >= 0)
            {
                //Apply scissor/clipping rectangle.
                if (clipOriginIsLowerLeft)
                {
                    context.SetScissor((int)clipRect.x,
                                       (int)(framebufferSize.y - clipRect.w),
                                       (int)(clipRect.z - clipRect.x),
                                       (int)(clipRect.w - clipRect.y));
                }
                else
                {
                    context.SetScissor((int)clipRect.x, (int)clipRect.y,
                                       (int)clipRect.z, (int)clipRect.w);
                }

                //Bind texture and draw.
                auto* texturePtr = (const Texture2D*)drawCmd.TextureId;
                shader->SetUniform("Texture", texturePtr->GetView());
                context.Draw(DrawMeshMode_Basic(*meshData,
                                                Math::IntervalU::MakeMinSize(glm::uvec1{ drawCmd.IdxOffset }, glm::uvec1{ drawCmd.ElemCount }),
                                                PrimitiveTypes::Triangle),
                             *shader,
                             DrawMeshMode_Indexed(std::nullopt, drawCmd.VtxOffset));
            }
        }
    }
}

void OGLImpl_BPlus::RenderFrame()
{
    auto* drawData = ImGui::GetDrawData();
    auto& context = *Context::GetCurrentContext();

    //Scale coordinates for retina displays.
    auto framebufferSize = (glm::ivec2)
                              (glm::fvec2{ drawData->DisplaySize.x, drawData->DisplaySize.y } *
                               glm::fvec2{ drawData->FramebufferScale.x, drawData->FramebufferScale.y });

    //Avoid rendering when minimized.
    if (framebufferSize.x <= 0 || framebufferSize.y <= 0)
        return;

    bool clipOriginIsLowerLeft = true;
#if !defined(OS_APPLE)
    GLenum lastClipOrigin = 0; glGetIntegerv(GL_CLIP_ORIGIN, (GLint*)&lastClipOrigin); // Support for GL 4.5's glClipControl(GL_UPPER_LEFT)
    if (lastClipOrigin == GL_UPPER_LEFT)
        clipOriginIsLowerLeft = false;
#endif

    PrepareRenderState(*drawData, framebufferSize);
    
    //Remember scissor state so that we can un-do our changes to it.
    auto externalScissorState = context.GetScissor();
    auto externalViewport = context.GetViewport();

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
                          clipOriginIsLowerLeft);
    }

    //Reset the state that we changed.
    context.SetViewport(externalViewport);
    if (externalScissorState.has_value())
        context.SetScissor(*externalScissorState);
    else
        context.DisableScissor();
}


#pragma endregion