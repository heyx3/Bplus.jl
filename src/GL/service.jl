"
A Context-wide mutable singleton, providing some utility for users.
It is highly recommended to define a service with `@bp_service`.
"
abstract type AbstractService end


# The following functions are an internal interface, used by Context, not to be exported:
service_internal_refresh(service::AbstractService) = nothing
service_internal_shutdown(service::AbstractService) = nothing


#TODO: A stronger version of 'force_unique' for services that must not exist more than once across multiple contexts
"""
Defines a B+ Context service with a standardized interface.

Example usage:

````
@bp_service Input(args...) begin
    # Some data that the service will hold:
    buttons::Dict{AbstractString, Any}
    axes::Dict{AbstractString, Any}
    current_scroll_pos::v2f
    # ... # [etc]

    # The service's initializer.
    INIT(a, b) = begin
        return new(a, b, zero(v2f)) # Provide values for the above fields
    end

    # The service's cleanup code.
    SHUTDOWN(service, is_context_closing::Bool) = begin
        close(service.some_file_handle)
        if !is_context_closing # Waste of time to destroy GL resources if the whole context is going away
            for tex in service.all_my_textures
                close(tex)
            end
        end
    end

    # If you want to be notified when the context refreshes,
    #    for example if outside OpenGL code ran and potentially changed OpenGL's state,
    #    you can define this:
    REFRESH(service) = begin
        service.last_rendered_program = glGetIntegerv(blah, blah)
    end

    # Custom functions for your service.
    # See below for info on how they are transformed into real functions.
    get_button(service, name) = service.buttons[name]
    add_button(service, name, value) = service.buttons[name] = value
end
````

The `args...` in the service name currently only accept one optional value, `force_unique`,
    which makes the service a singleton across the entire Context.
`force_unique` services will not let you create multiple instances,
    and you do not need to pass in references to the service when calling its functions.

# Interface

The following functions are generated for you based on your service's name
    (here we assume you named it `X`, case-sensitive):

## Startup/shutdown
* `service_X_init(args...)` : Executes your defined `INIT(args...)` code,
    adds the service to the Context, and returns it.
    If you did not specify `force_unique`, then you may call this multiple times
    to create multiple service instances.
* `service_X_shutdown([service])` : Closes your service, executing your defined
    `SHUTDOWN(service, is_context_closing)` code.
    Called automatically on context close, but you may kill it early if you want, in which case
        the value of the second parameter will be `false`.
    If you specified `force_unique`, then you do not have to provide the service instance when calling.
## Utility
* `service_X()` : Gets the current service instance, assuming it's already been created.
    This function only exists if you specified `force_unique`.
* `service_X_exists()` : Gets whether the service has already been created.
    This function only exists if you specified `force_unique`.
## Custom
Any other functions you declared get generated basically as-is.
  The first argument must always be the service, and then if you specified `force_unique`,
      the user of your function omits that parameter when calling.
  For example, `f(service, a, b) = a*b + service.z` turns into `f(a, b)` if `force_unique`.

# Notes

You can provide type parameters in your service name, in which case
    you must also pass them in when calling `new` in your `INIT`,
    and the `service_X()` getter will take those types as normal function parameters.

You can similarly provide type parameters in all functions, including special ones like `INIT`.
"""
macro bp_service(service_name, service_definitions)
    # Retrieve the different elements of the service declaration.
    service_args = [ ]
    if @capture(service_name, name_(args__))
        service_name = name
        service_args = args
    end

    # Parse the inner parameters to the service.
    service_is_unique = false
    for service_arg in service_args
        if service_arg == :force_unique
            service_is_unique = true
        else
            error("Unexpected argument to '", service_name, "': \"", service_arg, "\"")
        end
    end

    # Extract any type parameters for the service.
    service_type_params = nothing
    if @capture(service_name, generic_{types__})
        service_name = generic
        service_type_params = types
    end

    # Check the arguments have correct syntax.
    if service_name isa AbstractString
        service_name = Symbol(service_name)
    elseif !isa(service_name, Symbol)
        error("Service's name should be a simple token (or string), but it's '", service_name, "'")
    end
    if !Base.is_expr(service_definitions, :block)
        error("Expected a begin...end block for service's definitions; got: ",
              (service_definitions isa Expr) ?
                  service_definitions.head :
                  typeof(service_definitions))
    else
        service_definitions = filter(e -> !isa(e, LineNumberNode), service_definitions.args)
    end

    # Generate some basic code/snippets.
    expr_service_name = esc(service_name)
    expr_struct_name = esc(Symbol(:Service_, service_name))
    expr_struct_decl = exists(service_type_params) ?
                           :( $expr_struct_name{$(service_type_params...)} ) :
                           expr_struct_name
    function_prefix::Symbol = Symbol(:service_, service_name)
    expr_function_name_init = esc(Symbol(function_prefix, :_init))
    expr_function_name_shutdown = esc(Symbol(function_prefix, :_shutdown))
    expr_function_name_get = esc(function_prefix)
    expr_function_name_exists = esc(Symbol(function_prefix, :_exists))
    # If the service is unique, then we don't need the user to pass it in for every call,
    #    because we can grab it from the context.
    expr_local_service_var = Symbol("IMPL: SERVICE")
    expr_function_args_with_service(other_args) = if service_is_unique
                                                      other_args
                                                  else
                                                      vcat(:( $expr_local_service_var::$expr_struct_name ),
                                                           other_args)
                                                  end
    expr_function_body_get_service = if service_is_unique
                                         :( $expr_local_service_var::$expr_struct_name = $expr_function_name_get() )
                                     else
                                         :( )
                                     end

    # Parse the individual definitions.
    expr_struct_declarations = [ ]
    expr_global_declarations = [ ]
    # For simplicity, user-code escaping will be done *after* the loop.
    for definition in service_definitions
        # Is it a function definition?
        if is_function_expr(definition)
            function_data = SplitDef(definition)

            if function_data.name == :INIT
                # Define the service's constructor to implement the user's logic.
                constructor_data = deepcopy(function_data)
                constructor_data.name = expr_struct_name
                expr_constructor = combinedef(constructor_data)
                push!(expr_struct_declarations, func_def_to_call(expr_constructor))

                # Define the interface function for creating the context.
                init_function_data = deepcopy(function_data)
                init_function_data.name = expr_function_name_init
                init_function_data.body = quote
                    context = $(@__MODULE__).get_context()

                    serv = $expr_invoke_constructor
                    if $service_is_unique && haskey(context.unique_service_lookup, typeof(serv))
                        $expr_function_name_shutdown(serv, false)
                        error("Service has already been initialized: ", $(string(service_name)))
                    end

                    push!(context.services, serv)
                    if $service_is_unique
                        context.unique_service_lookup[typeof(serv)] = serv
                    end

                    return serv
                end
                push!(expr_global_declarations, combinedef(init_function_data))
            elseif function_data.name == :SHUTDOWN
                # Define an internal function that implements the user's logic.
                impl_shutdown_func_data = deepcopy(function_data)
                impl_shutdown_func_data.name = Symbol(function_prefix, :_shutdown_IMPL)
                push!(expr_global_declarations, combinedef(impl_shutdown_func_data))

                # Define the callback for when a Context is closing.
                push!(expr_global_declarations, :(
                    function $(@__MODULE__).service_internal_shutdown(service::$expr_struct_name)
                        $(impl_shutdown_func_data.name)(service, true)
                    end
                ))

                # Define the interface for manually shutting down the service.
                manual_shutdown_func_data = deepcopy(function_data)
                manual_shutdown_func_data.name = expr_function_name_shutdown
                if service_is_unique # Is the 'service' argument implicit?
                    deleteat!(manual_shutdown_func_data.args, 1)
                end
                manual_shutdown_func_data.body = quote
                    $expr_function_body_get_service
                    return $(impl_shutdown_func_data.name)($expr_local_service_var, false)
                end
                push!(expr_global_declarations, combinedef(manual_shutdown_func_data))
            elseif function_data.name == :REFRESH
                refresh_func_data = deepcopy(function_data)
                refresh_func_data.name = :( $(@__MODULE__).service_internal_refresh)
                push!(expr_global_declarations, combinedef(refresh_func_data))
            else
                # Must be a custom function.
                # Our generated boilerplate will internally define then invoke
                #    the user's literal written function.

                function_as_call = func_def_to_call(definition)
                function_as_call.args[2] = expr_local_service_var

                push!(expr_global_declarations, combinedef(SplitDef(
                    function_data.name,
                    expr_function_args_with_service(function_data.args[2:end]),
                    function_data.kw_args,
                    quote
                        # Define the users's function locally.
                        $definition

                        # Get the service if it's passed implicitly.
                        $expr_function_body_get_service

                        # Invoke the user's function.
                        return $function_as_call
                    end,
                    ()
                )))
            end
        else
            # Must be a field.
            (field_name, field_type, is_splat_operator, field_default_value) = splitarg(definition)
            if is_splat_operator || exists(field_default_value)
                error("Invalid syntax for field '", field_name, "' (did you try to provide a default value?)")
            end
            push!(expr_struct_declarations, definition)
        end
    end

    # Generate a getter for unique services.
    if service_is_unique
        push!(expr_global_declarations, :(
            @inline function $expr_function_name_get(type_params...)::$expr_struct_name
                context = $(@__MODULE__).get_context()
                service_type = if isempty(type_params)
                                   $expr_struct_name
                               else
                                   $expr_struct_name{type_params...}
                               end
                return context.unique_service_lookup[service_type]
            end
        ))
    end

    # Generate a function to check if the service exists.
    if service_is_unique
        push!(expr_global_declarations, :(
            @inline function $expr_function_name_exists(type_params...)::$expr_struct_name
                context = $(@__MODULE__).get_context()
                service_type = if isempty(type_params)
                                   $expr_struct_name
                               else
                                   $expr_struct_name{type_params...}
                               end
                return haskey(context.unique_service_lookup, service_type)
            end
        ))
    end

    # Generate the final code.
    expr_struct_declarations = esc.(expr_struct_declarations)
    expr_global_declarations = esc.(expr_global_declarations)
    return quote
        # The service data structure:
        Core.@__doc__ mutable struct $expr_struct_decl <: $(@__MODULE__).AbstractService
            $(expr_struct_declarations)
        end

        $(expr_global_declarations...)

        # The singleton getter:
        if $service_is_unique
            function $expr_function_name_get()::$expr_struct_name
                context = $(@__MODULE__).get_context()
                return context.unique_service_lookup[$expr_struct_name]
            end
        end
    end
end

export AbstractService, @bp_service