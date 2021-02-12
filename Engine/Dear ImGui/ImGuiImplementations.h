#pragma once

#include "ImGuiInterfaces.h"

#include "../Renderer/Textures/TextureD.hpp"
#include "../Renderer/Buffers/MeshData.h"
#include "../Renderer/Materials/CompiledShader.h"


namespace Bplus
{
    #pragma region SDL Interface implementation

    //The default implementation of ImGuiSDLInterface. This is usually enough.
    //Designed to be overridden if your custom use-case isn't too complicated.
    class BP_API ImGuiSDLInterface_Default : public ImGuiSDLInterface
    {
    public:
        ImGuiSDLInterface_Default(SDL_Window* mainWindow, SDL_GLContext glContext);
        virtual ~ImGuiSDLInterface_Default() override;

        void BeginFrame(float deltaTime) override;
        virtual void ProcessEvent(const SDL_Event& event) override;

    protected:

        std::array<bool, 3> mousePressed = { false, false, false };
        std::string clipboard;

        virtual void SetSDLClipboardText(const char* text);
        virtual const char* GetSDLClipboardText();

        virtual std::array<bool, 3> RefreshMouseData(glm::ivec2& outPos);
        virtual void GetWindowDisplayScale(glm::ivec2& windowSize,
                                           glm::fvec2& windowFramebufferScale);
        virtual void ProcessGamepadInput(ImGuiIO& io);

        SDL_Cursor*& GetSDLCursor(ImGuiMouseCursor index) { return mouseCursors[index]; }

    private:

        std::array<SDL_Cursor*, ImGuiMouseCursor_COUNT> mouseCursors;
    };

    #pragma endregion

    #pragma region BPlus Renderer implementation

    //An implementation for ImGuiOpenGLInterface which uses B+.
    class BP_API ImGuiOpenGLInterface_BPlus : public ImGuiOpenGLInterface
    {
    public:
        //Outputs an error message if something went wrong,
        //    or leaves the string alone if successful.
        ImGuiOpenGLInterface_BPlus(std::string& outErrorMsg);
        virtual ~ImGuiOpenGLInterface_BPlus() override;
        
        void RenderFrame() override;
          
    protected:
        //Sets up rendering state to do ImGUI drawing.
        //Used inside "RenderFrame()".
        void PrepareRenderState(ImDrawData& drawData, glm::ivec2 framebufferSize);
        //Used inside "RenderFrame()".
        void RenderCommandList(ImDrawData& drawData, const ImDrawList& commandList,
                               glm::ivec2 framebufferSize,
                               glm::fvec2 clipOffset, glm::fvec2 clipScale,
                               bool clipOriginIsLowerLeft);

    private:

        //OpenGL resources are instantiated/reset as needed.
        std::optional<Bplus::GL::Textures::TexView> fontTextureView;
        std::optional<Bplus::GL::Textures::Texture2D> fontTexture;
        std::optional<Bplus::GL::Buffers::Buffer> verticesBuffer, indicesBuffer;
        std::optional<Bplus::GL::CompiledShader> shader;
        std::optional<Bplus::GL::Buffers::MeshData> meshData;
    };

    #pragma endregion
}