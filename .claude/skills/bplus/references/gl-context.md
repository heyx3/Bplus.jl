# BplusApp.GL — Context, state, drawing

A thin wrapper over OpenGL 4.6 with the bindless-textures extension, plus management of a context and its GLFW window.

> **All counting and indices in `GL` functions are 1-based**, converted to 0-based under the hood, to stay consistent with the rest of Julia. Instanced rendering of N instances takes `IntervalU(min=1, max=N)`. GPU-side data — index buffers, indirect-draw structures — is still genuinely 0-based, so keep mesh generation 0-based.

## Context

`Context` is a mutable struct: a thread-local singleton bound to one GLFW window. Get the current thread's with `get_context()`.

It owns:

- The `GLFW.Window` and its vsync setting
- `state::RenderState` — every field of which is also exposed as a property of `Context` directly, so `get_context().viewport` works as well as `get_context().state.viewport`
- Which requested OpenGL extensions were actually available (a few bindless-texture ones are requested by default). Query with `extension_supported("GL_ARB_my_extension_name")`
- Attached services
- `device::Device` — GPU limits as reported by OpenGL. Some of these are not reported reliably by every driver; be careful depending on them.
- Currently-bound resources: shader, mesh, UBO slots, SSBO slots
- GLFW callbacks. Core GLFW allows one subscriber per event; `Context` multiplexes so you can add as many as you like. The `Input` and `GUI` services already consume GLFW input, so you rarely need these. Signatures are documented on the relevant `Context` fields.

### Creating one

Managing a Context's lifetime by hand is awkward given Julia's threading model. Prefer, in order:

