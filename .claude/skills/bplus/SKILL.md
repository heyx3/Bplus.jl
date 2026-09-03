---
name: bplus
description: Working guide to B+ (Bplus), the Julia game/3D framework built on OpenGL 4.6 with bindless textures — its game math (Vec/Quaternion/Mat/Box), OpenGL wrapper (Context, Program, Texture, Buffer, Mesh, Target), GLFW input, Dear ImGUI integration, ECS, SceneTree, and Fields. Use this whenever writing or reading Julia that imports Bplus, BplusCore, BplusApp, or BplusTools; whenever the code sits in a repo depending on any of those packages; and whenever you see the identifiers @using_bplus, @game_loop, bp_gl_context, v2f/v3f/v4f/v2i/v3u, fquat, @Mat/fmat4, Box2Df/IntervalU, @bp_check, @bp_enum, @bp_bitflag, @bp_service, @std140, @std430, @component, bp_glsl, render_mesh, target_activate, view_activate, service_GUI_*, or service_Input_*. Check here before assuming a Julia/OpenGL idiom carries over — B+ is 1-based where OpenGL is 0-based, its Vec is not a StaticArrays SVector, and most of its API is prefixed by convention (v/m_/q_/gui_/service_).
---

# B+ (Bplus)

A game and 3D graphics framework for Julia: OpenGL 4.6 + bindless textures, game math, GLFW, Dear ImGUI, ECS, and procedural-generation tools. MIT licensed, by William Manning.

`] add Bplus`, then `using Bplus; @using_bplus` pulls in every module.

## Package layout

Three stacked packages, plus an umbrella `Bplus` that re-exports all of them:

```
BplusCore    Utilities, Math                    (no OpenGL — usable standalone)
   ↓
BplusApp     GL → {GUI, Input}, ModernGLbp
   ↓
BplusTools   ECS, Fields, SceneTree, Cam3D, FileCacher
```

Depend on `BplusCore` alone if you only want the math. Each package has its own `@using_bplus_core` / `@using_bplus_app` / `@using_bplus_tools` macro. Modules are also reachable as `Bplus.Math`, `Bplus.GL`, etc.

## Reference files

Read the one that matches the task; don't read them all.

| File | Covers |
| --- | --- |
| `references/math.md` | `Vec`, `Quaternion`, `Mat`, `Box`/`Interval`, rays, shapes, curves, `perlin`, `Contiguous` |
| `references/utilities.md` | asserts, `Optional`, `@bp_enum`/`@bp_bitflag`, `PRNG`/`ConstPRNG`, `UpTo`, iterators, flood fill, metaprogramming |
| `references/gl-context.md` | `Context`, render state, services, clearing, `render_mesh`, compute, sync, GL debugging |
| `references/gl-resources.md` | `Program`/shaders/uniforms, `Buffer` + `@std140`/`@std430`, `Texture`/views/formats, `Mesh`, `Target` |
| `references/app.md` | `@game_loop`, `BasicGraphics`, `Input`, `GUI` (Dear ImGUI), `Cam3D`, `FileCacher` |
| `references/tools.md` | ECS (`@component`), `SceneTree`, `Fields` |

## Naming conventions — use them to guess an API before searching

B+ is prefix-heavy and consistent. If you want a function and don't know its name, the prefix narrows it fast:

- `v…` — vector math: `vdot`, `vcross`, `vnorm`, `vlength`, `vdist`, `vreflect`, `vselect`, `vappend`, `vsize`, `vindex`, `vequal`, `vbasis`.
- `m_…` / `m3_…` / `m4_…` — matrix ops and constructors: `m_combine`, `m_invert`, `m_apply_point`, `m4_world`, `m4_look_at`, `m4_projection`.
- `q_…` — quaternions: `q_apply`, `q_slerp`, `q_axisangle`, `q_mat4x4`, `q_basis`.
- `set_X(…)` + `with_X(…) do … end` — every piece of GL render state comes as this pair; the `with_` form restores the old value afterward.
- `service_X_init/_shutdown/_exists/_update` — Context services.
- `gui_…` — Dear ImGUI helpers, most taking a `do` block.
- `@bp_…` — framework macros (`@bp_check`, `@bp_enum`, `@bp_bitflag`, `@bp_service`).
- Type aliases compress dimension + component type: `v3f` = `Vec3{Float32}`, `v2i` = `Vec2{Int32}`, `v4u`, `vRGBAu8`, `fquat`, `fmat4x4`, `Box2Df`, `IntervalU`.

