Part of the [GL module](GL.md#Resources). Represents a managed OpenGL object.

All resource types implement the following interface (see doc-strings for more details):

* `get_ogl_handle(resource)`
* `is_destroyed(resource)::Bool`
* `Base.close(resource)` to clean it up

# Program

`Program` is a compiled shader.

## Creation

### From string literal

You can compile a shader from a static string literal with Julia's macro string syntax: `bp_glsl" [multiline shader code here] "`. Within the string, you can define mutiple sections:

* Code at the top of the string will be re-used for all shader stages.
  * A specific token will be defined based on which shader stage you are in: `IN_VERTEX_SHADER`, `IN_FRAGMENT_SHADER`, or `IN_GEOMETRY_SHADER`.
* Code following the token `#START_VERTEX` is used as the vertex shader.
* Code following the token `#START_FRAGMENT` is used as the fragment shader.
* Code following the token `#START_GEOMETRY` (optional) is used as the geometry shader.

### From simple compilation

For non-static shaders, loaded from a file or otherwise generated dynamically, the simplest way to compile them is to use the constructor `Program(vert_shader, frag_shader; geom_shader = nothing)`.
    * It also takes an optional named parameter `flexible_mode::Bool` (defaulting to `true`), which indicates whether it should suppress errors when setting uniforms that have been optimized out.

### From advanced compilation

For more advanced features, you can use `ProgramCompiler`. Construct an instance of it, try compiling with `compile_program()`, then if that succeeds, construct the `Program` with the output of `compile_program()`. For more details, see the doc-strings for these types and functions.

The main purpose of `ProgramCompiler` is that it can keep a cached pre-compiled version of the shader (presumably loaded from disk). If the cache is given, it tries to use that to compile the program first, only falling back to string compilation if it fails. On the other side of things, after successfully compiling from a string, the binary cache is updated so that you can write it out to disk for the next time the program is started.

*NOTE*: Compilation from a cached binary fails very often, for example after a driver update, so you can never rely on it. You should always provide the string shader as a fallback.

## Uniforms

Uniforms are parameters given to the shader. Lots of data types can be given as uniforms.

You can see the uniforms in a compiled program with `program.uniforms`, `program.uniform_blocks`, and `program.storage_blocks`, but do not modify those collections directly!

### Basic data

Basic uniform data can be individual variables (e.x. `uniform vec3 myPos;`), or arrays of variables (e.x. `uniform vec4 lightPositions[10];`). Either way, the following types of data are supported:

* Scalars, including booleans (specified by the type alias `UniformScalar`).
* Vectors (specified by the type alias `UniformVector{1}` through `UniformVector{4}`).
* Matrices (specified by the type alias `UniformMatrix{2, 2}` through `UniformMatrix{4, 4}`).
* Textures, in several forms:
  * The `Texture` resource itself (in which case its default sampler `View` is used).
  * A `View` of a texture resource.
  * A raw handle to a view, of type `Ptr_View`.
  * For more info on texture views, see [Texture](Resources.md#Texture) below.

The type alias `Uniform` explicitly captures all of these.

Uniforms can be set with the following functions:

* `set_uniform(program, name, value::Uniform[, array_index])` sets a single uniform within a program. If that uniform is part of an array, you should provide the 1-based index.
* `set_uniforms(program, name, value_list::Vector{<:Uniform}[, dest_offset=0])` sets an array of uniforms given a `Vector` of their values. Optionally starts with a uniform partway through the array, using `dest_offset`.
  * If the elements are instances of `Texture` or `View`, then the rules are relaxed  bit: you can provide any iterator of them, not just a `Vector{Texture}` or `Vector{View}`.
* `set_uniforms(program, name, T::Type{<:Uniform}, contiguous_elements[, dest_offset=0])` sets an array of `T`-type uniforms to the data in some [`Contiguous{T}`](Math.md#Contiguous). This is useful if, for example, you want to set an array of floats using a `Vector{v4f}`. The previous overload of `set_uniforms()` would infer the uniform type to be `vec4` instead of `float`, causing problems.

### Buffer data

Additionally, you can provide [buffer data](Resources.md#Buffer) to the shader in one of two forms, explained below.

**Important note**: The packing of buffer data into one a shader block follows very specific rules, most commonly "std140" and "std430". See the [Buffer section](Resources.md#Buffer) for some helpers to pack data correctly.

#### Uniform Buffers

Called "Uniform Buffer Objects" or "UBO" in OpenGL. These are replacements for individually setting uniforms per-program. A Uniform Buffer is assigned to a global slot, then any and all shaders can reference that slot to set a block of uniforms using the buffer's data.

They are meant to be relatively small and read-only. For larger data, or data that can also be written to, use [Storage Buffers](#Storage-Buffers) instead.

Assign a buffer to a global uniform slot with `set_uniform_block(buffer, slot::Int[, byte_range])`. By default, the buffer's full set of bytes is used for the uniform block. You can pass a custom range if the uniform data starts at a certain byte offset, or should be limited to a subset of the full range.

Point a shader's uniform block to that global slot with `set_uniform_block(program, block_name, slot::Int)`.

#### Storage Buffers

Called "Shader Storage Buffer Objects" or "SSBO" in OpenGL. These are arbitrarily-large arrays which you can both read and write to. They are very important for compute shaders.

For small, read-only data, use [Uniform Buffers](#Uniform-Buffers) instead.

Assign a buffer to a global storage slot with `set_storage_block(buffer, slot::Int[, byte_range])`. By default, the buffer's full set of bytes is used for the uniform block. You can pass a custom range if the shader data starts at a certain byte offset, or should be limited to a subset of the full range.

Point a shader's storage block to that global slot with `set_storage_block(program, block_name, slot::Int)`.

# Texture

# Buffer

# Mesh

# Target

## TargetBuffer


#TODO: Finish