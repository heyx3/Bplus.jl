# BplusApp.GL — Resources

`Program`, `Buffer`, `Texture`, `Mesh`, `Target`, `TargetBuffer`. All are `AbstractResource`: `get_ogl_handle`, `is_destroyed`, `Base.close`. **You must `close()` them** — nothing is GC-managed. They can't be copied, and their fields can't be set directly.

---

# Program

A compiled shader, for rendering or compute.

## From a string literal

```julia
prog = bp_glsl"""
    // Shared prelude — compiled into every stage.
    // Each stage gets one of IN_VERTEX_SHADER, IN_FRAGMENT_SHADER,
    //                        IN_GEOMETRY_SHADER, IN_COMPUTE_SHADER defined.
#START_VERTEX
    in vec2 vIn_corner;
    void main() { gl_Position = vec4(vIn_corner, 0.5, 1.0); }
#START_FRAGMENT
    out vec4 fOut_color;
    void main() { fOut_color = vec4(1); }
"""
```

Section tokens: `#START_VERTEX`, `#START_FRAGMENT`, `#START_GEOMETRY` (optional), `#START_COMPUTE` (makes it a compute program and forbids all other stages).

For a runtime string, `bp_glsl_str(shader_string)`. Pass `debug_out = stderr` to print the final assembled shaders.

## From constructors

`Program(vert_shader, frag_shader; geom_shader = nothing)` or `Program(compute_shader)`.

`flexible_mode::Bool = true` suppresses errors when you set a uniform the compiler optimized out. Tests often pass `flexible_mode=false` to catch typos.

## From `ProgramCompiler`

For binary caching: construct a `ProgramCompiler`, call `compile_program()`, and on success build the `Program` from its output. If given a cached binary (presumably loaded from disk), it tries that first and falls back to string compilation; after a successful string compile it updates the cache so you can write it out for next launch.

**Cached-binary compilation fails often** — a driver update invalidates it — so always keep the source string as a fallback.

## Uniforms

Inspect a compiled program via `program.uniforms`, `program.uniform_blocks`, `program.storage_blocks`. Read only; never mutate those collections.

Allowed uniform data (the `Uniform` type alias covers all of it):

- Scalars, booleans included (`UniformScalar`)
- Enums (`@enum` or `@bp_enum`), interpreted as their underlying integer type
- Vectors, `UniformVector{1}`…`UniformVector{4}`
- Matrices, `UniformMatrix{2,2}`…`UniformMatrix{4,4}`
- Textures: a `Texture` (uses its default sampler `View`), a `View`, or a raw `Ptr_View`

Setters:

```julia
set_uniform(program, name, value::Uniform[, array_index])   # 1-based index into a uniform array
set_uniforms(program, name, value_list::Vector{<:Uniform}[, dest_offset=0])
set_uniforms(program, name, T::Type{<:Uniform}, contiguous_elements[, dest_offset=0])
```

For `Texture`/`View` elements the second form relaxes to any iterator, not just a `Vector`.

The third form exists for a real ambiguity: a `Vector{v4f}` passed to the second overload infers the uniform type as `vec4`. If the shader actually declares `float[]`, pass `Float32` explicitly as `T` and the `Vector{v4f}` as the `Contiguous{Float32}`.

## Buffer-backed uniforms

