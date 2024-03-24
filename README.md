# B+


A game and 3D graphics framework for Julia, using OpenGL 4.6 + bindless textures.
Offers game math, wrappers for OpenGL/GLFW/Dear ImGUI, scene graph algorithms, and numerous other modules.

A number of example projects are provided in [the *BpExamples* repo](https://github.com/heyx3/BpExamples):

[![Example Projects](https://img.youtube.com/vi/Po3zGQRSRgE/hqdefault.jpg)](https://www.youtube.com/embed/Po3zGQRSRgE)

[Here is an open-source game project that heavily uses the ECS system](https://github.com/heyx3/Drill8):

[![Drill8](https://img.youtube.com/vi/H5IgtsJcJjw/hqdefault.jpg)](https://www.youtube.com/embed/H5IgtsJcJjw)

An ongoing [voxel toy project](https://github.com/heyx3/BpWorld) of mine:

[![Voxel Toy Project](https://img.youtube.com/vi/fGxGdNAPTSY/hqdefault.jpg)](https://www.youtube.com/embed/fGxGdNAPTSY)

If you're wondering why Julia, [read about it here](docs/!why.md)!

## Structure

To import everything B+ has to offer, just do `using Bplus; @using_bplus`.
All documented modules are available directly inside (e.x. `Bplus.Math`),
  but under the hood this library is segmented into several smaller packages:

* [BplusCore](https://github.com/heyx3/BplusCore) contains utilities and game math.
* [BplusApp](https://github.com/heyx3/BplusApp) provides convenient access to OpenGL, GLFW, and Dear ImGUI.
* [BplusTools](https://github.com/heyx3/BplusTools) provides a variety of other game-development tools, like a scene-graph algorithm, ECS algorithm, and more.

You can, for example, integrate `BplusCore` into your own project to get all the game math,
    without having to use OpenGL, Dear ImGUI, or any of the other parts of the library.

## Tests

Unit tests follow Julia convention: kept in the *test* folder, and centralized in *test/runtests.jl*.
Each sub-package (`BplusCore`, `BplusApp`, and `BplusTools`) runs its own tests,
    and this package runs each of those in sequence.
The test behavior is standardized by `BplusCore`, which provides the path
    to a test-running Julia file through `BplusCore.TEST_RUNNER_PATH`.

I never liked how the official Julia Test package does not let you assign messages to tests,
    so my tests are implemented with my own macros:
  * `@bp_check(condition, msg...)` for normal checks
  * `@bp_check_throws(statement, msg...)` to ensure something causes an error
  * `@bp_test_no_allocations(expr, expected_value, msg...)` to test expressions that need to be completely static and type-stable (game math, scene graph operations, raycast math, etc).
    * A slightly longer version is offered in case you need to set up the test with some operations that may allocate: `@bp_test_no_allocations_setup(setup_expr, test_expr, expected_value, msg...)`.

## Codebase

The three sub-packages are organized into a clear order.
Each one has a set of modules within it.

````
---------  BplusCore  ----------
|                              |
|   Utilities         Math     |
|                              |
--------------------------------
              |
              |
              |
              |
--------------  BplusApp  ---------------
|                                       |
|                  GL                   |
|                /    \                 |
|               /      \                |
|              /        \               |
|           GUI          Input          |
|                                       |
-----------------------------------------
              |
              |
              |
              |
--------------  BplusTools  ---------------
|                                         |
|    ECS      Fields      SceneTree       |
|                                         |
-------------------------------------------
````

To import all B+ sub-modules into your project, you can use this snippet: `using Bplus; @using_bplus`.

Below is a quick overview of the modules. [A thorough description is available here](docs/!home.md). You can also refer to the inline doc-strings, comments, and [sample projects](https://github.com/heyx3/BpExamples).

### `Bplus.Utilities`

A dumping ground for all sorts of convenience functions and macros.

Detailed info can be found [in this document](docs/Utilities.md), but here are the highlights:

* `PRNG`: An alternative to Julia's various `AbstractRNG`'s which is more suitable for aesthetic, short-term use-cases such as procedural generation.
* `@bp_check(condition, error_msg...)`, a run-time test that raises `error()` if the expression is false.
* `@make_toggleable_asserts()` sets up a "debug" vs "release" compile-time flag for your module.
* `@bp_enum` and `@bp_bitflag`: Much better alternatives to Julia's `@enum` and BitFlags' `@bitflag`.
* `SerializedUnion{Union{...}}`: Can serialize *and deserialize* a union of values, using the *StructTypes* package (which means it can be used in major IO packages like *JSON3*).
* `RandIterator{TIdx, TRng}`: Randomly and efficiently iterates through every value in some range `1:n`.
* `UpTo{N, T}`: A type-stable, immutable, stack-allocated container for up to N values of type T. Implements `AbstractVector{T}`.

### `Bplus.Math`

Typical game math stuff, but using Julia's full expressive power.

Detailed info can be found [in this document](docs/Math.md), but here is a high-level overview:

* Vectors are represented with `Vec{N, T}`.
* Quaternions are `Quaternion{F}`.
* Matrices are `@Mat{C, R, F}`.
* Various functions such as `lerp()` and `inv_lerp()`.
* `Box{N, T}` and `Interval{T}`, to represent contiguous spaces.
* `Ray{N, F}`, and a hierarchy of `AbstractShape`
* Noise functions:
  * `perlin(pos)`
* `Contiguous{T}` is an alias for a wide variety of storage for some sequence of `T`. For example, `NTuple{10, Vec{3, Float32}}` is both a `Contiguous{Float32}` and a `Contiguous{Vec{3, Float32}}`.

### `Bplus.GL`

Wraps OpenGL constructs, and provides a centralized place for a graphics context and its accompanying window. This is the big central module of B+ along with `Math`.

Detailed information can be found [in this document](docs/GL.md), but here is a high-level overview:

* `Context` represents the global OpenGL context, uniquely associated with a specific thread and GLFW window.
* Services are singletons attached to the `Context`. Some are built into B+, but you can also define your own using the `@bp_service` macro.
* `AbstractResource` is the type hierarchy for OpenGL objects, like `Texture`, `Buffer`, or `Mesh`.
* Clear the screen with `clear_screen()`. Draw something with `render_mesh()`.

### `Bplus.Input`

Detailed explanation available [here](docs/Input.md).

Provides a greatly-simplified window into GLFW input, as a [GL Context Service](docs/GL.md#Services). Input bindings are defined in a data-driven way that is easily serialized through the `StructTypes` package.

Make sure to update this service every frame with `service_Input_update()`; this is already done for you if you build your game logic within [`@game_loop`](docs/Helpers.md#Game-Loop).


### `Bplus.GUI`

Helps you integrate B+ projects with the Dear ImGUI library, exposed in Julia through the *CImGui.jl* package.

Detailed information can be found in [this document](docs/GUI.md), but here is how you use it at a basic level:

1. Initialize the service with `service_GUI_init()`.
2. Start the frame with `service_GUI_start_frame()`.
3. After starting the frame, use *CImGui* as normal.
4. End the frame with `service_GUI_end_frame()`

If you use [`@game_loop`](docs/Helpers.md#Game-Loop), then this is all handled for you and you can use *CImGui* freely.

To send a texture (or texture View) to the GUI system, you must wrap it with `gui_tex_handle(tex_or_view)`

### `Bplus.ECS`

A very simple entity-component system that's easy to iterate on, loosely based on Unity3D's model. If you want a high-performance, use an external Julia ECS package instead such as *Overseer.jl*.

Detailed information can be found in [this document](docs/ECS.md), but here is a high-level overview:

* A `World` is a list of `Entity` (plus accelerated lookup structures)
* An `Entity` is a list of `AbstractComponent`
* You can add, remove, and query entities for components.
  * You can also query the components across an entire `World`.
* You can define component types with the `@component` macro. Components have a OOP structure to them; abstract parent components can add fields, "promises" (a.k.a. abstract functions) and "configurables" (a.k.a. virtual functions).
  * Refer to the `@component` doc-string for a very detailed explanation of the features and syntax.
* You can update an entire world with `tick_world(world, delta_seconds::Float32)`.

### Helpers

Things that don't neatly fit into the other modules are documented [here](docs/Helpers.md).

### `Bplus.SceneTree`

Provides a memory-layout-agnostic implementation of scene graph algorithms. **Still a work-in-progress**, but the architecture is pretty much done.

Read about this feature [here](docs/SceneTree.md).

### `Bplus.Fields`

A data representation of scalar or vector fields, i.e. functions of some `Vec{N1, F1}` to some `Vec{N2, F2}`. As well as a nifty little language for quickly specifying them.

This is the first of a series of planned modules that will help you play with procedural content generation. For example, you can use a Field to easily fill a texture with interesting noise.

I'm not going to write more about this module for now, but check out the unit tests for examples.

# License
 
Copyright William Manning 2019-2023. Released under the MIT license.


Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
