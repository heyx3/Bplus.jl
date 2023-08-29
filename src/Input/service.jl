"Manages all inputs for a GL context/window"
@bp_service Input(force_unique) begin
    buttons::Dict{AbstractString, Vector{ButtonInput}}
    axes::Dict{AbstractString, AxisInput}

    current_scroll_pos::v2f
    context_window::GLFW.Window

    scroll_callback::Callable


    INIT() = begin
        context = get_context()

        service = new(
            Dict{String, Vector{ButtonInput}}(),
            Dict{String, AxisInput}(),
            zero(v2f),
            context.window,
            _ -> nothing # Dummy callable, real one is below
        )
        service.scroll_callback = (delta::v2f -> (service.current_scroll_pos += delta))

        # Use GLFW's 'sticky keys' mode to remember press/release events
        #    that start and end within a single frame.
        GLFW.SetInputMode(context.window, GLFW.STICKY_MOUSE_BUTTONS, true)

        # Register GLFW callbacks.
        push!(context.glfw_callbacks_scroll, service.scroll_callback)

        return service
    end
    SHUTDOWN(service, is_context_closing::Bool) = begin
        if !is_context_closing
            delete!(get_context().glfw_callbacks_scroll,
                    service.scroll_callback)
        end
    end

    service_Input_update(s)::Nothing = begin
        for button_list in values(s.buttons)
            map!(b -> update_input(b, s.context_window),
                 button_list, button_list)
        end
        for axis_name in keys(s.axes)
            s.axes[axis_name] = update_input(s.axes[axis_name],
                                             s.context_window, s.current_scroll_pos)
        end
    end


    "
    Creates a new button (throwing an error if it already exists), with the given inputs.
    Returns the source list of button inputs, so you can further configure them at will.
    All button inputs will be OR-ed together.
    "
    function create_button(service,
                           name::AbstractString,
                           inputs::ButtonInput...
                          )::Vector{ButtonInput}
        if haskey(service.buttons, name)
            error("Button already exists: '", name, "'")
        else
            service.buttons[name] = collect(inputs)
            return service.buttons[name]
        end
    end
    "
    Deletes the given button.
    Throws an error if it doesn't exist.
    "
    function remove_button(service, name::AbstractString)
        if haskey(service.buttons, name)
            delete!(service.buttons, name)
        else
            error("Button does not exist to be deleted: '", name, "'")
        end
    end

    "Creates a new axis (throwing an error if it already exists), with the given input"
    function create_axis(service,
                         name::AbstractString,
                         axis::AxisInput)
        if haskey(service.axes, name)
            error("Axis already exists: '", name, "'")
        else
            # If this input uses the mouse position, prepare it with the current state of the mouse.
            # Otherwise there will be a big jump on the first frame's input.
            if axis.id == MouseAxes.x
                @set! axis.prev_raw = GLFW.GetCursorPos(service.context_window).x
            elseif axis.id == MouseAxes.y
                @set! axis.prev_raw = GLFW.GetCursorPos(service.context_window).y
            end

            service.axes[name] = axis
            return nothing
        end
    end
    "
    Deletes the given axis.
    Throws an error if it doesn't exist.
    "
    function remove_axis(service, name::AbstractString)
        if haskey(service.axes, name)
            delete!(service.axes, name)
        else
            error("Axis does not exist to be deleted: '", name, "'")
        end
    end


    "Gets the current value of a button. Throws an error if it doesn't exist."
    function get_button(service, name::AbstractString)::Bool
        if haskey(service.buttons, name)
            return any(b -> b.value, service.buttons[name])
        else
            error("No button named '", name, "'")
        end
    end
    "Gets the current value of an axis. Throws an error if it doesn't exist."
    function get_axis(service, name::AbstractString)::Float32
        if haskey(service.axes, name)
            return service.axes[name].value
        else
            error("No axis named '", name, "'")
        end
    end
    "Gets the current value of a button or axis. Returns `nothing` if it doesn't exist."
    function get_input(service, name::AbstractString)::Union{Bool, Float32, Nothing}
        if haskey(service.buttons, name)
            return any(b -> b.value, service.buttons[name])
        elseif haskey(service.axes, name)
            return service.axes[name].value
        else
            return nothing
        end
    end

    "
    Gets the source list of button inputs for a named button.
    You can modify this list at will to reconfigure the button.
    Throws an error if the button doesn't exist.
    "
    function get_button_inputs(service, name::AbstractString)::Vector{ButtonInput}
        if haskey(service.buttons, name)
            return service.buttons[name]
        else
            error("No button named '", name, "'")
        end
    end
end

export service_Input_init, service_Input_shutdown,
       service_Input_get, service_Input_exists, service_Input_update
       create_button, create_axis,
       remove_button, remove_axis,
       get_input, get_button, get_axis,
       get_button_inputs