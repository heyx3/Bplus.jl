# B+

A game and 3D graphics framework for Julia, using OpenGL 4.6 + bindless textures.

Offers game math, a GL/windowing wrapper, a GLFW input wrapper, scene graph algorithms, and numerous other modules.

## Setup/Dependencies

After downloading the repo, run *scripts/setup.jl*. The directory or Julia project you run from doesn't matter.

The weirdest dependency is "[ModernGLbp](https://github.com/heyx3/ModernGL.jl)", my own fork of [ModernGL](https://github.com/JuliaGL/ModernGL.jl). It adds the necessary components for `ARB_bindless_texture`. It's added to this repo as a git submodule (and this is all handled within *setup.jl*).

## Tests

Unit tests follow Julia convention: kept in the *test* folder, and centralized in *test/runtests.jl*. A short-hand for correctly invoking this script is in *scripts/tests.jl*, so you can most easily run tests with `julia path/to/B-plus/test/tests.jl`.

I never liked how the official Julia Test package does not let you assign messages to tests, so my tests are implemented with my own macros: `@bp_check(condition, msg...)` for normal checks, and a special macro `@bp_test_no_allocations(expr, expected_value, msg...)` to test expressions that need to be completely static and type-stable (game math, scene graph operations, raycast math, etc). A slightly longer version of the last macro is offered in case you need to set up the test with some operations that may allocate: `@bp_test_no_allocations_setup(setup_expr, test_expr, expected_value, msg...)`.

The tests are divided into many individual files. There are also some sample test environments that aren't included in *test/* because they're not automated: *scripts/demo-cam3D.jl* and *scripts/demo-gui.jl* (which is not yet functional).

## Codebase

The code is organized into a number of modules, hierarchically. From lowest-level to highest:

````
---------------
|  Utilities  |
---------------
         |
         |
         |
  ----------------------
  |        Math        |
  ----------------------
    |            |     \
    |            |      \
    |            |       \
--------------- ------  -----------
|  SceneTree  | | GL |  |  Fields  |
--------------- ------  ------------
                / |  \
               /  |   \
              /   |    \
             /    |     \
-------------  -------  -----------
|  Helpers  |  | GUI |  |  Input  |
-------------  -------  -----------
````

Below is a quick overview. A thorough description will be written up elsewhere. You can also refer to the inline doc-strings, comments, and [sample projects](github.com/heyx3/BpExamples).

### Utilities

A dumping ground for all sorts of convenience functions and macros. Here are some of the most interesting ones:

* `PRNG`: An alternative to Julia's various `AbstractRNG`'s which is more suitable for aesthetic, short-term use-cases such as procedural generation (where you might spin up one PRNG for every pixel). An immutable version `ConstPRNG` is also provided, as mutable structs are almost always heap-allocated and therefore bad for tight loops.
* `@bp_check(condition, error_msg...)`, a run-time test that raises `error()` if the expression is false. Is **not** an assert; it always runs.
* `@bp_enum`: A wrapper around Julia's `@enum` that hides the enum values in a submodule, and also provides extra conveniences like parsing.
* `SerializedUnion{Union{...}}`: Can serialize *and deserialize* a union of values through StructTypes/JSON3. Use the macro `@SerializedUnion(T1, T2, ...)` as short-hand.
* `RandIterator{TIdx, TRng}`: Randomly and efficiently iterates through every value in some range `1:n`. Each value will appear exactly once.
* `UpTo{N, T}`: A type-stable, immutable (i.e. stack-allocated) container for up to N values of type T. Implements `AbstractVector{T}`.
* Some neat helper macros that allow you to individually configure each B+ sub=module into a "debug" or "release" mode, mainly to enable/disable asserts respectively. Heavily based on the "ToggleableAsserts" package. You can also set this up in your own project; refer to `@make_toggleable_asserts`
* A number of metaprogramming helpers in *macros.jl*

### Math

Typical game math stuff.

#### Functions

`lerp()`, `inv_lerp()`, `smoothstep()`, `smootherstep()`, `type[min|max]_finite()`, `solve_quadratic()`, `pad_i()`, `square()`.

#### Vec{N, T}

Vectors! Not to be confused with Julia's `Vector` which is a resizable array (much like C++'s `std::vector`). `Vec` can be allocated on the stack, but in Juli this also means they're immutable. So the only way you can "modify" them is to replace them with a copy. Julia's built-in "Setfield" package exists to make this sort of workflow easier.

