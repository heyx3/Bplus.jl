#pragma once

#include "../Dependencies.h"
#include "Data.h"
#include "Buffers/MeshData.h"
#include "../Math/Box.hpp"

#include <optional>
#include <array>
#include <functional>


namespace Bplus::GL
{
    //TODO: Upgrade to OpenGL 4.6.
    //TODO: Changing viewport Y axis and depth (how can GLM support this depth mode?): https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glClipControl.xhtml
    //TODO: Give various object names with glObjectLabel

    //Forward declarations:
    namespace Materials { class BP_API CompiledShader; }

    //Represents OpenGL's global state, like the current blend mode and stencil test.
    //Does not include some things like bound objects, shader uniforms, etc.
    class BP_API RenderState
    {
    public:

        glm::bvec4 ColorWriteMask = glm::bvec4{ true, true, true, true };
        bool EnableDepthWrite = true;

        ValueTests DepthTest = ValueTests::LessThanOrEqual;

        FaceCullModes CullMode = FaceCullModes::On;

        BlendStateRGB ColorBlending = BlendStateRGB::Opaque();
        BlendStateAlpha AlphaBlending = BlendStateAlpha::Opaque();

        StencilTest StencilTestFront = StencilTest(),
                    StencilTestBack = StencilTest();
        StencilResult StencilResultFront = StencilResult(),
                      StencilResultBack = StencilResult();
        GLuint StencilMaskFront = 0,
               StencilMaskBack = 0;

        //TODO: glPolygonMode controls "wireframe mode", for front and back faces
        //TODO: Anything else?


        //Gets the stencil test, assuming it's the same for both front and back faces.
        StencilTest GetStencilTest() const
        {
            BP_ASSERT(StencilTestFront == StencilTestBack,
                     "Using different stencil tests for front vs back faces");
            return StencilTestFront;
        }
        //Sets the stencil test (for both front and back faces) to the given value.
        void SetStencilTest(StencilTest newVal)
        {
            StencilTestFront = newVal;
            StencilTestBack = newVal;
        }

        //Gets the stencil test response, assuming it's the same for both front and back faces.
        StencilResult GetStencilResult() const
        {
            BP_ASSERT(StencilResultFront == StencilResultBack,
                "Using different stencil results for front vs back faces");
            return StencilResultFront;
        }
        //Sets the stencil test (for both front and back faces) to the given value.
        void SetStencilResult(StencilResult newVal)
        {
            StencilResultFront = newVal;
            StencilResultBack = newVal;
        }

        //Gets the stencil mask, assuming it's the same for both front and back faces.
        GLuint GetStencilMask() const
        {
            BP_ASSERT(StencilMaskFront == StencilMaskBack,
                     "Using different stencil masks for front vs back faces");
            return StencilMaskFront;
        }
        //Sets the stencil mask (for both front and back faces) to the given value.
        void SetStencilMask(GLuint newVal)
        {
            StencilMaskFront = newVal;
            StencilMaskBack = newVal;
        }
    };

    

    //Information that is common to most modes of rendering.
    struct BP_API DrawMeshMode_Basic
    {
        //The mesh to use.
        const Buffers::MeshData& Data;
        //The range of vertices (or indices) to draw.
        Math::IntervalU Elements;
        //The type of shapes being drawn (triangles, lines, triangle fan, etc).
        Buffers::PrimitiveTypes Primitive;

        //Creates an instance with the given fields.
        DrawMeshMode_Basic(const Buffers::MeshData& mesh,
                           Math::IntervalU elements, Buffers::PrimitiveTypes primitive)
            : Data(mesh), Elements(elements), Primitive(primitive) { }
        //Creates an instance with fields derived from the given mesh data,
        //    always starting the mesh from the first available element.
        //If the number of elements to draw is not given,
        //    the maximum possible number of elements is calculated from the mesh's buffer(s).
        DrawMeshMode_Basic(const Buffers::MeshData& dataSrc,
                           std::optional<uint32_t> nElements = std::nullopt);
    };

