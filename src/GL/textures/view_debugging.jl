"Raised in debug builds if a View is used in a Program without being activated."
struct InactiveViewsException <: Exception
    program::Ptr_Program
    offenders::Vector{Pair{Ptr_Uniform, View}}
end
function Base.showerror(io::IO, e::InactiveViewsException)
    print(io,
        "Shader Program ", Int(gl_type(e.program)),
        " is about to be used, but it has ", length(e.offenders),
        " texture uniforms with an inactive View, which is Undefined Behavior! ["
    )
    for offender in e.offenders
        print(io, "\n\t\"", offender[1], "\" (view ", offender[2].handle, ")")
    end
    print(io, "\n]")
end

view_debugger_service_enabled() = @bp_gl_debug() || get_context().debug_mode


"""
Provides a Context service in debug builds which checks that a program's Views
   are properly activated before use.
Note that this can't catch them in all circumstances!
View handles are just a plain uint64, so you can sneak them into a shader
   in all sorts of ways.
In particular, reading the handle from mesh data
   isn't something this service can notice.
"""
@bp_service ViewDebugging(force_unique, lazy) begin
    view_lookup::Dict{Ptr_View, View}
    uniform_lookup::Dict{Ptr_Program, Dict{Ptr_Uniform, Ptr_View}}

    INIT() = new(
        Dict{Ptr_View, View}(),
        Dict{Ptr_Program, Dict{Ptr_Uniform, Ptr_View}}()
    )
    SHUTDOWN(service, is_context) = nothing

    "Registers a new view instance"
    function service_ViewDebugging_add_view(service, ptr::Ptr_View, instance::View)
        if view_debugger_service_enabled()
            @bp_gl_assert(!haskey(service.view_lookup, ptr),
                          "Pointer already exists for view: ", ptr)
            service.view_lookup[ptr] = instance
        end
    end
    "Registers a new shader program"
    function service_ViewDebugging_add_program(service, ptr::Ptr_Program)
        if view_debugger_service_enabled()
            @bp_gl_assert(!haskey(service.uniform_lookup, ptr),
                          "Pointer already exists for program: ", ptr)
            service.uniform_lookup[ptr] = Dict{Ptr_Uniform, Ptr_View}()
        end
    end

    "Un-registers a shader program from this service"
    function service_ViewDebugging_remove_program(service, program::Ptr_Program)
        if view_debugger_service_enabled()
            @bp_gl_assert(haskey(service.uniform_lookup, program),
                          "Program ", program, " is missing from the View Debugger service")
            delete!(service.uniform_lookup, program)
        end
    end
    "Un-registers a View from this service"
    function service_ViewDebugging_remove_view(service, view::Ptr_View)
        if view_debugger_service_enabled()
            @bp_gl_assert(haskey(service.view_lookup, view),
                          "View ", view, " is missing from the View Debugger service")
            delete!(service.view_lookup, view)
        end
    end

    "Registers a new texture view for a given program's uniform."
    function service_ViewDebugging_set_view(service,
                                           program::Ptr_Program,
                                           u_ptr::Ptr_Uniform, u_value::Ptr_View)
        if view_debugger_service_enabled()
            if !haskey(service.uniform_lookup, program)
                service.uniform_lookup[program] = Dict{Ptr_Uniform, View}()
            end
            service.uniform_lookup[program][u_ptr] = u_value
        end
    end
    "Registers a set of texture views for a given program's uniform array."
    function service_ViewDebugging_set_views(service,
                                            program::Ptr_Program,
                                            u_start_ptr::Ptr_Uniform,
                                            u_values::Vector{<:Union{Ptr_View, gl_type(Ptr_View)}})
        if view_debugger_service_enabled()
            if !haskey(service.uniform_lookup, program)
                service.uniform_lookup[program] = Dict{Ptr_Uniform, View}()
            end
            for i::Int in 1:length(u_values)
                service.uniform_lookup[program][u_start_ptr + i - 1] = u_values[i]
            end
        end
    end

    "Checks a program to make sure its Views are all activated."
    function service_ViewDebugging_check(service, program::Ptr_Program)
        if view_debugger_service_enabled()
            @bp_gl_assert(haskey(service.uniform_lookup, program),
                          "Program ", program, " is missing from the View Debugger service")

            # Gather the inactive views used by the program,
            #    and raise an error if any exist.
            views = (
                (u_ptr => service.view_lookup[v_ptr])
                  for (u_ptr, v_ptr) in service.uniform_lookup[program]
                  if !service.view_lookup[v_ptr].is_active
            )
            if !isempty(views)
                throw(InactiveViewsException(program, collect(views)))
            end
        end
    end
end

# Not exported, because it's an internal debugging service.