Below is a lot of the basic info you'll need, but there are more details which you can check out in the file itself.

Numerous convenient aliases exist for `Vec`:
* `Vec2{T}` through `Vec4{T}` to hard-code the dimensionality
* `VecF{N}` (float32), `VecD{N}` (float64), `VecI{N}` (int32), `VecU{N}` (uint32), and `VecB{N}` (bool) to hard-code the component type
* `VecT{T, N}` flips the order of the type parameters, to more easily specify a component with unknown dimensionality
* Short-hand for specific type + size. For example: `v2f`, `v4i`, `v3b`
* Similar short-hand in terms of color. For example: `vRGBu8`, `vRGBAf`

Operations are implemented component-wise (`+`, `-`, `*`, `/`, `&`, `|`, `<`, `>`, `>=`, `<=`). The notable exception is equality and inequality (`==`, `!=`), which return a lone `Bool`.

The easiest way to constuct a Vec is by passing the elements: `Vec(x, y, ...)`. If you want to ensure they're casted to a particular value, specify at least the component type: `VecT{Float32}(-1, 2.5)`. You can also construct it like an ntuple, using a lambda and count: `Vec(i->i*2, 3)` (or `Vec(i->i*2, Val(3))` for compile-time-known constants). Finally, you can append vectors and individual values together with `vappend`: `vappend(Vec(1, 2), 3, Vec(4, 5, 6)) == Vec(1, 2, 3, 4, 5, 6)`.

The actual `Vec` struct has a `data` field which is a tuple of all the values. However, you can also swizzle the values however you want with `xyzw` and `rgba`: `v.x`, `v.xyx`, `v.xxxzw`, `v.rrra`, etc.

You can also use special swizzling characters; `1` to output 1 for that channel, `0` to output 0, `Δ` (\\delta) to output the maximum finite value for the vector's type, and `∇` (\\del) to ouptut the minimum finite value for the vector's type. For example, to set a color value's alpha to 1, you could use `v.rgbΔ` if it's `vRGBAu8`, or `v.rgb1` if it's `vRGBAf`.

`Vec` implements `AbstractVector`, meaning you can index it and otherwise treat it like a 1D array. Most of the built-in Julia array operations are overloaded to return a `Vec` intead of an array. For example, you can do component-wise equality with `map(==, v1, v2)`.

Standard vector operations are prefixed with a v: `vlength`, `vcross`, `vdot`, `vdist`, etc. Additionally, the operator \\times (`×`) is defined to mean `vcross`, and both \\cdot (`⋅`) and \\circ (`∘`) are defined to mean `vdot`.

You can represent ranges of integer coordinates with Julia's standard colon syntax. For example, `collect(v2i(1, 2) : v2i(3, 3))` yields `[ v2i(1, 2), v2i(2, 2), v2i(3, 2), v2i(1, 3), v2i(2, 3), v2i(3, 3)]`.

Broadcasting does **not** currently have custom behavior; you can use it but you'll get a `Vector{T}` out of it rather than a `Vec{N, T}`.

#### Quaternion{F}

The best way to do 3D rotations. Most functions using quaternions are prefixed with `q_`: `q_apply` (to rotate a `Vec3`), `q_slerp`, `q_is_identity`, `q_axisangle`, etc. Refer to the doc-strings for each function, or the source file *quat.jl*.

Aliases exist for float32 and float64 quaternions: `fquat` and `dquat`, respectively.

#### Mat{C, R, F, Len}