    //Extra data when drawing a mesh with indexed primitives.
    struct DrawMeshMode_Indexed
    {
        //An index value equal to this does not actually reference a vertex,
        //    but tells OpenGL to restart the primitive for continuous ones
        //    like triangle strip and line strip.
        //Does not affect separated primitive types, like points, triangles, or lines.
        std::optional<uint32_t> ResetValue;

        //All index values are offset by this amount.
        //Does not affect the "ResetValue"; that test happens before this offset.
        size_t ValueOffset = 0;

        DrawMeshMode_Indexed(std::optional<uint32_t> resetValue = std::nullopt,
                             size_t valueOffset = 0)
            : ResetValue(resetValue), ValueOffset(valueOffset) { }
    };
    //Extra data when drawing multiple subsets of a mesh using indexed primitives.
    struct DrawMeshMode_IndexedSubset
    {
        //A special index value that means "start the primitive over",
        //    for continuous primitives like triangle-fan or line-strip.
        std::optional<uint32_t> ResetValue;

        //For each mesh subset being drawn, this provides an offset
        //    for that subset's index values.
        //Does not affect the "ResetValue"; that test happens before this offset
        //    is applied to the value.
        std::vector<uint32_t> ValueOffsets;
    };


    //Manages OpenGL initialization, shutdown,
    //    and global state such as the current blend mode and stencil test.
    //Ensures good performance by remembering the current state and ignoring duplicate calls.
    //Note that only one of these should exist in each thread,
    //    and this constraint is enforced in the constructor.
    class BP_API Context
    {
    public:
        //The GLSL declaration of which OpenGL version is required for BPlus.
        static const char* GLSLVersion() { return "#version 450"; }
        //The GLSL declarations of which extensions are required for BPlus.
        static const std::array<const char*, 2> GLSLExtensions() { return { "#extension GL_ARB_bindless_texture : require",
                                                                            "#extension GL_ARB_gpu_shader_int64 : require" }; }

        static uint8_t GLVersion_Major() { return 4; }
        static uint8_t GLVersion_Minor() { return 5; }
        //TODO: Enumerate all the extensions, and check that they are supported.

        //May be null if no context exists right now.
        //Note that each thread has its own singleton instance.
        static Context* GetCurrentContext();


        #pragma region Callbacks

        //Registers a callback for when this thread's context is destroyed.
        static void RegisterCallback_Destroyed(std::function<void()>);

        //Registers a callback for when this thread's context's "RefreshState()" is called.
        static void RegisterCallback_RefreshState(std::function<void()>);

        #pragma endregion


        //Creates the context based on the given SDL window.
        //If there was an error, "errorMsg" will be set to it.
        Context(SDL_Window* owner, std::string& errorMsg,
                VsyncModes vsync = VsyncModes::Off);
        ~Context();

        //Gets whether this context got initialized properly.
        bool WasInitialized() const { return isInitialized; }
        //Gets the SDL window this context was created for.
        SDL_Window* GetOwner() const { return owner; }
        SDL_GLContext GetSDLContext() const { return sdlContext; }

        //Queries OpenGL for the current context state.
        //Call this after any OpenGL work is done not through this class.
        void RefreshState();


        const RenderState& GetState() const { return state; }
        void SetState(const RenderState& newState);


        OglPtr::Target GetActiveTarget() const { return activeRT; }
        void SetActiveTarget(OglPtr::Target t);
        void ClearActiveTarget(bool resetViewport = true, bool resetScissor = true);

        #pragma region Clear operations

        //Clears the default framebuffer's color and depth.
        void ClearScreen(float r, float g, float b, float a, float depth) { ClearScreen(depth); ClearScreen(r, g, b, a); }

        //Clears the default framebuffer's color.
        void ClearScreen(float r, float g, float b, float a);
        //Clears the default framebuffer's depth.
        void ClearScreen(float depth);

