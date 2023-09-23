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

A dumping ground for all sorts of convenience functions and macros.

Detailed info can be found [in this document](docs/Utilities.md). Here are some of the most interesting parts:

* `PRNG`: An alternative to Julia's various `AbstractRNG`'s which is more suitable for aesthetic, short-term use-cases such as procedural generation.
* `@bp_check(condition, error_msg...)`, a run-time test that raises `error()` if the expression is false.
* `@make_toggleable_asserts()` sets up a "debug" vs "release" compile-time flag for your module.
* `@bp_enum` and `@bp_bitflag`: Much better alternatives to Julia's `@enum` and BitFlags' `@bitflag`.
* `SerializedUnion{Union{...}}`: Can serialize *and deserialize* a union of values, using the *StructTypes* package (which means it can be used in major IO packages like *JSON3*).
* `RandIterator{TIdx, TRng}`: Randomly and efficiently iterates through every value in some range `1:n`.
* `UpTo{N, T}`: A type-stable, immutable, stack-allocated container for up to N values of type T. Implements `AbstractVector{T}`.

### Math

Typical game math stuff, but using Julia's full expressive power.

Detailed info can be found [in this document](docs/Math.md). Here are some of the most interesting parts:

* Vectors are represented with `Vec{N, T}`.
* Quaternions are `Quaternion{F}`.
* Matrices are `@Mat{C, R, F}`.
* Various functions such as `lerp()` and `inv_lerp()`.
* `Box{N, T}` and `Interval{T}`, to represent contiguous spaces.
* `Ray{N, F}`, and a hierarchy of `AbstractShape`
* Noise functions:
  * `perlin(pos)`
* `Contiguous{T}` is an alias for a wide variety of storage for some sequence of `T`. For example, `NTuple{10, Vec{3, Float32}}` is both a `Contiguous{Float32}` and a `Contiguous{Vec{3, Float32}}`.

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

Detailed information can be found [in this document](docs/GL.md). Here is a high-level overview:

* `Context` represents the global OpenGL context, uniquely associated with a specific thread and GLFW window.
* Services are singletons attached to the `Context`. Some are built into B+, but you can also define your own using the `@bp_service` macro.
* `AbstractResource` is the type hierarchy for OpenGL objects, like `Texture`, `Buffer`, or `Mesh`.
* Clear the screen with `clear_screen()`. Draw something with `render_mesh()`.

### Input

Provided through a context service (see **GL**>*Services* above). This module greatly simplifies the use of the GLFW package for reading input, and defines bindings in a data-driven way that you could easily serialize/deserialize. Make sure to update this service every frame with `service_Input_update()` (already done for you if you build your game logic within `@game_loop`).

#### Buttons

Buttons are binary inputs. A single button can come from more than one source, in which case they're OR-ed together (e.x. you could bind "fire" to right-click, Enter, and Joystick1->Button4).

Create a button with `create_button(name::AbstractString, inputs::ButtonInput...)`. Refer to the `ButtonInput` class for more info on how to configure your buttons. A button can come from keyboard keys, mouse buttons, or joystick buttons.

Get the current value of a button with `get_button(name::AbstractString)::Bool`.

#### Axes

Axes are continuous inputs, represented with `Float32`. Unlike buttons, they can only come from one source.

Create an axis with `create_axis(name::AbstractString, input::AxisInput)`. Refer to the `AxisInput` class for more info on how to configure your axis. An axis can come from mouse position, scroll wheel, joystick axes, or a list of `ButtonAsAxis`.

Get the current value of an axis with `get_axis(name::AbstractString)::Float32`.

### GUI

This module provides a context service (see **GL**>*Services* above) that manages the Dear ImGUI library through the C package `CImGui`.

Start every frame with `service_GUI_start_frame()`. End a frame with `service_GUI_end_frame()`, which immediately draws the frame's GUI (presumably to the screen, although you could activate a `Target` before calling it if you wanted). In between the "start frame" and "end frame" calls, you can use Dear ImGUI as normal.

If you build your game logic within `@game_loop`, then "start frame" and "end frame" are automatically called before and after your `LOOP` logic respectively.

### Helpers

A dumping ground for useful stuff. Like *Utilities*, but more focused on B+ than on Julia in general.

* `@game_loop`: A macro that makes it easy to run a B+ game. Refer to its doc-string for usage.
* `@using_bplus`: A macro that automatically loads all B+ modules with `using` statements.
* *Cam3D*: A basic customizable 3D camera.
  * State is stored in `Cam3D{F}`
  * Settings are stored in `Cam3D_Settings{F}`
  * Frame inputs are stored in `Cam3D_Input{F}`
  * Update the camera with `(state, settings) = cam_update(state, settings, input, delta_seconds)`.
  * Get view/projection matrices with `cam_view_mat()` and `cam_projection_mat()`.
* *Service_BasicGraphics*: a context service (see **GL**>*Services* above) which provides important, basic stuff. Automatically included for games built with `@game_loop`. ProvidesWE:
  * Triangle and quad meshes which cover the whole screen
  * A blit shader (used with `simple_blit()`)
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