1. `@game_loop` (see `references/app.md`) — handles the Context and all built-in services.
2. `bp_gl_context(args...) do context::Context … end` — constructs the Context, runs your code, cleans up. The first argument is your code (Julia's `do`-block idiom); everything else forwards to the `Context` constructor.

```julia
bp_gl_context(v2i(800, 500), "My Window",
              vsync=VsyncModes.off,
              debug_mode=true) do context::Context
    # ...
end
```

### Misc

`set_vsync(mode::E_VsyncModes)` / `get_context().vsync` — includes adaptive sync.
`get_window_size()` — the GLFW window size.
`refresh(context)` — forces the Context to re-read driver state after *external* OpenGL calls modified it. Potentially very slow; avoid if at all possible.

## Render state

Every piece of render state has four ways in:

```julia
get_context().cull_mode                        # read
get_context().cull_mode = FaceCullModes.off    # write
set_culling(FaceCullModes.off)                 # write
with_culling(FaceCullModes.off) do … end       # scoped: set, run, restore
```

| Field | Setter | Scoped wrapper |
| --- | --- | --- |
| `color_write_mask::vRGBA{Bool}` | `set_color_writes` | `with_color_writes` |
| `depth_write_mask::Bool` | `set_depth_writes` | `with_depth_writes` |
| `cull_mode::E_FaceCullModes` | `set_culling` | `with_culling` |
| `viewport::Box2Di` | `set_viewport` | `with_viewport` |
| `scissor::Optional{Box2Di}` | `set_scissor` | `with_scissor` |
| `blend_mode::@NamedTuple{rgb::BlendStateRGB, alpha::BlendStateAlpha}` | `set_blending` | `with_blending` |
| `depth_test::E_ValueTests` | `set_depth_test` | `with_depth_test` |
| `stencil_test` | `set_stencil_test`, `_front`, `_back` | `with_stencil_test`, `_front`, `_back` |
| `stencil_result` | `set_stencil_result`, `_front`, `_back` | `with_stencil_result`, `_front`, `_back` |
| `stencil_write_mask` | `set_stencil_write_mask`, `_front`, `_back` | `with_stencil_write_mask`, `_front`, `_back` |
| `render_state::RenderState` (everything at once) | `set_render_state` | `with_render_state` |

Notes:

- **`set_culling`, not `set_cull`** — the upstream `GL.md` misnames this one.
- `viewport` maps NDC (−1…+1 from `gl_Position`) onto a pixel range. `scissor` does *not* change any coordinate math; it just hides pixels outside the box.
- Stencil test/result/write-mask each accept either a single value or `@NamedTuple{front, back}` to use different settings per facing.
- Blend modes: see the `BlendState_` struct for custom ones. Presets: `make_blend_opaque(BlendStateRGB)`, `make_blend_additive(BlendStateAlpha)`, `make_blend_alpha(BlendStateRGBA)`.
- `set_render_state` isn't faster or slower than setting fields individually, just different.

## Clearing the screen

`clear_screen(values...)`:

- Color: a 4D vector of `GLfloat` (`vRGBAf`). If the back buffer is a non-normalized integer format, use `vRGBAi` or `vRGBAu` instead.
- Depth: one `GLfloat` (`Float32`) — e.g. `clear_screen(@f32 1.0)`
- Stencil: one `GLuint` (`UInt32`)
- Hybrid depth-stencil: two arguments, depth as `GLfloat` and stencil as `GLuint`

## Drawing

```julia
render_mesh(m::Mesh, p::Program; args...)
```

| Parameter | Effect |
| --- | --- |
| `shape::E_PrimitiveTypes` | Override the primitive type; defaults to the `Mesh`'s own |
| `indexed_params::Optional{DrawIndexed}` | Indexed drawing; defaults to indexed using all indices if the mesh has them |
| `elements::Union{IntervalU, Vector{IntervalU}, ConstVector{IntervalU}}` | Which elements to draw. A list of ranges means multi-draw. Defaults to everything. |
| `instances::Optional{IntervalU}` | Instanced rendering. Any contiguous interval — it needn't start at 1. |
| `known_vertex_range::Optional{IntervalU}` | Driver hint for the range of vertices your indices reference. Mainly useful with indexed rendering *plus* a custom `elements` range. |

**`elements` as multi-draw, `instances`, and `known_vertex_range` are mutually exclusive.** Using more than one raises an error.

Not implemented: indexed multi-draw with per-subset index offsets (an esoteric GL feature that would require splitting the function into three overloads), and indirect draw calls. If you dispatch indirect draws yourself, either leave the OpenGL state untouched or call `refresh(get_context())` afterward.

## Compute

`dispatch_compute_threads(...)` or `dispatch_compute_groups(...)`. A compute `Program` exposes its declared group size as `program.compute_work_group_size::v3i`.

## Sync

`gl_catch_up_before(actions::E_MemoryActions)` — ensures a future action sees the results of previous ones (e.g. sampling a texture a compute shader just wrote). Only needed for "incoherent" actions OpenGL can't predict; coherent ones like rendering into a target are handled for you. The `MemoryActions` bitflag names the operation you're about to perform, e.g. `MemoryActions.texture_upload_or_download`.

`gl_catch_up_renders_before(actions)` — potentially cheaper variant for actions in fragment shaders touching only part of the Target you just rendered to. The actions passed must be a subset of `MemoryActions.USE_IN_FRAGMENT_SHADERS`.

`gl_flush_texture_writes_in_place()` — call this when ping-ponging *within one texture*, before reading the region you just wrote.

`gl_execute_everything()` — forces all submitted commands to finish now. Rarely useful; two real cases are sharing resources with another context (not natively supported anyway) and chasing a driver bug by calling it after every draw to find the one that crashes.

## Services

A Service is a mutable singleton attached to a Context. Define your own with `@bp_service`; see its doc-string for full detail.

```julia
@bp_service Input(force_unique, lazy) begin
    buttons::Dict{AbstractString, Any}
    current_scroll_pos::v2f

    INIT(a, b) = new(a, b, zero(v2f))
    SHUTDOWN(service, is_context_closing::Bool) = begin
        close(service.some_file_handle)
        if !is_context_closing  # pointless to free GL resources if the context is dying anyway
            for tex in service.all_my_textures; close(tex); end
        end
    end
    REFRESH(service) = begin  # optional: called when the Context refreshes
        service.last_rendered_program = glGetIntegerv(blah, blah)
    end

    get_button(service, name) = service.buttons[name]
end
```

Options in the name's parentheses:

- `force_unique` — one instance per Context. This also removes the service parameter from every generated function, so `f(service, a, b)` is called as `f(a, b)`.
- `lazy` — auto-initialize (with no arguments) on first retrieval.

Generated for a service named `X`:

| Function | Notes |
| --- | --- |
| `service_X_init(args...)` | Runs `INIT`, registers with the Context, returns the service. Callable repeatedly unless `force_unique`. |
| `service_X_shutdown([service])` | Runs `SHUTDOWN(service, is_context_closing)`. Called automatically on context close; call it early to kill the service, in which case the flag is `false`. Omits the argument under `force_unique`. |
| `service_X()` | Gets the instance. Only exists under `force_unique`. |
| `service_X_exists()` | Only exists under `force_unique`. |
| your custom functions | Generated nearly as-is; first argument must be the service. Convention is to name them `service_X_f()`. |

The service name may carry type parameters, in which case you pass them to `new` inside `INIT` and to the `service_X()` getter as normal function parameters. Individual functions, `INIT` included, may also have type parameters.

### Built-in services

| Service | Provided by | Notes |
| --- | --- | --- |
| `Input` | `BplusApp.Input` | Keyboard/mouse/gamepad. Auto-managed by `@game_loop`. |
| `GUI` | `BplusApp.GUI` | Dear ImGUI integration. Auto-managed by `@game_loop`. |
| `BasicGraphics` | `BplusApp` | Screen triangle/quad, blit shader, empty mesh. Auto-managed by `@game_loop`. |
| `SamplerProvider` | internal | Deduplicates sampler objects across textures with identical settings. |
| `ViewDebugging` | internal | Catches *some* uses of bindless handles that were never activated. Only active when `GL` is in debug mode or the Context was started with `debug_mode=true`. |

## Debugging

B+ uses the older OpenGL error-catching form. `pull_gl_logs()` returns every message/error since the last call, in chronological order. The underlying list **drops messages if left unpolled too long**, so call it at least once a frame in debug builds.

`@check_gl_logs(msg...)` is the convenience version: pulls and immediately prints them with file, line number, and your context string. By default it suppresses one very spammy buffer-related driver message.

```julia
@check_gl_logs("after compiling the skybox shader")
```

## Low-level handles

Raw OpenGL handle typedefs live in *GL/handles.jl*, prefixed `Ptr_`: `Ptr_Program`, `Ptr_Texture`, `Ptr_Mesh`, `Ptr_View`, … Not exported by default; you rarely need them.

## Resources

Managed OpenGL objects are `AbstractResource` subtypes: `Program`, `Texture`, `Buffer`, `Mesh`, `Target`, `TargetBuffer`. All implement:

- `get_ogl_handle(resource)`
- `is_destroyed(resource)::Bool`
- `Base.close(resource)` — cleanup, which you must do; these are not GC-managed

Resources cannot be copied, and their fields cannot be set directly — go through the resource's own interface. Details in `references/gl-resources.md`.
