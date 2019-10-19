#pragma once

#include "../IO.h"
#include "../RenderLibs.h"
#include "../Config.h"

#include <chrono>
#include <thread>


namespace Apps
{
    //The base class for all apps in this editor.
    class Base
    {
    public:

        //Will be something like "#version 400".
        const char* const GLSLVersion;

        SDL_Window* const MainWindow;
        SDL_GLContext const OpenGLContext;
        ImGuiIO& ImGuiContext;
        Config& Config;

        ErrorCallback OnError;


        //This app will automatically ensure the window
        //    doesn't become smaller than this..
        glm::u32vec2 MinWindowSize{ 250, 250 };

        //The length of each physics time-step.
        //Physics is updated in time-steps of the exact same size each frame,
        //    for more stable and predictable behavior.
        //If the frame-rate is low, multiple physics updates will happen each frame
        //    so the system can keep up.
        float PhysicsTimeStep = 1.0f / 50.0f;
        //The max number of physics updates that can happen per frame.
        //    if more than this are needed in one frame,
        //    physics will appear to run in slow motion.
        //However, this setting is important because without it,
        //    the number of physics steps per frame could escalate endlessly.
        uint_fast32_t MaxPhysicsStepsPerFrame = 10;

        //A minimum cap on frame time.
        //If the frame is faster than this, the program will sleep for a bit.
        float MinDeltaT = -1;


        Base(const char* glslVersion, SDL_Window* mainWindow,
             SDL_GLContext openGLContext, ImGuiIO& imGuiContext,
             ::Config& config, ErrorCallback onError);
        virtual ~Base() { }


        void StartApp();
        void RunAppFrame();

        //Called when quitting the app.
        //If "force" is false, you can return "false" to cancel the quit.
        virtual bool DoQuit(bool force) { return true; }
        //Gets whether the app should quit.
        bool DidQuit() const { return isQuitting; }

        virtual void ProcessOSEvent(const SDL_Event& osEvent);


    protected:

        //Called when starting to run the app.
        virtual void DoBegin() { }

        //Does normal (i.e. non-physics) updates.
        virtual void DoUpdate(float deltaT) { }
        //Does physics updates.
        virtual void DoPhysics(float deltaT) { }

        //Does rendering.
        virtual void DoRendering(float deltaT) { GL::Clear(1, 0, 1, 1, 1); }


    private:

        double timeSinceLastPhysicsUpdate;
        uint64_t lastFrameStartTime;
        bool isQuitting;
    };
}