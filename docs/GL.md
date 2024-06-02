# GL

A thin wrapper over OpenGL 4.6 with the bindless textures extension, plus some helpers to manage a context/GLFW window.

**Important note**, all counting and indices in GL functions are 1-based, to stay consistent with the rest of Julia. They are converted to 0-based under the hood. For example, if doing instanced rendering with `N` instances, you want to pass the instancing range `IntervalU(min=1, max=N)`, not `IntervalU(min=0, max=N-1)`. Of course, GPU-side data such as a mesh's index buffer is still 0-based, so be careful when generating mesh data or indirect drawing data.

## Context

An OpenGL rendering context is a thread-local singleton, and is attached to a specific window. This is encapsulated in the mutable struct `Context`. Within your application code, you can retrieve the context for the current thread with `get_context()`.

Contexts manage several important things:
  * A reference to the `GLFW.Window` and its current vsync setting.
  * The current render state, in the field `state::RenderState`.
    * For convenience, all the fields in `RenderState` are exposed as properties of `Context`. For example, instead of doing `get_context().state.viewport`, you can just do `get_context().viewport`.
  * Which of your requested OpenGL extensions was actually available (by default, a few bindless-texture extensions are requested)
    * Call `extension_supported("GL_ARB_my_extension_name")` to check
  * Any services that are attached to this context (see [Services](#Services)).
  * The specific GPU device limits as reported through OpenGL, in the field `device::Device`.
    * Some of these statistics are not always reported reliably by every driver, so beware when using them.
  * The currently-bound resources -- shader, mesh, UBO slots, SSBO slots.
  * GLFW callbacks. The core GLFW library only allows one subscriber per event; Context allows you to add as many as you want.
    * The [Input](Input.md) and [GUI](GUI.md) services already handle GLFW input, so you likely won't need these callbacks yourself.
    * For more info on the callbacks and their signatures, see the relevant fields in `Context`.

### Initialization

Managing the life of a Context by yourself is not easy, due to Julia's threading model. For simple use-cases, you should just use the [`@game_loop` macro](Helpers.md#Game-Loop) to run all your code within the life of a `Context`.

If `@game_loop` doesn't provide enough flexibility, you can use the lower-level function `bp_gl_context()` to construct the context, run your custom application code, and then clean up the context automatically at the end. This function takes your code as the first parameter, which is an extremely common Julia idiom -- see the official docs about `do` blocks. The rest of the parameters go straight into the `Context` constructor, so refer to that for information on them.

### Render State

VSync (including adaptive sync) is normally managed by GLFW. In B+ it's represented by the `VsyncModes` enum. You can set it on Context creation. You can also change it afterwards with `set_vsync(mode::E_VsyncModes)`. You can get the current value with `get_context().vsync`.

You can easily get the GLFW window's size with `get_window_size()`.

The render state managed by the `Context` is specified by the `RenderState` struct. For any given piece of state defined in this struct, you can interact with it through the `Context` in several ways:

* Get its current value using properties: `get_context().cull_mode`
* Set its current value using properties: `get_context().cull_mode = FaceCullModes.off`
* Sets its current value using a more traditional function: `set_culling(FaceCullModes.off)`.
* Temporarily set its value, run some code, then set it back: `with_culling(FaceCullModes.off) do ... end`.

The following render state data is available:

* `color_write_mask::vRGBA{Bool}` toggles the ability of shaders to write to each individual color channel.
  * `set_color_writes(...)` is the function.
  * `with_color_writes(...)` is the wrapper.
* `depth_write_mask::Bool` toggles the ability of shaders to write to the depth buffer at all.
  * `set_depth_writes(...)` is the function.
  * `with_depth_writes(...)` is the wrapper.
* `cull_mode::E_FaceCullModes` changes back-face vs front-face culling.
  * `set_cull(...)` is the function.
  * `with_culling(...)` is the wrapper.
* `viewport::Box2Di` controls the range of pixels that can be rendered to.
  * More precisely, it maps the NDC output from a vertex shader (positions in the range -1 to +1 along each axis, written to `gl_Position`), to the given range of pixels.
  * `set_viewport(...)` is the function.
  * `with_viewport(...)` is the wrapper.
* `scissor::Optional{Box2Di}` limits the range of pixels that can be affected by draw calls. Pixels outside this box are not affected.
  * Unlike `viewport`, this setting does not change the coordinate math in mapping triangles to screen space. It just hides pixels.
  * `set_scissor(...)` is the function.
  * `with_scissor(...)` is the wrapper.
* `blend_mode::@NamedTuple{rgb::BlendStateRGB, alpha::BlendStateAlpha}` controls how newly-drawn pixels are added to the color already in the render target or framebuffer.
  * See the `BlendState_` struct for info on how to make custom blend modes. There are some presets, for example:
    * Opaque blending of color with `make_blend_opaque(BlendStateRGB)`
    * Additive blending of alpha with `make_blend_additive(BlendStateAlpha)`
    * Alpha blending of all 4 channels with `make_blend_alpha(BlendStateRGBA)`
  * `set_blending(...)` is the function.
  * `with_blending(...)` is the wrapper.
* `depth_test::E_ValueTests` controls the test used against the depth buffer to draw or occlude pixels.
  * `set_depth_test(...)` is the function.
  * `with_depth_test(...)` is the wrapper.
* `stencil_test::Union{StencilTest, @NamedTuple{front::StencilTest, back::StencilTest}}` controls the test used against the stencil buffer to draw or discard pixels. You may optionally use a different test for front-facing geometry vs back-facing geometry.
  * `set_stencil_test(...)`, `set_stencil_test_front(...)`, and `set_stencil_test_back(...)` are the functions.
  * `with_stencil_test(...)`, `with_stencil_test_front(...)`, and `with_stencil_test_back(...)` are the wrappers.
* `stencil_result::Union{StencilResult, @NamedTuple{front::StencilResult, back::StencilResult}}` controls the effect of a pixel on the stencil buffer. You may optionally use a different test for front-facing vs back-facing geometry.
  * `set_stencil_result(...)`, `set_stencil_result_front(...)`, and `set_stencil_result_back(...)` are the functions.
  * `with_stencil_result(...)`, `with_stencil_result_front(...)`, and `with_stencil_result_back(...)` are the wrappers.
* `stencil_write_mask::Union{GLuint, @NamedTuple{front::GLuint, back::GLuint}}` controls which bits of the stencil buffer can be affected by newly-drawn pixels. You may optionally use a different mask for front-facing vs back-facing geometry.
  * `set_stencil_write_mask(...)`, `set_stencil_write_mask_front(...)`, and `set_stencil_write_mask_back(...)` are the functions.
  * `with_stencil_write_mask(...)`, `with_stencil_write_mask_front(...)`, and `with_stencil_write_mask_back(...)` are the functions.
* `render_state::RenderState` controls the entire state at once. This is not more or less efficient than individually setting the states you care about; just different.
  * `set_render_state(...)` is the function.
  * `with_render_state(...)` is the wrapper.

### Refreshing the context

If external code makes OpenGL calls and modifies the driver state, you can call `refresh(context)` to force the Context to acknowledge these changes. This could be a very slow operation, so avoid using if at all possible!

## Resources

For low-level OpenGL work (which you usually don't need to do), OpenGL handle typedefs are provided in *GL/handles.jl*. These handles are all prefixed with `Ptr_`. For example, `Ptr_Program`, `Ptr_Texture`, `Ptr_Mesh`. These types aren't exported by default.

Managed OpenGL objects are instances of `AbstractResource`. All resource types implement the following interface (see doc-strings for more details):

* `get_ogl_handle(resource)`
* `is_destroyed(resource)::Bool`
* `Base.close(resource)` to clean it up

Resources cannot be copied and their fields cannot be directly set; instead you should use whatever interface that resource provides.

For more info on each type of resource, see [this document](Resources.md). The types of resources are as follows:

* [Program](Resources.md#Program) : A compiled shader
* [Texture](Resources.md#Texture) : A GPU texture (1D, 2D, 3D, cubemap)
* [Buffer](Resources.md#Buffer) : A GPU-side array of data
* [Mesh](Resources.md#Mesh) : What OpenGL calls a "Vertex Array Object".
* [Target](Resources.md#Target) : A render target, or what OpenGL calls a "FrameBuffer Object".
* [TargetBuffer](Resources.md#TargetBuffer) : Used internally when you don't need part of a Target to be sampleable (e.x. you need a depth buffer for depth-testing but not for custom effects).

## Sync

* To make sure that a specific future action can see the results of previous actions (for example, sampling from a texture after writing to it with a compute shader), call `gl_catch_up_before()`.
  * This is only for "incoherent" actions, which OpenGL can't predict, as opposed to coherent actions like rendering into a target.
  * The variant `gl_catch_up_renders_before()` is potentially more efficient when you are specifically considering actions taken in fragment shaders, concerning only a subset of the Target that you just rendered to.
* To force all submitted OpenGL commands to finish right now, call `gl_execute_everything()`. This isn't very useful, except in a few scenarios:
  * You are sharing resources with another context (not natively supported in B+ anyway),
       to ensure the shared resources are done being written to by the source context.
  * You are tracking down a driver bug, and want to call this after every draw call
       to track down the one that crashes.
* If you are ping-ponging within a single texture, by reading from and rendering to separate regions, you should call `gl_flush_texture_writes_in_place()` before reading from the region you just wrote to.
* 

## Services

Sometimes you want your own little singleton associated with a window or rendering context, providing easy access to utilities and resources. These singletons are called "Services". You can define a new service using the `@bp_service()` macro. Its doc-string describes how to make and use it.

The following servces are built into B+:

* `Input`, from the [*BplusApp.Input module](Input.md), provides a high-level view into GLFW keyboard, mouse, and gamepad inputs. Automatically managed for you if you use the [`@game_loop`](Helpers.md#Game-Loop) macro.
* `GUI`, from the [*BplusApp.GUI module](GUI.md), takes care of Dear ImGUI integration. Automatically managed for you if you use the [`@game_loop`](Helpers.md#Game-Loop) macro.
* [`BasicGraphics`, from the *BplusApp module](Helpers.md#Basic-Graphics), provides simple shaders and meshes. Automatically managed for you if you use the [`@game_loop`](Helpers.md#Game-Loop) macro.
* `SamplerProvider` is an internal service that allocates sampler objects as needed. Separate textures with the same sampler settings will reference the same sampler.
* `ViewDebugging` helps catch *some* uses of bindless texture handles without properly activating them. It's only enabled if the *BplusApp.GL* module is in [debug mode](Utilities.md#Asserts), or the GL Context is started in debug mode.

## Clearing

The screen can be cleared with `clear_screen(values...)`:

* To clear color, pass a 4D vector of component type `GLfloat` (you can use the alias `vRGBAf`).
  * If the back buffer is somehow in a non-normalized integer format, provide a component type of either `GLint` (`vRGBAi`) or `GLuint` (`vRGBAu`) as appropriate.
* To clear depth, pass a single `GLfloat` (a.k.a. `Float32`).
* To clear stencil, pass a single `GLuint` (a.k.a. `UInt32`).
* To clear a hybrid depth-stencil buffer, pass two arguments: the depth as `GLfloat` and the stencil as `GLuint`.

## Drawing

Draw a `Mesh` resource `m`, using a `Program` resource `p`, by calling:

`render_mesh(m, p; args...)`

There are numerous named parameters to customize the draw call. **Important note**, as with the rest of the *BplusApp.GL* module, all counting and indices in these paramters are 1-based, to stay consistent with the rest of Julia. They are converted to 0-based under the hood. For example, if doing instanced rendering with `N` instances, you want to pass the range `IntervalU(min=1, size=N)`, not `IntervalU(min=0, size=N)`.

* `shape::E_PrimitiveTypes` changes the type of primitive (line strip, triangle list, etc). By default, this information is taken from the `Mesh` being drawn.
* `indexed_params::Optional{DrawIndexed}` controls indexed drawing. By default, if the `Mesh` has indices, it will draw in indexed mode using all availble indices.
* `elements::Union{IntervalU, Vector{IntervalU}, ConstVector{IntervalU}}` controls which parts of the mesh are drawn. By default, the mesh's entire available data is drawn. You can pass a different contiguous range of elements to darw with. You can also do "multidraw", by providing a list of contiguous ranges.
* `instances::Optional{IntervalU}` allows you to optionally do instanced rendering, where the the mesh is re-drawn `N` times, providing a different "instance index" to the shader each time. Note that you can pass any contiguous interval of instances; it doesn't have to start at 1.
* `known_vertex_range::Optional{IntervalU}` can provide the driver with an optimization hint about the range of vertices being referenced by the indices you are drawing. This is mainly for if you are using indexed rendering *and* passing a custom range for `elements`.

**Important note**: Three of the parameters above are mutually-exclusive: multi-draw with `elements`, instancing with `instances`, and the optimization hint `known_vertex_range`. If you attempt to use more than one at once, you will get an error.

*NOTE*: There is one small, esoteric OpenGL feature that is not included, because adding it would probably require separating this function into 3 different overloads: indexed multi-draw is allowed to use different index offsets for each subset of elements.

*NOTE*: Indirect draw calls are not yet implemented. If you dispatch them yourself, remember to leave the OpenGL state unchanged, or else refresh the Context with `refresh(get_context())`.

## Compute Dispatch

Dispatch a compute shader with `dispatch_compute_threads()` or `dispatch_compute_groups()`.

## Debugging

For now, B+ uses the older form of OpenGL error catching. Call `pull_gl_logs()` to get a list, in chronological order, of all messages/errors that have occurred since the last time you called `pull_gl_logs()`. This list may drop messages if it's not checked for too long, so it's recommended to call it at least once a frame in [debug builds](Utilities.md#Asserts) of your game.

For convenience, you can use the macro `@check_gl_logs(msg...)` to pull log events and immediately print them along with the file and line number, and any information you provide.