A matrix. Really an alias for Julia's `StaticArrays.SMatrix`, a.k.a. an immutable stack-allocatable 2D array.

Unfortunately, due to Julia's syntax rules, the type parameters for `Mat` must include both the row and column count, and the total number of elements (row * column). For convenience, use the `@Mat(C, R, F)` macro which auto-inserts the element count.

Like `Vec`, there are many aliases for `Mat`:
* Hard-coded component types: `MatF{C, R, Len}` for float32 and `MatD{C, R, Len}` for float64
* Hard-coded dimensions: `Mat3{F}` for 3x3 and `Mat4{F}` for 4x4.
* Hard-coded everything: `fmat2x2`, `fmat4x3`, `dmat3`, etc.

Like `Vec` and `Quaternion`, matrix functions are prefixed with `m_`. `m_apply_point`, `m_apply_vector`, `m_combine`, `m_identity`.

Numerous functions exist for computing particular matrices: `m3_translate`, `m4_translate`, `m_scale`, `m3_rotate`, `m4_world`, `m4_look_at`, `m4_projection`, `m4_ortho`.

#### Ray{N, F}

A N-dimensional ray using components of type F. `ray_at(r, t)` gets a position along the ray. `closest_point(r, p)` gets the closest point on a ray (pass `min_t=-Inf` to solve it for a line instead of a ray).

For raycasting, see **Shapes** below.

#### Noise

`perlin(f::Union{Number Vec})` implements any N-dimensional perlin noise. See the optional arguments for ways to customize it (such as making values wrap around a boundary).

Other noise functions are still to be implemented.

#### Contiguous{T}

An alias for a wide variety of storage of a sequence of `T` that is contiguous in memory. For example, `NTuple{4, v3f}` is a `Contiguous{Float32}`, and also a `Contiguous{Vec{3, Float32}}`.

This is useful for interop; for example, you can upload any `Contiguous` data to a texture's pixels.

Unfortunately, due to Julia language limitations, the contiguous data can only be nested to a certain depth (because each possible nesting has to be listed explicitly). Currently it can be nested 4 layers deep, which should be plenty.

`contiguous_length(c, T)` gets the number of elements.  For example, the contiguous length of an `NTuple{4, v3f}`, for type `Float32`, is 12. While the contiguous length of that ntuple for type `v3f` is 4.

#### AbstractShape{N, F}

Some kind of geometry that can collide with other geometry, or be intersected by rays.

This code is still a WIP, but currently offers the following interface:

* `volume(s)`
* `bounds(s)::Box`
* `center(s)::Vec`
* `is_touching(s, p::Vec)::Bool`
* `closest_point(s, p::Vec)::Vec`
* `intersections(s, r::Ray; ...)::UpTo{M, F}`. Can also return the surface normal at the first intersection if you pass a special flag (and only for 3D shapes).

The most important shape is `Box{N, F}`. Other shapes are:

* `Sphere`
* `Capsule`
* `Plane`
* `Triangle`

#### Box{N, F}

An axis-aligned rectangular region. Along with participating in the above `AbstractShape` interface, it provides lots of functionality for use as a bounding box.

You can construct it by providing some combination of two fields among `min`, `max`, `size`, and `center`, as a named tuple. For example, `Box((min=Vec(1, 1), size=Vec(10, 10)))`. The `min` and `max` values are inclusive.

You can get the information about a box with the functions `min_inclusive()`, `min_exclusive()`, `max_inclusive()`, `max_exclusive()`, `size()`, and `center()`.

Many aliases are provided, in the same vein as `Vec` and `Mat`'s aliases.

Other important functions:
* `boundary()`: get the bounds of a group of boxes and points.
* `intersect()`: get the intersection of two or morme boxes.
* `union()`: get the union of two or more boxes.

#### Interval{F}

A `Box{1}` can also be thought of as an interval on the number line. However, working with `Vec{1}` instances is cumbersome. You can instead use the helper type `Interval{F}`, which is like a `Box{1, F}` without the need to wrap the numbers in a `Vec{1, F}`.

