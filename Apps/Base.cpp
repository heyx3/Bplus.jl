#include "Base.h"

using namespace Apps;

Base::Base(const char* glslVersion, SDL_Window* mainWindow,
           SDL_GLContext openGLContext, ImGuiIO& imGuiContext,
           ::Config& config, ErrorCallback onError)
    : GLSLVersion(glslVersion), MainWindow(mainWindow),
      OpenGLContext(openGLContext), ImGuiContext(imGuiContext),
      Config(config), OnError(onError)
{

}

void Base::StartApp()
{
    timeSinceLastPhysicsUpdate = 0;
    lastFrameStartTime = SDL_GetPerformanceCounter();
    isQuitting = false;

    DoBegin();
}
void Base::RunAppFrame()
{
    uint64_t newFrameTime = SDL_GetPerformanceCounter();
    double deltaT = (newFrameTime - lastFrameStartTime) /
                    (double)SDL_GetPerformanceFrequency();

    //If the frame-rate is too fast, wait.
    if (deltaT < MinDeltaT)
    {
        double missingTime = MinDeltaT - deltaT;
        std::this_thread::sleep_for(std::chrono::duration<double>(missingTime + .00000001));
        RunAppFrame();
        return;
    }

    lastFrameStartTime = newFrameTime;

    //Initialize the GUI frame.
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(MainWindow);
    ImGui::NewFrame();

    //Update physics.
    timeSinceLastPhysicsUpdate += deltaT;
    while (timeSinceLastPhysicsUpdate > PhysicsTimeStep)
    {
        timeSinceLastPhysicsUpdate -= PhysicsTimeStep;
        DoPhysics(PhysicsTimeStep);
    }

    //Update other stuff.
    DoUpdate((float)deltaT);

    //Do rendering.
    GL::SetViewport((int)ImGuiContext.DisplaySize.x,
                    (int)ImGuiContext.DisplaySize.y);
    DoRendering((float)deltaT);
    //Finally, do GUI rendering.
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(MainWindow);
}

void Base::ProcessOSEvent(const SDL_Event& osEvent)
{
    switch (osEvent.type)
    {
        case SDL_EventType::SDL_WINDOWEVENT:
            switch (osEvent.window.event)
            {
                case SDL_WINDOWEVENT_CLOSE:
                    if (osEvent.window.windowID == SDL_GetWindowID(MainWindow) &&
                        DoQuit(false))
                    {
                        isQuitting = true;
                    }
                    break;
                case SDL_WINDOWEVENT_RESIZED:
                    if (osEvent.window.data1 < MinWindowSize.x ||
                        osEvent.window.data2 < MinWindowSize.y)
                    {
                        SDL_SetWindowSize(MainWindow,
                                          std::max(osEvent.window.data1, (int)MinWindowSize.x),
                                          std::max(osEvent.window.data2, (int)MinWindowSize.y));
                    }
                    break;

                default: break;
            }

        case SDL_QUIT:
            if (DoQuit(false))
                isQuitting = true;
            break;

        default: break;
    }
}