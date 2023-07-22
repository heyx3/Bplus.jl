"The game loop's state and parameters"
Base.@kwdef mutable struct GameLoop
    context::Context
    service_input::InputService
    service_basic_graphics::BasicGraphicsService
    service_gui::Optional{GuiService}

    # The amount of time elapsed since the last frame
    delta_seconds::Float32 = 0
    # Counter that's incremented at the beginning of every frame
    frame_idx::Int = 0

    # THe maximum framerate.
    # The game loop will wait at the end of each frame if the game is running faster.
    max_fps::Optional{Int} = nothing

    # The maximum frame duration.
    # 'delta_seconds' will be capped at this value, even if the frame took longer.
    # This stops the game from significantly jumping after one hang.
    max_frame_duration::Float32 = 0.1

    # The timestamp of the beginning of this loop.
    last_frame_time_ns::UInt64 = 0
end

"""
Runs a basic game loop, with all the typical B+ services.
The syntax looks like this:

````
@game_loop {
    INIT(
        # Pass all the usual arguments to the constructor for a `GL.Context`.
        # For example:
        v2i(1920, 1080), "My Window Title";
        debug_mode=true
    )

    SETUP = begin
        # Julia code block that runs at the beginning of the loop.
        # You can configure loop paramers by changing fields of the variable `LOOP::GameLoop`.
    end
    LOOP = begin
        # Julia code block that runs inside the loop.
        # Runs in a `for` loop in the same scope as `SETUP`.
        # You should eventually kill the loop with `break`.
        # If you use `return` or `throw`, then the `TEARDOWN` section won't run.
    end
    TEARDOWN = begin
        # Julia code block that runs after the loop, when the game is ending.
        # Runs in the same scope as `SETUP`.
    end
}
````

In all the code blocks but INIT, you have access to the game loop state through the variable
    `LOOP::GameLoop`
"""
macro game_loop(block)
    if !Base.is_expr(block, :block)
        error("Game loop should be a 'begin ... end' block")
    end
    statements = block.args
    filter!(s -> !isa(s, LineNumberNode), statements)

    skip_gui::Bool = false
    init_args = ()
    setup_code = nothing
    loop_code = nothing
    teardown_code = nothing
    for statement in statements
        if Base.is_expr(statement, :call) && (statement.args[1] == :INIT)
            if init_args != ()
                error("Provided INIT more than once")
            end
            init_args = statement.args[2:end]
        elseif Base.is_expr(statement, :(=)) && (statement.args[1] == :SETUP)
            if exists(setup_code)
                error("Provided SETUP more than once")
            end
            setup_code = statement.args[2]
        elseif Base.is_expr(statement, :(=)) && (statement.args[1] == :LOOP)
            if exists(loop_code)
                error("Provided LOOP more than once")
            end
            loop_code = statement.args[2]
        elseif Base.is_expr(statement, :(=)) && (statement.args[1] == :TEARDOWN)
            if exists(teardown_code)
                error("Provided TEARDOWN more than once")
            end
            teardown_code = statement.args[2]
        elseif Base.is_expr(statement, :(=)) && (statement.args[1] == :USE_GUI)
            skip_gui = !statement.args[2]
        else
            error("Unknown code block: ", statement)
        end
    end

    loop_var = esc(:LOOP)
    do_body = :( (game_loop_impl_context::Context, ) -> begin
        # Set up the loop state object.
        $loop_var::GameLoop = GameLoop(
            context=game_loop_impl_context,
            service_input=service_input_init(game_loop_impl_context),
            service_basic_graphics=get_basic_graphics(game_loop_impl_context),
            service_gui=if $(esc(skip_gui))
                nothing
            else
                service_gui_init(game_loop_impl_context)
            end
        )
        # Set up timing.
        $loop_var.last_frame_time_ns = time_ns()
        $loop_var.delta_seconds = zero(Float32)

        # Auto-resize the GL viewport when the window's size changes.
        push!($loop_var.context.glfw_callbacks_window_resized, (new_size::v2i) ->
            set_viewport($loop_var.context, Box2Di(min=Vec(1, 1), size=new_size))
        )


        # Run the loop.
        $(esc(setup_code))
        while true
            GLFW.PollEvents()

            # Update/render.
            service_input_update($loop_var.service_input)
            if exists($loop_var.service_gui)
                service_gui_start_frame($loop_var.service_gui)
            end
            $(esc(loop_code))
            if exists($loop_var.service_gui)
                service_gui_end_frame($loop_var.service_gui, $loop_var.context)
            end
            GLFW.SwapBuffers($loop_var.context.window)

            # Advance the timer.
            $loop_var.frame_idx += 1
            new_time::UInt = time_ns()
            $loop_var.delta_seconds = Float32((new_time - $loop_var.last_frame_time_ns) / 1e9)
            # Cap the framerate, by waiting if necessary.
            if exists($loop_var.max_fps)
                wait_time = (1/$loop_var.max_fps) - $loop_var.delta_seconds
                if wait_time > 0
                    sleep(wait_time)
                    # Update the timestamp again after waiting.
                    new_time = time_ns()
                    $loop_var.delta_seconds = Float32((new_time - $loop_var.last_frame_time_ns) / 1e9)
                end
            end
            $loop_var.last_frame_time_ns = new_time
            # Cap the length of the next frame.
            $loop_var.delta_seconds = min(Float32($loop_var.max_frame_duration),
                                            $loop_var.delta_seconds)
        end
        $(esc(teardown_code))
    end )

    # Wrap the game in a lambda in case users invoke it globally.
    # Global code is very slow in Julia.
    return :( (() ->
        $(Expr(:call, bp_gl_context,
            do_body,
            esc.(init_args)...
    )))() )
end

export @game_loop

#TODO: Fixed-update increments for physics-like stuff