For example, you can construct an interval with `Interval((min=1, size=10))`. Whereas with a box you would need to do `Box((min=Vec(1), size=Vec(10)))`.

### SceneTree

Provides a memory-layout-agnostic implementation of scene graph algorithms. **Still a work-in-progress**, but the architecture is pretty much done.

#### Defining your memory layout

To use this module, you should first decide how you store your nodes. You can see examples of this in the unit test file *test/scene-tree.jl*. In particular, you need:
* A unique ID for each node. Call this data type `ID`.
* [Optional] a "context" that can retrieve the node for an ID. Call this type `TContext`. If you don't need a context to retrieve nodes, use `Nothing`.
* A custom node type which somehow owns an instance of the immutable struct `SceneTree.Node{ID, F}`. You should pick a specific `F` (and, of course, an `ID`). Call this node type `TNode`.

#### Implementing the memory layout interface

Once you have an architecture in mind, you can integrate it with `Bplus.SceneTree` by implementing the following interface with your types:

* `null_node_id(::Type{ID})::ID`: a special ID value representing 'null'.
* `is_null_id(id::ID)::Bool`: by default, compares with `null_node_id(ID)` using `===` (triple-equals, a.k.a. egality).
* `deref_node(id::ID[, context::TContext])::TNode`: gets the node with the given ID. Assume the ID is not null, and the node exists.
* `update_node(id::ID[, context::TContext], new_value::SceneTree.Node{ID})`: should replace your node's associated `SceneTree.Node` with the given new value.
* `on_rooted(id::ID[, context::TContext])` and/or `on_uprooted([same args])` to be notified when a node loses its parent (a.k.a. becomes a "root") or gains a parent (a.k.a. becomes "uprooted"). This is useful if you have a special way of storing root nodes separate from child nodes.

#### Examples

A really simple and dumb representation would be to put each node in a list, assume nodes are never deleted, and use each node's index in the list as its ID. Let's also say the component type used is Float32. In this case, you can define the following architecture:
* `const MyID = Int`
* `const MyContext = Vector{SceneTree.Node{MyID, Float32}}`
* `null_node_id(::Type{MyID}) = 0`
  * This is type piracy, a bad practice in Julia, so in a real scenario maybe wrap your integer ID's in a custom struct.
* `deref_node(i::MyID, context::MyContext) = context[i]`
* `update_node(i::Int, context::Vector, value::SceneTree.Node) = (context[i] = value)`.

Now your list of nodes can participate in a scene graph!

Another example would be to make a custom mutable struct, `MyEntity`, which has the field `node::SceneTree.Node{ID, Float32}` (plus whatever else you want your entity to have, like a list of game components). Then `ID` is simply a reference to `MyEntity` (mutable structs are reference types), and no context is needed! You could simply do `deref_node(id::MyEntity) = id.node`. However, to handle null ID's, you'll want to wrap the ID type in Julia's `Ref` type, which boxes it and allows for null values. So the final implementation looks like:
* `const MyID = Ref{MyEntity}`
* `deref_node(id::MyID) = id[].node`
* `update_node(id::MyID, data::SceneTree.Node) = (id[].node = data)`
* `null_node_id(::Type{MyID}) = MyID()`
* `is_null_id(::Type{MyID})`

NOTE: in practice, the last example is not easy to implement in Julia. The problem is that the `ID` type references `MyEntity`, but `MyEntity` also has to reference the `ID` type (in its `node` field). Check out the unit tests, which handle this by adding an extra layer of indirection via type parameters.

#### Usage

Refer to function definitions for more info on the optional params within each function.

Creating nodes and adding them to the scene graph is something you do yourself, since you defined the memory architecture.

Once you have some nodes, you can parent them to each other with `set_parent(child::ID, parent::ID[, context]; ...)`. To unparent a node, pass a 'null' ID for the parent.

You can get the transform matrix of a node with `local_transform(id[, context])` and `world_transform(id[, context])`. The "world transform" means the transform in terms of the root of the tree.

