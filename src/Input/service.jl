"Manages all inputs for a GL context/window"
mutable struct InputService
    buttons::Dict{AbstractString, Vector{ButtonInput}}
    axes::Dict{AbstractString, AxisInput}

    current_scroll_pos::v2f
    context_window::GLFW.Window
end
export InputService


##  Management  ##

const SERVICE_NAME_INPUT = :bplus_input_system

function service_input_init(context::GL.Context = GL.get_context())::InputService
    serv = InputService(
        Dict{String, Vector{ButtonInput}}(),
        Dict{String, AxisInput}(),
        zero(v2f),
        context.window
    )

    # Use GLFW's 'sticky keys' mode to remember press/release events
    #    that start and end within a single frame.
    GLFW.SetInputMode(context.window, GLFW.STICKY_MOUSE_BUTTONS, true)

    # Register some GLFW callbacks.
    push!(context.glfw_callbacks_scroll, (s::v2f) ->
        serv.current_scroll_pos += s
    )

    # Register the service with the context.
    wrapper = GL.Service(serv, on_destroyed = service_input_cleanup)
    GL.register_service(context, SERVICE_NAME_INPUT, wrapper)
    return serv
end
function service_input_cleanup(s::InputService)
    # Nothing to do for now
end

function service_input_get(context::GL.Context = GL.get_context())::InputService
    return GL.get_service(context, SERVICE_NAME_INPUT)
end

function service_input_update(s::InputService = service_input_get())::Nothing
    for button_list in values(s.buttons)
        map!(b -> update_input(b, s.context_window),
             button_list, button_list)
    end
    for axis_name in keys(s.axes)
        s.axes[axis_name] = update_input(s.axes[axis_name],
                                         s.context_window, s.current_scroll_pos)
    end
end

export service_input_init, service_input_get, service_input_update


##  User interface  ##

#TODO: Help with (de-)serialization of bindings

"
Creates a new button (throwing an error if it already exists), with the given inputs.
Returns the source list of button inputs, so you can further configure them at will.
All button inputs will be OR-ed together.
"
function create_button(name::AbstractString,
                       inputs::ButtonInput...
                       ;
                       service::InputService = service_input_get()
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
function remove_button(name::AbstractString,
                       service::InputService = service_input_get())
    if haskey(service.buttons, name)
        delete!(service.buttons, name)
    else
        error("Button does not exist to be deleted: '", name, "'")
    end
end

"Creates a new axis (throwing an error if it already exists), with the given input"
function create_axis(name::AbstractString,
                     axis::AxisInput,
                     service::InputService = service_input_get())
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
function remove_axis(name::AbstractString,
                     service::InputService = service_input_get())
    if haskey(service.axes, name)
        delete!(service.axes, name)
    else
        error("Axis does not exist to be deleted: '", name, "'")
    end
end


"Gets the current value of a button. Throws an error if it doesn't exist."
function get_button(name::AbstractString, service::InputService = service_input_get())::Bool
    if haskey(service.buttons, name)
        return any(b -> b.value, service.buttons[name])
    else
        error("No button named '", name, "'")
    end
end
"Gets the current value of an axis. Throws an error if it doesn't exist."
function get_axis(name::AbstractString, service::InputService = service_input_get())::Float32
    if haskey(service.axes, name)
        return service.axes[name].value
    else
        error("No axis named '", name, "'")
    end
end
"Gets the current value of a button or axis. Returns `nothing` if it doesn't exist."
function get_input(name::AbstractString, service::InputService = service_input_get())::Union{Bool, Float32, Nothing}
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
function get_button_inputs(name::AbstractString, service::InputService = service_input_get())::Vector{ButtonInput}
    if haskey(service.buttons, name)
        return service.buttons[name]
    else
        error("No button named '", name, "'")
    end
end

export create_button, create_axis,
       remove_button, remove_axis,
       get_input, get_button, get_axis,
       get_button_inputs