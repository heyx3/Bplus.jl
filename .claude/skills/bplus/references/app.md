# Game loop, Input, GUI, and helpers

Covers `@game_loop` and `BasicGraphics` (BplusApp), `Input` and `GUI` (BplusApp), plus `Cam3D` and `FileCacher` (BplusTools).

---

# `@game_loop`

Handles the Context, the `Input`/`GUI`/`BasicGraphics` services, timing, framerate capping, and automatic GL-viewport resizing on window resize.

```julia
@game_loop begin
    INIT(
        # Arguments forwarded straight to the GL.Context constructor.
        v2i(1920, 1080), "My Window Title";
        debug_mode=true
    )

    SETUP = begin
        # Runs once, just before the loop. Load assets, add CImGui fonts,
        # configure the loop by setting fields of `LOOP::GameLoop`.
    end
    LOOP = begin
        # Runs inside a `for` loop, in the same scope as SETUP.
        # End it with `break` — a `return` or `throw` skips TEARDOWN.
    end
    TEARDOWN = begin
        # Runs after the loop, in the same scope as SETUP.
    end
end
```

`LOOP::GameLoop` is in scope in `SETUP`, `LOOP`, and `TEARDOWN` (not `INIT`).

Read-only fields: `delta_seconds::Float32`, `frame_idx::Int` (elapsed frames + 1), `last_frame_time_ns::UInt64`, `context::Context`, `service_input`, `service_basic_graphics`, `service_gui`.

Settable, from either `SETUP` or `LOOP`:

- `max_fps::Optional{Int} = 300` — the loop waits at the end of a frame if running faster.
- `max_frame_duration::Float32 = 0.1` — caps `delta_seconds` so the game doesn't lurch after a hang.

Each iteration runs, in order: `GLFW.PollEvents()`, `service_Input_update()`, `service_GUI_start_frame()`, **your `LOOP` block**, `service_GUI_end_frame()`, `GLFW.SwapBuffers`, then timing and the FPS cap.

When this is too rigid, use `bp_gl_context(...) do context::Context … end` and drive those steps yourself.

---

# BasicGraphics service

A Context service holding resources almost every project wants:

- `screen_triangle` — a single triangle with 2D NDC positions that covers the whole screen. The efficient choice for post-processing. UVs derive from the XY positions (or `gl_FragCoord`).
- `screen_quad` — two triangles forming a −1…+1 square. Works for post-processing too, but is less efficient than `screen_triangle`.
- `blit` — a simple shader that draws a 2D texture, e.g. copying a Target to the screen.
- `empty_mesh` — no vertex data, for fully procedural geometry. Uses `PrimitiveTypes.point` (the upstream `Helpers.md` writes `PrimitiveType.points`; both halves of that are wrong).

## Blitting

```julia
simple_blit(tex_or_view; params...)
```

| Parameter | Default | Effect |
| --- | --- | --- |
| `quad_transform::fmat3x3` | identity | Transforms the 2D quad |
| `color_transform::fmat4x4` | identity | Transforms sampled color into output color |
| `output_curve::Float32` | — | Exponent applied to all 4 output channels |
| `disable_depth_test::Bool` | `true` | Disables depth testing for the draw, then restores it |
| `manage_tex_view::Bool` | `true` | Calls `view_activate`/`view_deactivate` if the view isn't already active |

For a custom blit-like shader, build the vertex stage with `make_vertex_shader_blit()` (see its doc-string) and draw with `custom_blit(tex_or_view, shader_program)`. It takes `disable_depth_test` and `manage_tex_view` as above, plus:

- `assume_full_screen::Bool` — draw the screen triangle instead of the quad
- `tex_name::String` — if your texture uniform isn't named `"u_tex"`

---

# Input service

A simplified window into GLFW input, defined data-driven so bindings serialize cleanly through *StructTypes*.

Update it every frame with `service_Input_update()` — already handled inside `@game_loop`. `service_Input_reset()` returns it to a blank slate.