Functions to *set* transform, or get individual position/rotation/scale values, are still to be implemented.

You can iterate over different parts of a tree:
* `siblings(id[, context], include_self=true)` gets an iterator over a node's siblings in order.
* `children(id[, context])` gets an iterator over a node's direct children (no grand-children).
* `parents(id[, context])` gets an iterator over a node's parent, grand-parent, etc. The last element will be a root node.
* `family(id[, context], include_self=true)` gets an iterator over the entire tree underneath a node. The iteration is Depth-First, and this is the most efficient iteration if you don't care about order.
* `family_breadth_first(id[, context], include_self=true)` gets a breadth-first iterator over the entire tree underneath a node. This implementation avoids any heap allocations, at the cost of some processing overhead. It's recommended for small trees.
* `family_breadth_first_deep(id[, context], include_self=true)` gets a breadth-first iterator over the entire tree underneath a node. This implementation makes heap allocations, but iterates more efficiently than the other breadth-first implementation. It's recommended for large trees.

Some other helper functions:
* `try_deref_node(id[, context])` gets a node, or `nothing` if the ID is null.
* `is_deep_child_of(parent_id, child_id[, context])` gets whether a node appears somewhere underneath another node.

### Fields

A data representation of scalar or vector fields, i.e. functions of some `Vec{N1, F1}` to some `Vec{N2, F2}`. As well as a nifty little language for quickly specifying them.

This is the first of a series of planned modules that will help you play with procedural content generation. For example, you can use a Field to easily fill a texture with interesting noise.

I'm not going to write more about this module for now, but check out the unit tests for examples.

### GL

Wraps OpenGL constructs, and provides a centralized place for a graphics context and its accompanying window. This is the big central module of B+ along with `Math`.

#### Context

The core object tying everything together is `Context`, representing a single OpenGL context and GLFW window. Your entire program logic will likely live within a call to `bplus_gl_context()`, whose first argument is a lambda to execute between creating and destroying the context.

By the rules of OpenGL, contexts are a thread-local singleton. So for as long as the context is alive, and from anywhere *within the thread that's running the context*, you can get the current context object with `Bplus.GL.get_context()::Context`. Most `GL` functions don't require you to explicitly provide the context, since they can retrieve it themselves.

The context manages all sorts of rendering state, such as viewports/scissoring, blend mode, depth and stencil modes, and the current render target. Most OpenGL global parameters are controlled by the context, via setter functions and/or special properties defined in the `RenderState` struct. For example, you could call `set_viewport(min::v2i, max::v2i)`, or you could do `get_context().viewport = (min=my_min, max=my_max)`.

The context also provides hooks into common GLFW callbacks. For example, to be notified when the window size changes, do:

````
push!(get_context.glfw_callbacks_window_resized, new_size::v2i -> begin
    # [your code here]
end)
````

#### Device

