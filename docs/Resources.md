Part of the [`GL` module](GL.md#Resources). Represents a managed OpenGL object.

All resource types implement the following interface (see doc-strings for more details):

* `get_ogl_handle(resource)`
* `is_destroyed(resource)::Bool`
* `Base.close(resource)` to clean it up

# Program

`Program` is a compiled shader, either for rendering or compute.

### From string literal

You can compile a shader from a static string literal with Julia's macro string syntax: `bp_glsl" [multiline shader code here] "`. Within the string, you can define mutiple sections:

* Code at the top of the string will be re-used for all shader stages.
  * A specific token will be defined based on which shader stage you are in: `IN_VERTEX_SHADER`, `IN_FRAGMENT_SHADER`, `IN_GEOMETRY_SHADER`, `IN_COMPUTE_SHADER`.
* Code following the token `#START_VERTEX` is used as the vertex shader.
* Code following the token `#START_FRAGMENT` is used as the fragment shader.
* Code following the token `#START_GEOMETRY` (optional) is used as the geometry shader.
* Code following the token `#START_COMPUTE` makes this Program into a compute shader, forbidding all other shader types.

For dynamic strings, you can call `bp_glsl_str(shader_string)` to get the same result.

### From simple compilation

For a little more control, use the constructor `Program(vert_shader, frag_shader; geom_shader = nothing)`, or `Program(compute_shader)`.

It also takes an optional named parameter `flexible_mode::Bool` (defaulting to `true`), which indicates whether it should suppress errors when setting uniforms that have been optimized out.

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

Clear a Uniform Buffer global slot with `set_uniform_block(slot::Int)`.

#### Storage Buffers

Called "Shader Storage Buffer Objects" or "SSBO" in OpenGL. These are arbitrarily-large arrays which you can both read and write to. They are very important for compute shaders.

For small, read-only data, use [Uniform Buffers](#Uniform-Buffers) instead.

Assign a buffer to a global storage slot with `set_storage_block(buffer, slot::Int[, byte_range])`. By default, the buffer's full set of bytes is used for the uniform block. You can pass a custom range if the shader data starts at a certain byte offset, or should be limited to a subset of the full range.

Point a shader's storage block to that global slot with `set_storage_block(program, block_name, slot::Int)`.

Clear a Storage Buffer global slot with `set_storage_block(slot::Int)`.

# Buffer

A struct or array of data, stored on the GPU. It is often used for mesh data, but can also be used in shaders.

If using them in shaders, make sure to check the [Packing section](#Packing) below.

To create a buffer, just call one of its constructors. You can pass an explicit byte-size, or a set of initial data which is immediately uploaded to the buffer. You cannot resize a buffer after creating it.

## Data

To set a buffer's data (or a subset of it), call `set_buffer_data(buffer, data::Vector{T}; ...)`. See the function definition for info on the various parameters.

To get a buffer's data (or a subset of it), call `get_buffer_data(buffer, output=UInt8; ...)`. You can provide an array to write into, or merely the type of data to have the function allocate a new array for you. See the function definition for info on the various parameters.

To copy one buffer's data to another, call `copy_buffer(src, dest; ...)`. You can use the optional named parameters to pick subsets of the source or destination buffer.

## Packing

When using buffers in shaders (see [Buffer Data](#Buffer-Data) above), it's important to make sure the data is packed properly. OpenGL allows the "std140" packing standard for Uniform Blocks ("UBO"), and either "std140" or the more efficient "std430" for Storage Blocks ("SSBO").

These formats can be tricky to pack correctly. So B+ provides two macros, `@std140` and `@std430`, which set up a struct's data to be padded exactly the way it should be on the GPU! This helps you ensure the Julia version of a struct matches the shader version exactly.

See the doc-strings for these macros for help using them. Valid field types are:
    * Any `Uniform` (see [Uniforms](#Uniforms) above)
    * A nested struct that was also defined with the same macro (`@std140` or `@std430`).
    * An `NTuple` of the above data, representing a static-sized array.

# Texture

A 1D, 2D, 3D, or "Cubemap" grid of pixels. B+ uses the Bindless Textures extension, so textures are mostly passed to shaders as simple integer handles, associated with a particular `View` on that texture.

Texture arrays are not needed in a renderer with bindless textures, so they aren't supported.

MSAA textures are not yet implemented.

## Sampling

Textures can be associated with sampler objects, of type `TexSampler{N}`. `N` is the dimensionality of the texture; useful because wrapping behavior can be specified per-axis. Cubemap textures are special -- they don't have "wrapping" behavior or a border color, so B+ uses `TexSampler{1}` for them. All samplers are upscaled to `TexSampler{3}` when stored in the `Texture` resource for type-stability.

A texture is associated with a specific sampler setting on creation, but you can easily request a different sampler when passing the texture to a shader (see [Views](#Views) below).

The different parameters you can pass to a `TexSampler` are specified clearly in the struct's definition, so refer to it for more information.

`TexSampler` supports serialization with the *StructTypes.jl* package (and therefore the *JSON3* package).

### SamplerProvider service

Sampler objects can be re-used across textures, so there's no need for many of them. There is a built-in service that provides samplers, `SamplerProvider`, that textures use internally. You shouldn't have to use it, but if you're making bare OpenGL calls it may be useful to have.

## Creation

Create a 1D, 2D, or 3D texture with `Texture(format, size; args...)` or `Texture(format, data::PixelBuffer, data_bgr_ordering::Bool = false; args...)`.
  * The dimensionality is inferred from the dimensionality of `size` or `data`.

Create a cubemap texture with `Texture_cube(format, square_length::Integer; args...)` or `Texture(format, six_faces_data::PixelBufferD{3}; args...)`. For more info on how cubemaps are specified, see [Cubemaps](#Cubemaps) below.

The optional arguments to the constructor are as follows:
  * `sampler = TexSampler{N}()` is the default sampler associated with the N-dimensional texture.
  * `n_mips::Integer = [maximum]` is the number of mips the texture has. Pass `1` to effectively disable mip-mapping.
  * `depth_stencil_sampling::Optional{E_DepthStencilSources}` controls what can be sampled/read from a texture with a depth-stencil hybrid format. Only one of Depth or Stencil can be read at a time.
    * This can be changed after construction with `set_tex_depthstencil_source(tex, new_value)`.
  * `swizzling` can switch the components of the texture when it's being sampled or read from an Image view. Does *not* affect writes to the texture, or blending operations when rendering to the texture.
    * This can be changed after construction with `set_tex_swizzling(tex, new_swizzle)`.


## Upload/Download Format

You can use almost any kind of basic scalar/vector data to upload or download a texture's pixels. The set of allowed pixel array types is captured by the type alias `PixelBuffer`, which is a Julia array of `N` dimensions and `T` elements. `N` is the texture dimensionality and `T` is the pixel data type. `T` must come from the type alias `PixelIOValue`, which is either a scalar `PixelIOComponent` or a vector of 1 to 4 such components.

For color textures, the components to work with are usually deduced from the type of the pixel data.
For example, an array of `v2f` is assumed to be working with the RG components.
If uploading/downloading a single channel from the texture, it defaults to the Red channel.
  * Change deduced channels from *RGB* to *BGR* by passing `bgr_ordering=true` into texture operations.
  * Change the single-channel from *Red* to *Green* or *Blue* by passing e.x. `single_component=PixelIOChannels.green` into texture operations.

**Important Note**: when setting fewer channels than actually exist in a color texture (e.x. only Green in an RG texture, or only RG in an RGBA texture), OpenGL sets missing color channels to 0 and the missing Alpha channel to 1. They are not left unchanged as you might expect.

For hybrid depth-stencil textures, you can use the special packed pixel types `Depth24uStencil8u` or `Depth32fStencil8u` depending on the specific format. These can be directly uploaded into their corresponding textures.

Cubemap textures are treated as 3D, where the Z coordinate represents the 6 faces.

## Operations

### Clearing

To generically clear a texture without knowing the format type, use `clear_tex_pixels(tex, value, ...)` and it will be inferred from the texture format and provided pixel value.

It's recommended to use one of the more specific functions when you know the format:
  * `clear_tex_color(tex, color::PixelIOValue, ...)`
  * `clear_tex_depth(tex, depth::PixelIOComponent, ...)`
    * Can't be used for hybrid depth-stencil textures
  * `clear_tex_stencil(tex, stencil::UInt8, ...)`
    * Can't be used for hybrid depth-stencil textures
  * `clear_tex_depthstencil(tex, depth::Float32, stencil::UInt8, ...)`
  * `clear_tex_depthstencil(tex, value::Union{Depth24uStencil8u, Depth32fStencil8u}, ...)`
    * The pixel format must precisely match the texture format.

These functions have the following optional arguments:
  * `subset::TexSubset = [entire texture]`. For cubemap textures, this subset is 2D, and cleared on each desired face.
  * `bgr_ordering::Bool = false` (only for 3- and 4-channel color) should be true if data is specified as BGR instead of RGB (faster for upload in many circumstances).
  * `single_component::E_PixelIOChannels` (only for 1-component color) controls which color channel is being set, if only one channel is provided. Must be `red`, `green`, or `blue`, not any of the multi-channel enum values.
  * `recompute_mips::Bool = true` if true, automatically computes mips after clearing.

### Setting pixels

When setting a texture's pixels, you must provide an array of the same dimensionality as the texture you're setting.

For more info on pixel data formats, see [*Upload/Download Format* above](#UploadDownload-Format).

To generically set a texture's pixels without knowing the format type, use `set_tex_pixels(tex, pixels::PixelBuffer; args...)` and Julia will infer it from the texture format and provided pixel format.

**Important Note**: when setting fewer channels than actually exist in a color texture (e.x. only Green in an RG texture, or only RG in an RGBA texture), OpenGL sets missing color channels to 0 and the missing Alpha channel to 1. They are not left unchanged as you might expect.

It's recommended to use one of the more specific functions when you know the format:

* `set_tex_color(tex, color::PixelBuffer, ...)`
* `set_tex_depth(tex, depth::PixelBuffer, ...)`
  * Can't be used for hybrid depth-stencil textures
* `set_tex_stencil(tex, stencil::PixelBuffer{UInt8}, ...)`
  * Can't be used for hybrid depth-stencil textures
  * Also accepts a `Vector{Vec{1, UInt8}}`
* `set_tex_depthstencil(tex, value::PixelBuffer{<:Union{Depth24uStencil8u, Depth32fStencil8u}}, ...)`
  * The pixel format must precisely match the texture format.

These functions have the following optional arguments:

* `subset::TexSubset = [entire texture]`. For cubemap textures, this subset is set on each desired face.
* `bgr_ordering::Bool = false` (only for 3- and 4-channel color) should be true if data is specified as BGR instead of RGB (faster for upload in many circumstances).
* `recompute_mips::Bool = true` if true, automatically computes mips afterwards.
* `single_component::E_PixelIOChannels = PixelIOChannels.red` : which color channel is getting set. Only relevant if you are passing scalar values to a color texture.

### Getting pixels

When getting a texture's pixels, you must provide an output array of the same dimensionality as the texture you're setting. Cubemap textures are considered 3D, with Z spanning the 6 faces.

For more info on pixel data formats, see [*Upload/Download Format* above](#UploadDownload-Format).

It's recommended to use one of the more specific functions when you know the format:

* `get_tex_color(tex, out_colors::PixelBuffer, ...)`
* `get_tex_depth(tex, out_depth::PixelBuffer, ...)`
  * Can't be used for hybrid depth-stencil textures
* `get_tex_stencil(tex, out_stencil::PixelBuffer{UInt8}, ...)`
  * Can't be used for hybrid depth-stencil textures
  * Also accepts a `Vector{Vec{1, UInt8}}`
* `get_tex_depthstencil(tex, out_hybrid::PixelBuffer{<:Union{Depth24uStencil8u, Depth32fStencil8u}}, ...)`
  * The pixel format must match the texture format.

These functions have the following optional named arguments:

* `subset::TexSubset = [entire texture]`. For cubemap textures, this subset is set on each desired face.
* `bgr_ordering::Bool = false` (only for 3- and 4-channel color) should be true if data is specified as BGR instead of RGB (faster for download in many circumstances).

### Copying

You can directly copy the bits of one texture's pixels to another texture with `copy_tex_pixels(src, dest, args...)`.
This is comparable to a `memcpy()`, meaning that the data is copied over without translation.
It is allowed if and only if the source and destination texture have the same bit size per-pixel.

When copying between a compressed and uncompressed texture, the requirement is a bit different: the bit size of one pixel of the uncompressed texture should match the bit size of one *block* of the compressed texture.

Optional arguments are as follows:

* `src_subset::TexSubset` : picks a subset of the source texture.
* `dest_min = 1` : picks a min corner of the destination texture to copy to.
* `dest_mip = 1` : picks a mip level of the destination texture to copy to.

### Other

Query a texture's metadata with:
  * `get_mip_byte_size(tex, mip_level::Integer)`
  * `get_gpu_byte_size(tex)`

Update a texture's global sampling settings (regardless of sampler object) with:
  * `set_tex_swizzling(tex, new_swizzle)`
  * `set_tex_depthstencil_source(tex, new_source)`

For information on how to deal with bindless texture views, see [Views](#Views) below.


## Cubemaps

The precise specification for cubemap pixel data in OpenGL is annoying to keep in mind. So B+ defines some data in *GL/textures/cube.jl* to encode this for you:

* The enum `E_CubeFaces` lists all 6 faces in their memory order. For example, `CubeFaces.from_index(1)` gets the first face, which is `CubeFaces.pos_x`.
* The six faces and their orientations are specified in `CUBEMAP_MEMORY_LAYOUT`, which is a tuple of 6 `CubeFaceOrientation`.
* `CubeFaceOrientation` has a lot of useful data, plus helper functions. For example:
  * The UV.x axis on the +Y face is `CUBEMAP_MEMORY_LAYOUT[CubeFaces.to_index(CubeFaces.pos_y)].horz_axis`.
  * The UV coordinate `v2f(0.85, 0.2)` on the -Z face corresponds to the cubemap vector `get_cube_dir(CUBEMAP_MEMORY_LAYOUT[CubeFaces.to_index(CubeFaces.neg_z)], v2f(0.85, 0.2))`.
  * The first pixel on the +X face corresponds to the cubemap corner `CUBEMAP_MEMORY_LAYOUT[CubeFaces.to_index(CubeFaces.pos_x)].min_corner`.

## Format

Texture formats have a very clear and unambiguous specification in B+. A `TexFormat` is one of:

* `SimpleFormat`, a struct combining a `E_FormatTypes`, a `E_SimpleFormatComponents`, and a `E_SimpleFormatBitDepths`.
  * For example, 4-bit RGBA is `SimpleFormat(FormatTypes.normalized_uint, SimpleFormatComponents.RGBA, SimpleFormatBitDepths.B4)`.
  * Some combinations of these are not valid, for example any formats that amount to less than 8 bits per pixel.
* `E_CompressedFormats` for block-compressed formats like DXT5 or BC7.
* `E_SpecialFormats` for all the special cases:
  * Asymmetrical bit depths, like `GL_RGB565`
  * sRGB formats, like `GL_SRGB_ALPHA8`
  * Esoteric formats, like `GL_RGB9_E5`
* `E_DepthStencilFormats` for depth, stencil, and depth-stencil textures.

`TexFormat` implements the following interface. In cases where there isn't an exact answer (e.x. getting the bits per pixel of a block-compressed format), an estimation is provided.
* `is_supported(format, tex_type)`
* `is_color(format)`
* `is_depth_only(format)`, `is_stencil_only(format)`, and `is_depth_and_stencil(format)`
* `is_integer(format)`, and `is_signed(format)`
* `stores_channel(format, channel)`
* `get_n_channels(format)`
* `get_pixel_bit_size(format)`
* `get_byte_size(format, tex_size)`
* `get_ogl_enum(format)`
* `get_native_ogl_enum(format, tex_type)`

## Views

*NOTE*: To better understand this topic, refer to [outside resources on OpenGL bindless textures](https://www.khronos.org/opengl/wiki/Bindless_Texture).

With the Bindless Textures extension to OpenGL, textures in shaders are accessed in a much simpler way from how they were in traditional OpenGL. A texture is now just a UInt64 handle representing the texture under a certain "View". This texture can be passed around like any other UInt64, for example inside a Uniform Block or Storage Block.

A View on a texture is either associated with a sampler, allowing you to sample from the texture (what OpenGL calls a "Texture View"), or it is a "simple view" associated with a `SimpleViewParams`. Simple Views (what OpenGL calls an "Image View") only allow for reading and/or writing individual pixels of a specific mip level. It can also make a 3D or cubemap texture appear 2D by focusing on a single Z-slice or face, respectively.

Views for a texture are retrieved with `get_view(tex, custom_sampler=nothing)` and `get_view(tex, p::SimpleViewParams)`. Most functions which accept a `View` will also accept a `Texture` and automatically call `get_view(tex)` as needed.

**Important Note**: The GPU driver cannot predict when bindless textures are used, since they can be hidden anywhere as a plain int64, so you must manually tell it to "activate" a view before you can use it, with `view_activate()`. Then when you are done, you should "deactivate" the view to make room for other textures, with `view_deactivate()`.
  * Simple views take an extra parameter, which is the access mode of the view. By default it is both read and write, but you can create a read-only or write-only view.

### 64-bit Integers in Shaders

Normally GLSL doesn't have 64-bit int types and so you need to stitch together two 32-bit uints; however, B+ also tries to add the "shader int64" extension which adds int64 and uint64 as first-class data types. Unfortunately this extension apparently isn't supported well on integrated GPU's.


# Mesh

Called a "Vertex Array Object" or "VAO" in OpenGL. A `Mesh` is a configuration of buffers reprsenting vertex data and optionally index data. It also has a default "primitive type" such as `triangle_strip`, but this can easily be overridden when rendering the mesh.

## Vertex Data

Vertex data is specified in two parts:

1. "Sources" name a list of buffers, as instances of `VertexDataSource`. Each buffer represents an array of vertex data, with some byte offset and an element byte size.
2. "Fields" or "Attributes" pull specific data from the listed sources. Represented as an instance of `VertexAttribute`.

For example, if you have one buffer which lists each vertex as position+normal+uv, then you'll have one "source" and three "fields".

### Vertex Fields

Fields are more complex than sources, as there are many ways to pull data out of a buffer for use in the vertex shader. `VertexAttribute` contains the following data:

* The source buffer index
* The byte offset from each element of the buffer to the field being extracted
* The type of the buffer data, and the way it should be interpreted in the vertex shader (see below).
* A "per_instance" number, for instanced rendering. If greater than 0, this data is per every N instances rather than per-vertex.

The full specification of the vertex data type is in *GL/buffers/vertices.jl*, but here is the gist:

Whatever data format is in our buffer, the goal is to make it appear in the vertex shader as one of a few standard data types -- `vec2`, `ivec2`, `uvec3`, `dvec4`, etc.

1. If your buffer data is a simple 1D to 4D vector of `Float16`, `Float32`, `Float64`, `UInt8`, `UInt16`, `UInt32`, `Int8`, `Int16`, or `Int32`: start by picking the B+ vector type matching your buffer's data type; we'll call it `V`. For example, if you're specifying UV coordinates which come in as 2 32-bit floats, use `V=v2f`.
   1. If the data should pass directly through to the shader (e.x. `v2i` showing up as `ivec2`, or `v4f` showing up as `vec4`), then simply use `VSInput(V)`.
   2. If the data is integer and should be casted or normalized into float, then use `VSInput_FVector(V, normalized::Bool)`.
2. If your buffer data is a *matrix* of size 2x2 up to 4x4, with 32-bit or 64-bit components, then pick the matching B+ matrix type `M` and use `VSInput(M)`. For example, `VSInput(fmat2x3)`.
3. If your buffer data is one of these weird types, use the corresponding special field type:
   1. For an `N`-dimensional vector of fixed-point decimals (16 bits for integer, and 16 for decimal), which should appear as `vecN` in the vertex shader, use `VSInput_FVector_Fixed(N).`
   2. For RGB unsigned-float data packed into 10B11G11, which appears in the shader as `vec3`, use `VSInput_FVector_Packed_UF_B10_G11_R11()`.
   3. For RGBA data packed into 2-bit Alpha and 10-bits each for RGB, which may or may not be signed, and which appears in the shader as `vec4` through either casting or normalization, use `VSInput_FVector_Packed_A2_BGR10(signed::Bool, normalized::Bool)`.

## Example

Putting it all together, here is an example of a mesh made of two triangles in a strip, using one buffer per field:

````
buffer_positions = Buffer(false, [ v4f(-0.75, -0.75, -0.75, 1.0),
                                   v4f(-0.75, 0.75, 0.25, 1.0),
                                   v4f(0.75, 0.75, 0.75, 1.0),
                                   v4f(2.75, -0.75, -0.75, 1.0) ])
buffer_colors = Buffer(false, [ vRGBu8(255, 123, 10),
                                vRGBu8(0, 0, 0),
                                vRGBu8(34, 23, 80),
                                vRGBu8(255, 255, 255) ])
buffer_bones = Buffer(false, UInt8[ 1, 2, 1, 3 ])
mesh = Mesh(
    PrimitiveTypes.triangle_strip,
    [
        VertexDataSource(buffer_positions, sizeof(v4f)),
        VertexDataSource(buffer_colors, sizeof(vRGBu8)),
        VertexDataSource(buffer_bones, sozeif(UInt8))
    ],
    [
        VertexAttribute(1, 0x0, VSInput(v4f)),
        VertexAttribute(2, 0x0, VSInput(vRGBu8, true)),
        VertexAttribute(3, 0x0, VSInput(UInt8))
    ]
)

...

// [in shader]
in vec4 pos;
in vec3 color;
in uint bone;
````

And here is that same example, using one buffer which keeps all fields together:


````
struct Vertex
    pos::v4f
    color::vRGBu8
    bone::UInt8
end

...

buffer = Buffer(false, [
    Vertex(v4f(-0.75, -0.75, -0.75, 1.0),
           vRGBu8(255, 123, 10),
           1),
    Vertex(v4f(-0.75, 0.75, 0.25, 1.0),
           vRGBu8(0, 0, 0),
           2),
    Vertex(v4f(0.75, 0.75, 0.75, 1.0),
           vRGBu8(34, 23, 80),
           1),
    Vertex(v4f(2.75, -0.75, -0.75, 1.0),
           vRGBu8(255, 255, 255),
           3)
])
mesh = Mesh(
    PrimitiveTypes.triangle_strip,
    [ VertexDataSource(buffer, sizeof(Vertex)), ],
    [
        VertexAttribute(1, 0x0, VSInput(v4f)),
        VertexAttribute(1, sizeof(v4f), VSInput(vRGBu8, true)),
        VertexAttribute(1, sizeof(v4f) + sizeof(vRGBu8), VSInput(UInt8))
    ]
)

...

// [in shader]
in vec4 pos;
in vec3 color;
in uint bone;
````

## Index Data

If the mesh should be able to use indexed rendering, provide a `MeshIndexData` on construction. This struct points to a specific buffer and index type (`UInt8`, `UInt16`, or `UInt32`).

To enable or change a mesh's index data buffer after creation, use `set_index_data(mesh, buffer, type)`.

To remove a mesh's index data buffer, use `remove_index_data(mesh)`.

# Target

What OpenGL calls a "FrameBuffer Object" or "FBO". A collection of textures you can draw into instead of drawing into the screen.

For each color output, the fragment shader can write to one output variable. There can only be one depth, stencil, or depth-stencil texture. If you don't need the depth/stencil texture, you can omit it. If you only need depth/stencil for render operations and never for sampling, the `Target` can set up a limited `TargetBuffer` in place of a real `Texture`.

Targets can output to particular slices of a 3D texture or faces of a cubemap texture. The texture attachment can also be "layered", meaning ALL slices/faces are available and the geometry shader can emit different primitives to different layers as it pleases.

## Creation

There are several ways to construct a `Target`. Whenever you provide settings rather than an explicit texture, the `Target` will create a texture for you, and will remember to destroy it when the `Target` is destroyed.

Existing textures that you want to attach should be wrapped in the `TargetOutput` struct, which allows you to easily bind more exotic things to act like 2D attachments, such as slices of a 3D texture or faces of a cubemap texture. You can also bind an entire 3D texture or cubemap, and decide which slice/face to output to in the geometry shader. OpenGL refers to this as "layered" rendering.

* `Target(size::v2u, n_pretend_layes::Int)` creates an instance with no actual outputs, but which pretends to have the given number of outputs.
* `Target(color::Union{TargetOutput, Vector{TargetOutput}}, depth_stencil::TargetOutput)` creates an instance with zero or more color attachments, and the given depth/stencil attachment.
* `Target(depth_stencil::TargetOutput)` creates an instance with no color attachments, only a depth and/or stencil attachment.
* `Target(color::TargetOutput, depth_stencil::E_DepthStencilFormats, ds_no_sampling::Bool = true, ds_sampling_mode = DepthStencilSources.depth)` creates a target with one color output, and one depth/stencil output which by default is not sampleable (meaning, it's not a `Texture`). If you *do* mark it sampleable, the fourth parameter controls what can be sampled from it.

## Use

* `target_activate(target_or_nothing, reset_viewport=true, reset_scissor=true)`
  * Sets all future rendering to go into the given target, or to the screen if `nothing` is passed.
* `target_clear(target, value, color_fragment_slot=1)` will clear one of the target's attachments.
  * For color, pass a value of `vRGBAf` for float or normalized (u)int textures, `vRGBAu` for uint textures, and `vRGBAi` for int textures.
    * When clearing color you must also specify the index. You can think of this as the color attachment index at first. However, if you call `target_configure_fragment_outputs()`, then note that the index here is really the fragment shader output index.
  * For depth, pass any float value (`Float16` `Float32`, `Float64`).
  * For stencil, pass a `UInt8`, or any other unsigned int which will get casted down.
    * This operation is affected by the stencil write mask, and by default will try to catch unexpected behavior by throwing an error if any bits of the mask are turned off. This check can be disabled by passing `false`.
  * For hybrid depth-stencil, pass a `Depth32fStencil8u`, *regardless* of the actual format used.
* `target_configure_fragment_outputs(target, slots)` routes each fragment shader output when rendering to this target, to one of the color attachments (or `nothing` to discard that output). For example, passing `[5, nothing, 1]` means that the fragment shader's first output goes to color attachment 5, its second output is discarded, its third output goes to color attachment 1, and any subsequent outputs are also discarded.
  * When the `Target` is first created, all attachments are active and in order (as if you passed `[1, 2, 3, ...]`).
  * When clearing a Target's color attachments, you provide an index in terms of these fragment shader outputs, *not* in terms of color attachment. For example, after passing `[5, nothing, 1]`, then clearing slot 1 actually means clearing attachment 5.