        template<typename TVec4>
        void ClearScreen(const TVec4& rgba, std::optional<float> depth = std::nullopt)
        {
            ClearScreen(rgba.r, rgba.g, rgba.b, rgba.a);
            if (depth.has_value())
                ClearScreen(depth.value());
        }

        #pragma endregion

        #pragma region Draw operations

        //Draws the given mesh with the given shader, into the current active Target.
        //Optionally draws in indexed mode.
        //Optionally draws multiple instances of the mesh data.
        void Draw(const DrawMeshMode_Basic& mesh, const Materials::CompiledShader& shader,
                  std::optional<DrawMeshMode_Indexed> indices = std::nullopt,
                  std::optional<Math::IntervalU> instancing = std::nullopt) const;

        //Draws multiple subsets of the given mesh using the given shader,
        //    drawing into the current active Target.
        //Optionally draws in indexed mode.
        void Draw(const Buffers::MeshData& mesh, Buffers::PrimitiveTypes primitive,
                  const Materials::CompiledShader& shader,
                  const std::vector<Math::IntervalU>& subsets,
                  std::optional<DrawMeshMode_IndexedSubset> indices = std::nullopt) const;

        //Draws the given mesh using indexed rendering, with the given shader,
        //    drawing into the current active Target.
        //Also tells the graphics driver which subset of the mesh's vertices
        //    are actually used, so it can optimize memory access.
        void Draw(const DrawMeshMode_Basic& mesh, const Materials::CompiledShader& shader,
                  const DrawMeshMode_Indexed& indices,
                  const Math::IntervalU& knownVertexRange) const;


        //The notes I took when preparing the draw calls interface:
        //All draw modes:
        //   * Normal              "glDrawArrays()" ("first" element index and "count" elements)
        //   * Normal + Multi-Draw "glMultiDrawArrays()" (multiple Normal draws from the same buffer data)
        //   * Normal + Instance   "glDrawArraysInstanced()" (draw multiple instances of the same mesh).
        //        should actually use "glDrawArraysInstancedBaseInstance()" to support an offset for the first instance to use
        //
        //   * Indexed              "glDrawElements()" (draw indices instead of vertices)
        //   * Indexed + Multi-Draw "glMultiDrawElements()"
        //   * Indexed + Instance   "glDrawElementsInstanced()" (draw multiple instances of the same indexed mesh).
        //        should actually use "glDrawElementsInstancedBaseInstance()" to support an offset for the first instance to use
        //   * Indexed + Range      "glDrawRangeElements()" (provide the known range of indices that could be drawn, for driver optimization)
        //
        //   * Indexed + Base Index              "glDrawElementsBaseVertex()" (an offset for all indices)
        //   * Indexed + Base Index + Multi-Draw "glMultiDrawElementsBaseVertex()" (each element of the multi-draw has a different "base index" offset)
        //   * Indexed + Base Index + Range      "glDrawRangeElementsBaseVertex()"
        //   * Indexed + Base Index + Instanced  "glDrawElementsInstancedBaseVertex()"
        //        should actually use "glDrawElementsInstancedBaseVertexBaseInstance()" to support an offset for the first instance to use
        //
        //All Indexed draw modes can have a "reset index", which is
        //    a special index value to reset for continuous fan/strip primitives

        #pragma endregion

        //TODO: Indirect drawing: glDrawArraysIndirect(), glMultiDrawArraysIndirect(), glDrawElementsIndirect(), and glMultiDrawElementsIndirect().

        bool SetVsyncMode(VsyncModes mode);
        VsyncModes GetVsyncMode() const { return vsync; }

        void SetFaceCulling(FaceCullModes mode);
        FaceCullModes GetFaceCulling() const { return state.CullMode; }

        #pragma region Viewport

        void SetViewport(int minX, int minY, int width, int height);
        void SetViewport(int width, int height) { SetViewport(0, 0, width, height); }
        void SetViewport(const Math::Box2Di& area) { SetViewport(area.MinCorner.x, area.MinCorner.y, area.Size.x, area.Size.y); }