## Buttons

Binary inputs. One button may have several sources, OR-ed together — bind "fire" to right-click, Enter, and Joystick1 → Button4 simultaneously.

```julia
create_button(name::AbstractString, inputs::ButtonInput...)
get_button(name::AbstractString)::Bool
```

Sources: keyboard keys, mouse buttons, joystick buttons. See `ButtonInput` for configuration.

## Axes

Continuous `Float32` inputs. Unlike buttons, an axis has exactly **one** source.

```julia
create_axis(name::AbstractString, input::AxisInput)
get_axis(name::AbstractString)::Float32
```

Sources: mouse position, scroll wheel, joystick axes, or a list of `ButtonAsAxis`. See `AxisInput`.

---

# GUI service (Dear ImGUI)

Wraps Dear ImGUI, exposed in Julia through *CImGui.jl*.

B+ vectors (`Vec{N,T}`) convert freely to and from CImGui's `ImVec2`, `ImVec4`, `ImColor`.

## Lifecycle

`service_GUI_init()`, `service_GUI_exists()`, `service_GUI_start_frame()`, `service_GUI_end_frame()` (renders to the currently-bound target, usually the screen), `service_GUI_shutdown()` (optional if the program is closing), `service_GUI_rebuild_fonts()` (after adding custom fonts), `service_GUI()` (the instance).

Inside `@game_loop` this is all done for you — just call CImGui functions in the `LOOP` block.

The underlying library uses static variables and doesn't manage independent contexts, so **only one GL Context may run this service at a time**.

To draw a texture in the GUI, wrap it: `gui_tex_handle(tex_or_view)`. Passing the raw OpenGL handle will not work.

Debugging: `service_GUI().debug_log_render_commands = true`.

## `GuiText`

Text editing across a C boundary means C strings, clipboard integration, and dynamic resizing. `GuiText` handles all of it.

```julia
t = GuiText("ab\ncdef\ngh", is_multiline=true)
any_changes::Bool = gui_text!(t)
string(t)                    # current value
update!(t, new_string)       # set it
```

## Scoped helpers

Most Dear ImGUI state is static variables you set as you draw, so the common need is "set, draw, restore". These helpers take your GUI code as a lambda — use `do` syntax:

```julia
gui_with_indentation() do
    # indented GUI code
end
```

Nearly all take a keyword `unchanged::Bool` which, when true, runs your lambda *without* changing ImGUI state.