Packing must follow std140 (UBOs) or std140/std430 (SSBOs) exactly — see [Packing](#packing) below.

### Uniform Buffers (UBO)

Small, read-only replacements for per-program uniforms. A buffer goes into a global slot; any shader can point a block at that slot.

```julia
set_uniform_block(buffer, slot::Int[, byte_range])   # buffer → global slot
set_uniform_block(program, block_name, slot::Int)    # shader block → slot
set_uniform_block(slot::Int)                         # clear the slot
```

### Storage Buffers (SSBO)

Arbitrarily large, readable *and* writable. Essential for compute shaders.

```julia
set_storage_block(buffer, slot::Int[, byte_range])
set_storage_block(program, block_name, slot::Int)
set_storage_block(slot::Int)
```

---

# Buffer

A struct or array of data on the GPU — mesh data, or shader blocks.

`Buffer(byte_size, can_cpu_change_data_after_creation, recommend_storage_on_cpu=false)` is the base constructor. Others take initial data, or a bitstype whose size is used. **Buffers cannot be resized after creation.**

```julia
buf = Buffer(false, [ v4f(-0.75, -0.75, -0.75, 1.0), v4f(0.75, 0.75, 0.75, 1.0) ])
```

## Reading and writing

```julia
set_buffer_data(buf, ptr::Ref, buffer_byte_range::IntervalU = [all bytes])
set_buffer_data(buf, array::AbstractArray, buffer_first_byte=1)
set_buffer_data(buf, some_bits_data, buffer_first_byte=1)
set_buffer_data(buf, block::AbstractOglBlock, buffer_first_byte=1)   # from @std140 / @std430
set_buffer_data(buf, block_array::BlockArray, buffer_first_byte=1)
```

```julia
get_buffer_data(buf, out_ptr::Ref, buffer_byte_range::IntervalU = [all bytes])
get_buffer_data(buf, out_contiguous_array::AbstractArray, buffer_first_byte=1)
get_buffer_data(buf, T, buffer_first_byte=1)::T
get_buffer_data(buf, (T, size...), buffer_first_byte=1)::Array{T}   # T must be a bitstype
```

For the `get_buffer_data(buf, T)` form, `T` may be:

- a bitstype
- `Vector{F}` for a bitstype `F` — reads as many elements as fit, unless you pass `fixed_element_count=n`
- a `@std140`/`@std430` type
- `StaticBlockArray{N, S[, TMode]}` where `S` is a bitstype or a block type; the mode parameter can be omitted when `S` is itself a block type
- `Vector{T}` for a block type `T`, which behaves specially: it reads raw bytes into a `Vector{UInt8}` (the whole buffer by default) and returns `Vector{T{Base.SubArray{UInt8,…}}}` — each element views part of the original array. The output isn't contiguous POD, but the underlying byte array is; reach it via `block_byte_array(v[1]).parent`.

`copy_buffer(src, dest; ...)` copies GPU-side; keyword arguments select subsets of either side.

## Packing

`@std140` and `@std430` generate a struct whose byte layout exactly matches the GPU's expectations, so the Julia and GLSL definitions can't drift apart. The struct is backed by a mutable byte array, so properties can be *read and written*, and blocks nest.

```julia
@enum(En::UInt32, a, b)

@std140 struct MyInnerUniformBlock
    f::Float32
    bools::vb4
    en::En                                    # UInt32 → 'uint' in the shader
    position_array::StaticBlockArray{12, v3f}
end

@std140 struct MyOuterUniformBlock
    i::Int32                                  # careful: Julia's `Int` is 64-bit
    items::StaticBlockArray{5, MyInnerUniformBlock}
    b::Bool
end

const MY_UBO_DATA = MyOuterUniformBlock(3, ntuple(i -> zero(MyInnerUniformBlock), 5), true)

length(block_byte_array(MY_UBO_DATA))   # total size
block_padding_size(MY_UBO_DATA)         # how much of that is padding

MY_UBO_DATA.i = 1122334455

set_buffer_data(my_ubo, MY_UBO_DATA)
my_ubo_data = get_buffer_data(my_ubo, MyOuterUniformBlock)
```

`@std430` is identical in usage; use it for SSBOs, where it packs more efficiently. UBOs must use std140.

Valid field types: any `Uniform`; a nested struct declared with the same macro; `StaticBlockArray{N, T}`.

The macro rejects `mutable struct` — wrap it in a `Ref` if you need that. Generated structs implement `Random.rand()`, `Random.rand!()`, and `==`, each by recursing into the fields, which makes round-trip tests easy. Row-major matrix annotation is not yet supported. Set the global `BUFFER_LAYOUT_MACRO_DEBUG_STREAM` to an `IO` to dump the generated struct code.

---

# Texture

A 1D, 2D, 3D, or cubemap grid of pixels. B+ uses bindless textures, so shaders receive a `UInt64` handle tied to a particular `View`.

Not supported: texture arrays (unnecessary with bindless) and MSAA textures (not yet implemented).

## Creation

```julia
Texture(format, size; args...)
Texture(format, data::PixelBuffer, data_bgr_ordering::Bool = false; args...)
Texture_cube(format, square_length::Integer; args...)
Texture_cube(format, six_faces_data::PixelBufferD{3}; args...)
Texture(template::Texture, format)   # match an existing texture's shape
```

Dimensionality is inferred from `size` or `data`.

| Keyword | Meaning |
| --- | --- |
| `sampler = TexSampler{N}()` | Default sampler for this texture |
| `n_mips::Integer = [max]` | Pass `1` to disable mip-mapping |
| `depth_stencil_sampling::Optional{E_DepthStencilSources}` | For hybrid formats — only Depth *or* Stencil is readable at a time. Change later with `set_tex_depthstencil_source(tex, v)`. |
| `swizzling` | Reorders components on sample/image-read. Does **not** affect writes or blending. Change later with `set_tex_swizzling(tex, s)`. |

## Sampling

`TexSampler{N}`, where `N` is the texture dimensionality (wrapping is per-axis). Cubemaps have no wrapping or border color, so they use `TexSampler{1}`. All samplers are upscaled to `TexSampler{3}` when stored in the `Texture` for type stability. Parameters are documented in the struct definition. Serializes via *StructTypes* / *JSON3*.

A texture binds one sampler at creation, but you can request a different one per-use through a `View`.

Sampler objects are deduplicated by the internal `SamplerProvider` service — textures with identical settings share one.

## Pixel formats for upload/download

`PixelBuffer` = a Julia array of `N` dimensions (the texture's) and `T` elements, where `T` is a `PixelIOValue`: a scalar `PixelIOComponent` or a 1–4 component vector of them.

To convert *FileIO* image pixels: `convert_pixel(pixel, T)`.

```julia
using FileIO, ImageIO
bplus_pixel_array = convert_pixel.(FileIO.load("MyImage.png"), vRGBu8)
```

Channels are deduced from the pixel type — an array of `v2f` means RG; a single channel defaults to Red. Override with `bgr_ordering=true` (RGB→BGR) and `single_component=PixelIOChannels.green`.

> **Setting fewer channels than the texture has does not leave the rest alone.** OpenGL sets the missing color channels to 0 and a missing alpha to 1.

Hybrid depth-stencil textures use the packed types `Depth24uStencil8u` or `Depth32fStencil8u`, matching the format exactly.

Cubemaps are treated as 3D with Z spanning the six faces.

## Operations

Every operation below takes `subset::TexSubset` (default: the whole texture; 2D and applied per-face for cubemaps).

**Clearing** — `clear_tex_pixels(tex, value, ...)` infers from the format; prefer the specific version when you know it:
`clear_tex_color`, `clear_tex_depth`, `clear_tex_stencil` (the last two not valid for hybrid formats), `clear_tex_depthstencil(tex, depth::Float32, stencil::UInt8, ...)` or `clear_tex_depthstencil(tex, ::Union{Depth24uStencil8u, Depth32fStencil8u}, ...)` (format must match exactly).
Extra keywords: `bgr_ordering` (3/4-channel color), `single_component` (1-channel color; must be `red`/`green`/`blue`), `recompute_mips::Bool = true`.

**Setting** — `set_tex_pixels(tex, pixels::PixelBuffer; args...)`, or `set_tex_color`, `set_tex_depth`, `set_tex_stencil` (also accepts `Vector{Vec{1,UInt8}}`), `set_tex_depthstencil`. The pixel array must have the same dimensionality as the texture. Keywords: `bgr_ordering`, `recompute_mips = true`, `single_component = PixelIOChannels.red`.

**Getting** — `get_tex_color`, `get_tex_depth`, `get_tex_stencil`, `get_tex_depthstencil`, each into an output `PixelBuffer` of matching dimensionality. Keywords: `bgr_ordering`.

**Copying** — `copy_tex_pixels(src, dest, args...)` is a bitwise `memcpy`, no translation. Legal iff both textures have the same bits per pixel — or, between a compressed and an uncompressed texture, iff one uncompressed pixel matches one compressed *block*. Keywords: `src_subset::TexSubset`, `dest_min = 1`, `dest_mip = 1`.

**Metadata** — `get_mip_byte_size(tex, mip_level)`, `get_gpu_byte_size(tex)`.

## Formats

A `TexFormat` is one of:

- `SimpleFormat(::E_FormatTypes, ::E_SimpleFormatComponents, ::E_SimpleFormatBitDepths)` — e.g. `SimpleFormat(FormatTypes.normalized_uint, SimpleFormatComponents.RGBA, SimpleFormatBitDepths.B4)`. Not all combinations are valid; anything under 8 bits per pixel isn't.
- `E_CompressedFormats` — DXT5, BC7, …
- `E_SpecialFormats` — asymmetrical depths (`GL_RGB565`), sRGB (`GL_SRGB_ALPHA8`), esoterica (`GL_RGB9_E5`)
- `E_DepthStencilFormats`

Interface (estimating where an exact answer doesn't exist, e.g. bits-per-pixel of a block-compressed format): `is_supported(format, tex_type)`, `is_color`, `is_depth_only`, `is_stencil_only`, `is_depth_and_stencil`, `is_integer`, `is_signed`, `stores_channel(format, channel)`, `get_n_channels`, `get_pixel_bit_size`, `get_byte_size(format, tex_size)`, `get_ogl_enum`, `get_native_ogl_enum(format, tex_type)`.

## Views

Background: [OpenGL bindless textures](https://www.khronos.org/opengl/wiki/Bindless_Texture).

A View is a `UInt64` handle referring to a texture under some access mode. Because it's just an integer, it can live anywhere — inside a uniform block, a storage block, wherever.

Two kinds:

- **Texture View** — associated with a sampler; lets the shader sample. `get_view(tex, custom_sampler=nothing)`
- **Simple View** (OpenGL's "Image View") — associated with `SimpleViewParams`; reads and/or writes individual pixels of one mip level. Can present a 3D texture or cubemap as 2D by focusing one Z-slice or face. `get_view(tex, p::SimpleViewParams)`

Most functions accepting a `View` also accept a `Texture` and call `get_view` for you.

> **Views must be activated.** The driver cannot see a handle hidden in a `UInt64`, so call `view_activate(view)` before use and `view_deactivate(view)` after, to free room for other textures. Forgetting this is the classic silent failure. Simple views take an extra access-mode parameter, read+write by default.

```julia
view_activate(get_view(tex))
render_mesh(mesh, program)
view_deactivate(get_view(tex))
```

`ViewDebugging` (active in GL debug mode) catches *some* misuses.

**64-bit ints in GLSL:** B+ requests the "shader int64" extension, which adds first-class `int64`/`uint64` instead of stitching two 32-bit uints. It's reportedly poorly supported on integrated GPUs.

---

# Mesh

OpenGL's "Vertex Array Object". A configuration of buffers holding vertex data and optionally index data, plus a default primitive type (overridable at draw time).

Vertex data comes in two parts:

1. **Sources** — `VertexDataSource(buffer, element_byte_size[, byte_offset])`, one per buffer.
2. **Fields / attributes** — `VertexAttribute(source_index, byte_offset_within_element, vs_input[, per_instance])`, pulling one shader input out of a source. `per_instance > 0` means the data advances once every N instances instead of per-vertex.

One buffer of position+normal+uv is one source and three fields.

## Choosing the `VSInput`

The goal is for buffer data to appear in the vertex shader as one of `vec2`, `ivec2`, `uvec3`, `dvec4`, and so on. Full spec in *GL/buffers/vertices.jl*.

1. **Simple vector** of `Float16/32/64`, `UInt8/16/32`, or `Int8/16/32` — pick the matching B+ vector type `V` (UVs as two 32-bit floats → `V = v2f`).
   - Pass straight through (`v2i` → `ivec2`, `v4f` → `vec4`): `VSInput(V)`
   - Integer data cast or normalized into a float vector: **`VSInput_FVector(V, normalized::Bool)`**
2. **Matrix**, 2x2 to 4x4, 32- or 64-bit components: `VSInput(M)`, e.g. `VSInput(fmat2x3)`.
3. **Packed formats:**
   - N-dimensional fixed-point (16 integer bits, 16 decimal) appearing as `vecN`: `VSInput_FVector_Fixed(N)`
   - RGB unsigned-float packed 10B/11G/11R, appearing as `vec3`: `VSInput_FVector_Packed_UF_B10_G11_R11()`
   - RGBA packed as 2-bit alpha + 10 bits each RGB, appearing as `vec4`: `VSInput_FVector_Packed_A2_BGR10(signed::Bool, normalized::Bool)`

> The upstream `Resources.md` example writes `VSInput(vRGBu8, true)`. There is no such overload — the normalized case is `VSInput_FVector(vRGBu8, true)`.

## Example — one buffer per field

```julia
buffer_positions = Buffer(false, [ v4f(-0.75, -0.75, -0.75, 1.0),
                                   v4f(-0.75,  0.75,  0.25, 1.0),
                                   v4f( 0.75,  0.75,  0.75, 1.0),
                                   v4f( 2.75, -0.75, -0.75, 1.0) ])
buffer_colors = Buffer(false, [ vRGBu8(255, 123, 10), vRGBu8(0, 0, 0),
                                vRGBu8(34, 23, 80),   vRGBu8(255, 255, 255) ])
buffer_bones = Buffer(false, UInt8[ 1, 2, 1, 3 ])

mesh = Mesh(
    PrimitiveTypes.triangle_strip,
    [ VertexDataSource(buffer_positions, sizeof(v4f)),
      VertexDataSource(buffer_colors,    sizeof(vRGBu8)),
      VertexDataSource(buffer_bones,     sizeof(UInt8)) ],
    [ VertexAttribute(1, 0x0, VSInput(v4f)),
      VertexAttribute(2, 0x0, VSInput_FVector(vRGBu8, true)),
      VertexAttribute(3, 0x0, VSInput(UInt8)) ]
)
```

```glsl
in vec4 pos;
in vec3 color;
in uint bone;
```

## Example — one interleaved buffer

```julia
struct Vertex
    pos::v4f
    color::vRGBu8
    bone::UInt8
end

buffer = Buffer(false, [ Vertex(v4f(-0.75, -0.75, -0.75, 1.0), vRGBu8(255, 123, 10), 1),
                         # ...
                       ])

mesh = Mesh(
    PrimitiveTypes.triangle_strip,
    [ VertexDataSource(buffer, sizeof(Vertex)) ],
    [ VertexAttribute(1, 0x0,                            VSInput(v4f)),
      VertexAttribute(1, sizeof(v4f),                    VSInput_FVector(vRGBu8, true)),
      VertexAttribute(1, sizeof(v4f) + sizeof(vRGBu8),   VSInput(UInt8)) ]
)
```

## Index data

Pass a `MeshIndexData` (a buffer plus an index type of `UInt8`/`UInt16`/`UInt32`) at construction, or afterwards with `set_index_data(mesh, buffer, type)`. Remove it with `remove_index_data(mesh)`.

`GL.count_mesh_vertices(mesh)` reports the vertex count.

---

# Target

OpenGL's "FrameBuffer Object". A set of textures to draw into instead of the screen.

One fragment-shader output per color attachment; at most one depth, stencil, or depth-stencil attachment. If depth/stencil is only needed for render operations and never sampled, the Target can allocate a lightweight `TargetBuffer` instead of a real `Texture`.

Attachments may be a slice of a 3D texture, a face of a cubemap, or "layered" — the whole 3D texture or cubemap at once, with the geometry shader choosing a layer per primitive.

## Construction

Existing textures you want to attach get wrapped in `TargetOutput`, which is also how you express slices, faces, and layered attachments. Whenever you give settings rather than a texture, the Target creates the texture and destroys it with itself.

```julia
Target(size::v2u, n_pretend_layers::Int = 1)
Target(size::v2u, color::TexFormat, depth_stencil::E_DepthStencilFormats)
Target(color::Union{TargetOutput, Vector{TargetOutput}},
       depth::Union{Nothing, E_DepthStencilFormats, TargetOutput})
Target(depth::TargetOutput)
Target(color::TargetOutput, depth_stencil::E_DepthStencilFormats,
       ds_no_sampling::Bool = true,
       ds_sampling_mode = DepthStencilSources.depth)
```

The first form has no real attachments but reports the given layer count. In the last, the depth/stencil output is non-sampleable by default; mark it sampleable and the fourth argument picks what can be read.

Reach an attachment's texture with `target.attachment_colors[i].tex`.

## Use

```julia
target_activate(target)          # subsequent rendering goes here
target_clear(target, vRGBAf(1, 0, 1, 0))
target_clear(target, @f32 1.0)
# ... draw ...
target_activate(nothing)         # back to the screen
```

`target_activate(target_or_nothing, reset_viewport=true, reset_scissor=true)`.

`target_clear(target, value, color_fragment_slot=1)` clears one attachment:

- Color: `vRGBAf` for float/normalized textures, `vRGBAu` for uint, `vRGBAi` for int — plus the slot index.
- Depth: any float (`Float16`/`Float32`/`Float64`).
- Stencil: `UInt8`, or another unsigned int that gets cast down. Affected by the stencil write mask; by default it throws if any mask bits are off, to catch surprises. Pass `false` to disable that check.
- Hybrid: a `Depth32fStencil8u`, *regardless* of the actual format.

`target_configure_fragment_outputs(target, slots)` routes fragment-shader outputs to color attachments. `[5, nothing, 1]` sends output 1 → attachment 5, discards output 2, sends output 3 → attachment 1, and discards anything beyond. Targets start as `[1, 2, 3, …]`.

> After calling it, the index in `target_clear` is a **fragment-shader output index**, not an attachment index. With `[5, nothing, 1]`, clearing slot 1 clears attachment 5.