        void GetViewport(int& outMinX, int& outMinY, int& outWidth, int& outHeight) const;
        Math::Box2Di GetViewport() const { Math::Box2Di box; GetViewport(box.MinCorner.x, box.MinCorner.y, box.Size.x, box.Size.y); return box; }

        void SetScissor(int minX, int minY, int width, int height);
        void SetScissor(const Math::Box2Di& area) { SetScissor(area.MinCorner.x, area.MinCorner.y, area.Size.x, area.Size.y); }
        void DisableScissor();

        //If scissor is disabled, returns false.
        //Otherwise, returns true and sets the given output parameters.
        bool GetScissor(int& outMinX, int& outMinY, int& outWidth, int& outHeight) const;
        //If scissor is disabled, returns nothing.
        std::optional<Math::Box2Di> GetScissor() const
        {
            Math::Box2Di box;
            if (GetScissor(box.MinCorner.x, box.MinCorner.y, box.Size.x, box.Size.y))
                return box;
            else
                return std::nullopt;
        }

        #pragma endregion

        #pragma region Depth/Color

        void SetDepthTest(ValueTests mode);
        ValueTests GetDepthTest() const { return state.DepthTest; }

        void SetDepthWrites(bool canWriteToDepth);
        bool GetDepthWrites() const { return state.EnableDepthWrite; }

        void SetColorWriteMask(glm::bvec4 canWrite);
        glm::bvec4 GetColorWriteMask() const { return state.ColorWriteMask; }

        #pragma endregion

        #pragma region Blending

        //Gets the current global blend operation, assuming
        //    both color and alpha have the same setting.
        BlendStateRGBA GetBlending() const;
        //Sets both color and alpha blending to the given state.
        void SetBlending(const BlendStateRGBA& state);

        BlendStateRGB GetColorBlending() const { return state.ColorBlending; }
        void SetColorBlending(const BlendStateRGB& state);

        BlendStateAlpha GetAlphaBlending() const { return state.AlphaBlending; }
        void SetAlphaBlending(const BlendStateAlpha& state);

        #pragma endregion

        #pragma region Stencil

        //Gets the current global stencil test, assuming
        //    both front- and back-faces have the same stencil test setting.
        const StencilTest& GetStencilTest() const;
        //Sets both front- and back-faces to use the given stencil test.
        void SetStencilTest(const StencilTest& test);

        const StencilTest& GetStencilTestFrontFaces() const { return state.StencilTestFront; }
        void SetStencilTestFrontFaces(const StencilTest& test);

        const StencilTest& GetStencilTestBackFaces() const { return state.StencilTestBack; }
        void SetStencilTestBackFaces(const StencilTest& test);


        //Gets the current global stencil write operations, assuming
        //    both front- and back-faces have the same stencil write settings.
        const StencilResult& GetStencilResult() const;
        //Sets both front- and back-faces to use the given stencil write operations.
        void SetStencilResult(const StencilResult& writeResults);

        const StencilResult& GetStencilResultFrontFaces() const { return state.StencilResultFront; }
        void SetStencilResultFrontFaces(const StencilResult& writeResult);

        const StencilResult& GetStencilResultBackFaces() const { return state.StencilResultBack; }
        void SetStencilResultBackFaces(const StencilResult& writeResult);


        //Gets the current global stencil mask, determining which bits
        //    can actually be written to by the "StencilResult" settings.
        GLuint GetStencilMask() const;
        void SetStencilMask(GLuint newMask);

        GLuint GetStencilMaskFrontFaces() const { return state.StencilMaskFront; }
        void SetStencilMaskFrontFaces(GLuint newMask);

        GLuint GetStencilMaskBackFaces() const { return state.StencilMaskBack; }
        void SetStencilMaskBackFaces(GLuint newMask);

        #pragma endregion

    private:

        bool isInitialized = false;
        SDL_GLContext sdlContext;
        SDL_Window* owner;

        RenderState state;
        glm::ivec4 viewport;
        Lazy<glm::ivec4> scissor;
        VsyncModes vsync;
        
        OglPtr::Target activeRT;
    };
}