Hardware/OpenGL queried constants are available through `get_context().device::Device`. For example, `get_context().device.gpu_name` gets the name of the GPU being used for this program (a good way to double-check if you're stuck on the integrated card!)

#### Resources

A `Resource` is some kind of OpenGL object: texture, buffer, VAO, FBO, etc. You can get its OpenGL handle with `get_ogl_handle(r)`. You can clean it up with `close(r)`, and check if it's been cleaned up already with `is_destroyed(r)`.

A full overview of resource types will be documented elsewhere, but here is a quick reference of the major ones:
* `Texture`, defined in *GL/textures/texture.jl*
  * Remember that B+ uses bindless textures, and textures are always given to shaders through a `View`, which must be activated with `view_activate(v)` before rendering.
* `Target`, a.k.a. "framebuffers" in OpenGL, defined in *GL/targets/target.jl*
* `Buffer`, defined in *GL/buffers/buffer.jl*
* `Mesh`, a.k.a. "VAO" in OpenGL, defined in *GL/buffers/mesh.jl*
  * The types of data that can be passed from mesh/buffer into vertex shader are laid out by the type hierarchy defined in *GL/buffers/vertices.jl*
* `Program`, a.k.a. shaders, defined in *GL/program.jl*
  * The string macro `bp_glsl"[shader code here]"` can be used to create a shader program from a string literal.

#### Clearing and Drawing

You can clear a render target (or the screen, by passing null) with `render_clear(context, target::GL.Ptr_Target, value[, target_attachment_idx])`. The value should be one of the following types:
* `vRGBA`, to clear color buffers (regardless of the buffer's actual components).
* `GLfloat`, a.k.a. `Float32`, to clear depth buffers
* `GLint`, a.k.a. `Int32`, to clear stencil buffers
* Two values, `GLfloat` then `GLint`, to clear depth-stencil buffers

You can draw a mesh using a given shader program with `render_mesh([context, ]mesh::Mesh, program::Program; args...)`. There are *many* ways to customize this draw call -- indexing, instancing, multi-draw... Refer to the function's documentation for more info.

#### Services

Because the GL context is a sort of singleton, it allows you to register global services with it. Several services are provided by B+; use them as a reference if writing your own.

* `ViewDebuggerService` : double-checks that you're not using a texture view in a shader without activating it. Automatically included if Bplus.GL is configured for debugging (i.e. you've redefined `Bplus.GL.gl_aserts_enabled() = true`).
* `InputService` : provides access to `Bplus.Input` (see **Input** below)
* `GuiService` : provides access to `Bplus.GUI` (see **GUI** below)
* `BasicGraphicsService` : provides access to various common rendering resources (see **Helpers** below)

### Input

Provided through a context service (see **GL**>*Services* above). This module greatly simplifies the use of the GLFW package for reading input, and defines bindings in a data-driven way that you could easily serialize/deserialize. Opt into the service with `service_input_init([context])`. Update the service every frame with `service_input_update([service])`.

#### Buttons

Buttons are binary inputs. A single button can come from more than one source, in which case they're OR-ed together (e.x. you could bind "fire" to right-click, Enter, and Joystick1->Button4).

Create a button with `create_button(name::AbstractString, inputs::ButtonInput...)`. Refer to the `ButtonInput` class for more info on how to configure your buttons. A button can come from keyboard keys, mouse buttons, or joystick buttons.

Get the current value of a button with `get_button(name::AbstractString[, service])::Bool`.

#### Axes

Axes are continuous inputs, represented with `Float32`. Unlike buttons, they can only come from one source.

Create an axis with `create_axis(name::AbstractString, input::AxisInput)`. Refer to the `AxisInput` class for more info on how to configure your axis. An axis can come from mouse position, scroll wheel, joystick axes, or a list of `ButtonAsAxis`.

Get the current value of an axis with `get_axis(name::AbstractString[, service])::Float32`.

### GUI

This module provides a context service (see **GL**>*Services* above) that manages the Dear ImGUI library (exposed though the Julia package `CImGui`).

Opt into the service with `service_gui_init([context])`. Start a frame with     `service_gui_start_frame([context])`. End a frame with `service_gui_end_frame([context])`, which immediately draws the frame's GUI (make sure it draws to the screen and not some render target!).

In between the "start frame" and "end frame" calls, you can use Dear ImGUI as normal.

### Helpers

A dumping ground for useful stuff. Like *Utilities*, but more focused on B+ than on Julia in general.

* *Cam3D*: A basic customizable 3D camera.
  * State is stored in `Cam3D{F}`
  * Settings are stored in `Cam3D_Settings{F}`
  * Frame inputs are stored in `Cam3D_Input{F}`
  * Call `cam_update(state, settings, input, delta_seconds)` to tick and compute updated state/settings
* *BasicGraphicsService*: a context service (see **GL**>*Services* above) which provides important, basic stuff. Such as:
  * Triangle and quad meshes which cover the whole screen
  * A blit shader (used with `resources_blit()`)
  * An empty mesh, for dispatching draw calls with entirely procedural geometry.

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
