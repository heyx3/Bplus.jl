The module *Bplus.Helpers*. A grab-bag of B+ utilities you'll likely find useful.

# Game Loop

Handles all the boilerplate of a basic game loop for you. Refer to the doc-string `@game_loop`, and the file *Helpers/game_loop.jl*, for detailed info on usage.

The loop handles all built-in B+ services for you: [`Input`](Input.md), [`GUI`](GUI.md), and [`BasicGraphics`](Helpers.md#Basic-Graphics)

Within the loop you'll have access to the variable `LOOP::GameLoop`, which has the following fields which you can read but should not set:

* `delta_seconds::Float32` : the amount of time elapsed since the last loop iteration.
* `frame_idx::Int` : the number of elapsed frames so far, plus 1.
* `last_frame_time_ns::UInt64` : the most recent timestamp, from the end of the last frame.
* `context::Context` : the OpenGL/GLFW context
* `service_input` : the [Input service](Input.md).
* `service_basic_graphics` : the [Basic Graphics service](#Basic-Graphics).
* `service_gui` : the [GUI service](GUI.md).

`LOOP` also has some fields which you can set to configure the loop. You can set them both in the `SETUP` phase and the `LOOP` phase.

* `max_fps::Optional{Int} = 300` caps the game's framerate.
* `max_frame_duration::Float32 = 0.1` caps the `delta_seconds` field in the case of very slow frames. This prevents significant jumps in the game after a hang.

# Basic Graphics

A [B+ Context service](GL.md#Services) that provides lots of basic resources:
e which defines a bunch of useful GL resources:

 * `screen_triangle` : A 1-triangle mesh with 2D positions in NDC-space.
    When drawn, it will perfectly cover the entire screen,
        making it easy to spin up post-processing effects.
    The UV coordinates can be calculated from the XY positions
        (or from gl_FragCoord).
 * `screen_quad` : A 2-triangle mesh describing a square, with 2D coordinates
        in NDC space (-1 to +1)
    This _can_ be used for post-processing effects, but it's less efficient
        than `screen_triangle` for technical reasons.
 * `blit` : A simple shader to render a 2D texture (e.x. copy a Target to the screen).
    Refer to `simple_blit()`.
 * `empty_mesh` : A mesh with no vertex data, for dispatching entirely procedural geometry.
    Uses `PrimitiveType.points`.

To draw a texture, call `simple_blit(tex_or_view; params...)`. It has the following named parmeters:

* `quad_transform::fmat3x3` transforms the 2D quad.
  * Defaults to the identity transform.
* `color_transform::fmat4x4` transforms the sampled color into an output color.
  * Defaults to the identity transform.
* `output_curve::Float32` is an exponent that can be applied to all 4 output channels.
* `disable_depth_test::Bool` is a flag for automtically disabling depth tests before drawing the quad, then reinstating the depth test state afterwards.
  * By default it's true.
* `manage_tex_view::Bool` is a flag for automatically calling `view_activate()` and `view_deactivate()` on the blitted texture/view if it's not already activated.
  * By default it's true.

# File Cacher

A system for reloading files from disk anytime they are changed. To use this system, do the following:

1. Define the type of data that is cached. We will refer to this type as `TCached`. For example, a texture cache could use `Bplus.GL.Texture`.
2. Define a function which loads, or *re*-loads, an asset from disk. We will refer to this as `reload_response::Base.Callable`.
   * The signature should be `(path::AbstractString[, old_data;:TCached]) -> new_data::TCached[, dependent_files]`.
     * If your loaded asset depends on other files beyond its own path, add a second return value which is an iterator of those files. Then a reload can be triggered by any of these files changing.
   * This function is allowed to throw if the file can't be loaded for any reason.
3. Define a function which acknowledges an error from calling `reload_response`. We will refer to this as `error_response::Base.Callable`.
   * The signature should be `(path::AbstractString, exception, trace[, old_data::TCached]) -> [fallback_data::TCached]`.
   * If you want to return a fallback instance (like an "error texture"), you can return it here. Otherwise, return anything other than a `TCached`, such as `nothing`.
   * You may also want to log an error here, e.x. `@error "Failed to load $path" ex=(exception, trace)`
4. Create an instance of the cacher with the above parameters: `FileCacher{TCached}(reload_response=reload_response, error_response=error_response, other_args...)`.
   * Pass `relative_path = p` to change the relative path for files.
     * It defaults to `pwd()`, a.k.a. the location Julia is running from.
   * Pass `check_interval_ms = a:b` to change the randomized time interval for checking each file for changes.
     * The randomization prevents all cached files from being checked at the same time, which could cause a disk bottleneck.
     * Default is `3000:5000`, i.e. 3-5 seconds.
5. Update the cacher with `check_disk_modifications!(cacher)::Bool`.
   * It returns true if any files have been reloaded.
   * The naive way to call this is once every frame, but you can probably get away with doing it much less often since files are not reloaded very frequently.
6. Get a file with `get_cached_data!(cacher, relative_or_absolute_path)::Optional{TCached}`.
   * If your `error_response` provides a fallback instance, then this function *always* returns a `TCached`.