| Function | Purpose |
| --- | --- |
| `gui_window(args...; kw...)::Optional` | New window; args forward to `CImGui.Begin()`. Returns your block's output, or `nothing` if culled. |
| `gui_with_item_width(width::Real)` | Widget width |
| `gui_with_indentation(indent::Optional{Real} = nothing)` | Indent |
| `gui_with_tooltip(; skip=false)` | Tooltip on the previous widget/group; your lambda fills it. Takes `skip` rather than `unchanged` — when true the lambda doesn't run at all. |
| `gui_with_padding(padding...)` | Window padding; X and Y, or a tuple |
| `gui_with_clip_rect(rect::Box2Df, intersect_with_current::Bool, draw_list = nothing)` | Clip rectangle |
| `gui_with_font(font_or_idx::Union{Ptr, Int})` | Switch to an already-loaded font |
| `gui_with_unescaped_tabbing()` | Disable Tab-to-next-widget, so a text editor can receive Tab |
| `gui_with_nested_id(values::Union{AbstractString, Ptr, Integer}...)` | Push onto ImGUI's ID stack. Widgets are identified by label + ID stack, so this disambiguates same-labelled widgets. |
| `gui_within_fold(label)` | Collapsible region (ImGUI's "tree node") |
| `gui_with_style(var::ImGuiStyleVar, value)` | One style variable |
| `gui_with_style(color_idx::ImGuiCol_, value)` | One style color |
| `gui_within_group()` | Treat widgets as one group — e.g. a vertical section inside a horizontal layout, followed by `CImGui.SameLine()` |
| `gui_tab_views()` / `gui_tab_item()` | Tab bar and one tab |
| `gui_within_child_window(size, flags=0)::Optional` | Sub-window; returns your output or `nothing` if culled |

## Drawing primitives

```julia
gui_draw_line(coords, color, thickness=1; ...)
gui_draw_rect(coords, color; ...)
gui_draw_quad(corners, color; ...)
```

Coordinate format depends on the shape — `NTuple{2, v2f}` for lines, `Box2Df` for rects — and is always absolute. Make it relative to ImGUI's next-widget cursor by wrapping: `GuiDrawCursorRelative(coords, emit_dummy)`, where `emit_dummy` (a `Bool`, or a per-axis `Vec{2, Bool}`) controls whether the shape advances the cursor and emits a `Dummy` widget reaching its max corner.

Color/fill options depend on the shape. Functions taking a `GuiDrawSimpleColorType` accept `GuiDrawColorBorder` (border only) or `GuiDrawColorFilled`. Those taking a `GuiDrawColorType` additionally accept `GuiDrawMultiColor` (one color per coordinate) and `GuiDrawColorTexture` (filled with an image).

The `canvas` keyword picks the destination. Drawing in ImGUI normally targets a "draw list", but ~90% of the time that's the current window and ~9% the screen foreground/background, so these take the `GuiDrawingCanvas` enum as shorthand (default `GuiDrawingCanvas.current_window`) or any raw `Ptr{CImGui.ImDrawList}`.

## Other helpers

- `gui_get_draw_clip_area()::Box2Df`
- `gui_add_font_from_memory_ttf(bytes_view, pixel_sizes[, oversampling])` — loads one TTF at several sizes, returning a font handle per size
- `gui_spherical_vector(label, direction::v3f; settings...)::v3f` — edits a direction as pitch/yaw
- `gui_next_window_space(uv_space::Box2Df, ...)` — sizes the next window as a percentage of the program window, with optional padding border and pixel-size clamp

---

# Cam3D

A movable, turnable 3D camera. Three structs: `Cam3D{F}` (state), `Cam3D_Settings{F}` (config), `Cam3D_Inputs{F}` (this frame's controls).

```julia
(cam, settings) = cam_update(cam, settings, input, delta_seconds)
```

Settings come back because they can change — e.g. when the `speed_change` input fires.

- `cam_basis(cam)` — all three facing axes; `cam_rightward(cam)` — just the right axis
- `cam_view_mat(cam)`, `cam_projection_mat(cam)`
- Projection settings are either `PerspectiveProjection{F}` or `OrthographicProjection{F}`

---

# FileCacher

Reloads files from disk whenever they change.

1. Pick the cached type, `TCached` (e.g. `BplusApp.GL.Texture`).
2. Write `reload_response`: `(path::AbstractString[, old_data::TCached]) -> new_data::TCached[, dependent_files]`. Return a second value — an iterator of other files — if the asset depends on them, and a change to any will trigger a reload. It may throw.
3. Write `error_response`: `(path::AbstractString, exception, trace[, old_data::TCached]) -> [fallback_data::TCached]`. Return a fallback (an "error texture") or anything that isn't a `TCached`, such as `nothing`. Log here too: `@error "Failed to load $path" ex=(exception, trace)`.
4. `FileCacher{TCached}(reload_response=…, error_response=…, other_args...)`
   - `relative_path = p` — defaults to `pwd()`, i.e. where Julia was launched
   - `check_interval_ms = a:b` — randomized per-file check interval, default `3000:5000`. The randomization staggers checks so they don't all hit the disk at once.
5. `check_disk_modifications!(cacher)::Bool` — true if anything reloaded. Once a frame works, but far less often is fine; files rarely change.
6. `get_cached_data!(cacher, relative_or_absolute_path)::Optional{TCached}` — always returns a `TCached` if your `error_response` supplies a fallback.
