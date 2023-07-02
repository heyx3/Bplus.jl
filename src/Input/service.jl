"Manages all inputs for a GL context/window"
mutable struct InputService
    buttons::Dict{String, Vector{ButtonInput}}
    axes::Dict{String, AxisInput}

    current_scroll_pos::v2f
    context_window::GLFW.Window
end
export InputService


##  Management  ##

const SERVICE_NAME_INPUT = :bplus_input_system

function service_input_init(context::GL.Context = GL.get_context())::InputService
    serv = InputService(
        Dict{String, Vector{ButtonID}}(),
        zero(v2f),
        context.window
    )

    # Use GLFW's 'sticky keys' mode to remember press/release events
    #    that start and end within a single frame.
    GLFW.SetInputMode(context.window, GLFW.STICKY_MOUSE_BUTTONS, glfw_sticky_inputs)

    # Register some GLFW callbacks.
    push!(context.glfw_callbacks_scroll, (x::Float32, y::Float32) ->
        serv.current_scroll_pos += v2f(x, y)
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
        map!(button_list, button_list) do b::ButtonInput
            return update_input(b, s.context_window)
        end
    end
    for axis_list in values(s.axes)
        map!(axis_list, axis_list) do a::AxisInput
            return update_input(a, s.context_window, s.current_scroll_pos)
        end
    end
end

export service_input_init, service_input_get, service_input_update


##  User interface  ##

function add_input(name::AbstractString, input::Union{ButtonInput, AxisInput},
                   service::InputService = service_input_get())
    if input isa ButtonInput
        if haskey(service.buttons, name)
            error("Button named '", name, "' already exists")
        else
            service.buttons[name] = input
        end
    elseif input isa AxisInput
        if haskey(service.axes, name)
            error("Axis named '", name, "' already exists")
        else
            service.axes[name] = input
        end
    else
        error("Unknown input type: ", typeof(input))
    end
end
function remove_input(name::AbstractString, service::InputService = service_input_get())
    delete!(service.buttons, name)
    delete!(service.axes, name)
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
        service.axes[name]
    else
        error("No axis named '", name, "'")
    end
end

export add_input, remove_input,
       get_input, get_button, get_axis