## The things that actually bite

These are the mistakes worth checking for in any B+ code you read or write.

**GL indices are 1-based.** The whole `GL` module counts from 1 and converts to 0 under the hood. Instanced rendering of N instances is `IntervalU(min=1, size=N)`, not `min=0`. But GPU-side data — index buffers, indirect-draw structs — is still genuinely 0-based, so mesh generation code has to stay 0-based. This is the single most common source of off-by-one here.

**`Vec` indexing into a Julia array is reversed.** `arr[v]` uses X as the *row*. For pixel-grid ordering, wrap it: `a[TrueOrdering(v)] == a[reverse(v)]`. Same for `vsize(arr)` — pass `true_order=true` to get the intuitive order.

**`==` on `Vec` returns one `Bool`, not a `Vec` of bools.** Use `vequal(a, b)` for component-wise. Every other operator *is* component-wise.

**Broadcasting a `Vec` gives you a `Vector`.** `Vec <: AbstractVector`, so `a .+ b` returns a heap-allocated 1D array, not a `Vec`. `map()` is overloaded to return a `Vec`; broadcast is not. Avoid `.` syntax on `Vec` in hot paths.

**`@set!` on a nested field copies the owner.** `@set! my_obj.v.x += 10` copies all of `my_obj`, not just `my_obj.v`. Pull the vector into a local first when the owner is large.

**Compile-time flags are functions, never `const`s.** `@make_toggleable_asserts(prefix_)` and `Math.get_up_axis()` work because Julia inlines a trivial function and recompiles callers when you redefine it. A `const C = @my_debug(100, 1000)` is computed once and never updates. Never bind a redefinable function's result to a `const`.

**Bindless texture views must be activated.** The driver can't see a `UInt64` handle hidden in a buffer, so `view_activate(view)` before use and `view_deactivate(view)` after. Forgetting this is a silent black-texture or crash. (`simple_blit` manages this for you by default.)

**`Int` in Julia is 64-bit.** In `@std140`/`@std430` structs you almost always want `Int32`/`UInt32` to match GLSL `int`/`uint`.

**Uploading fewer channels than the texture has zeroes the rest.** Setting only Green on an RG texture sets Red to 0; missing Alpha becomes 1. OpenGL does not leave them unchanged.

**Three `render_mesh` parameters are mutually exclusive:** multi-draw via `elements`, `instances`, and the `known_vertex_range` hint. Using more than one is an error.

**Resources are not GC-managed.** `Program`, `Texture`, `Buffer`, `Mesh`, `Target` all need `close()`. They can't be copied and their fields can't be set directly.

## Canonical starting point

`@game_loop` handles the Context, the `Input`/`GUI`/`BasicGraphics` services, timing, framerate cap, and viewport resizing:

```julia
using Bplus; @using_bplus
using CImGui   # B+ wraps Dear ImGUI but does not re-export CImGui itself

@game_loop begin
    INIT(v2i(1920, 1080), "My Window Title"; debug_mode=true)

    SETUP = begin
        # Assets and game state. `LOOP::GameLoop` is in scope here and below.
        LOOP.max_fps = 144
        my_tex = Texture(SimpleFormat(FormatTypes.normalized_uint,
                                      SimpleFormatComponents.RGBA,
                                      SimpleFormatBitDepths.B8),
                         v2u(256, 256))
    end
    LOOP = begin
        if GLFW.WindowShouldClose(LOOP.context.window)
            break   # `break`, not `return` — a return skips TEARDOWN
        end
        clear_screen(vRGBAf(0.1, 0.1, 0.15, 1))
        clear_screen(@f32 1.0)
        simple_blit(my_tex)
        CImGui.Text("frame $(LOOP.frame_idx), dt=$(LOOP.delta_seconds)")
    end
    TEARDOWN = begin
        close(my_tex)
    end
end
```

`LOOP` exposes read-only `delta_seconds`, `frame_idx`, `context`, `service_input`, `service_gui`, `service_basic_graphics`, and settable `max_fps` (default 300) and `max_frame_duration` (default 0.1s, which clamps `delta_seconds` after a hang).

When `@game_loop` is too rigid, drop to `bp_gl_context(args...) do context::Context … end` — same Context construction, your own loop, automatic cleanup. You then own `service_Input_update()`, `service_GUI_start_frame()`/`_end_frame()`, and `GLFW.SwapBuffers`.

## Debug mode

Each module has its own toggleable assert flag, off by default. Turn one on *while loading* your code, never in a release build:

```julia
BplusApp.GL.bp_gl_asserts_enabled() = true
BplusTools.ECS.bp_ecs_asserts_enabled() = true
```

Do the same for your own project with `@make_toggleable_asserts(my_game_)`, which generates `@my_game_assert`, `@my_game_debug`, and `my_game_asserts_enabled()`.

For OpenGL errors, call `@check_gl_logs("some context")` at least once a frame in debug builds — the underlying log drops messages if left unpolled. `pull_gl_logs()` is the non-printing version.

## Testing

Tests live in `test/`, centralized in `test/runtests.jl`; the umbrella package runs each sub-package's suite in sequence (`BplusCore.TEST_RUNNER_PATH` standardizes the runner). B+ uses its own macros rather than `Test`, because the stdlib doesn't allow per-test messages:

- `@bp_check(condition, msg...)`
- `@bp_check_throws(statement, msg...)`
- `@bp_test_no_allocations(expr, expected_value, msg...)` — for math and scene-graph code that must stay static and type-stable
- `@bp_test_no_allocations_setup(setup_expr, test_expr, expected_value, msg...)` when setup itself may allocate

## Known errors in the published docs

The upstream `docs/` folder has a few mistakes; prefer these:

- Render state: the culling setter is **`set_culling(...)`**, not `set_cull(...)` (`GL.md` says the latter).
- Vertex attributes: for integer data cast or normalized into a float vector, use **`VSInput_FVector(V, normalized::Bool)`**. `Resources.md`'s mesh example writes `VSInput(vRGBu8, true)`, which has no such overload.
- Sync: **`gl_catch_up_before(actions::E_MemoryActions)`** takes a required `MemoryActions` bitflag argument; `GL.md` shows it with none.
- `BasicGraphics.empty_mesh` uses **`PrimitiveTypes.point`**; `Helpers.md` writes `PrimitiveType.points`.
- Typos in `Matrix.md` and `Math.md`: the real names are **`m_transpose`** and **`contiguous_ptr`**.
- `docs/!home.md` links a `Fields.md` that was never written. That's because the Field module is unfinished and considered dead in the water (a bad approximation of other Julia packages).

## Where to look when this skill isn't enough

- Inline doc-strings are thorough — `@component`, `@bp_service`, `@std140`, `@game_loop`, and `perlin` all carry long worked examples.
- Unit tests are the most reliable usage examples, especially `BplusTools/test/fields.jl`, `BplusTools/test/scene-tree.jl`, and `BplusApp/test/GL.jl`.
- [BpExamples](https://github.com/heyx3/BpExamples) — sample projects. [Drill8](https://github.com/heyx3/Drill8) — an open-source game leaning on the ECS. [BpWorld](https://github.com/heyx3/BpWorld) — a voxel toy.
