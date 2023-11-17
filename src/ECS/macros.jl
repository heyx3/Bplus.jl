# An internal interface is used to reference inherited behavior from parent components.
#    Metadata:
component_macro_fields(  T::Type{<:AbstractComponent}) = [f=>fieldtype(f, T) for f in fieldnames(T)]
component_macro_init_parent_args(::Type{<:AbstractComponent}) = (args = [], kw_args=[]) # Both elements are vecotrs of SplitDef
#    Standard events:
component_macro_init(       ::Type{<:AbstractComponent}, ::AbstractComponent, args...; kw_args...) = error()
component_macro_cleanup(    ::Type{<:AbstractComponent}, ::AbstractComponent, ::Bool) = error()
component_macro_tick(       ::Type{<:AbstractComponent}, ::AbstractComponent        ) = error()
component_macro_finish_tick(::Type{<:AbstractComponent}, ::AbstractComponent        ) = error()
#    Promises:
component_macro_all_promises(::Type{<:AbstractComponent}) = () # Symbols
component_macro_unimplemented_promises(::Type{<:AbstractComponent}) = () # Symbols
component_macro_implements_promise(::Type{<:AbstractComponent}, ::Val)::Bool = error()
component_macro_promise_data(::Type{<:AbstractComponent}, ::Val)::SplitDef = error()
component_macro_promise_execute(::Type{<:AbstractComponent}, ::AbstractComponent, ::Val,
                                args...; kw_args...) = error()
#    Configurables:
component_macro_overridable_configurables(::Type{<:AbstractComponent}) = () # Symbols
component_macro_overrides_configurable(::Type{<:AbstractComponent}, ::Val)::Bool = error()
component_macro_configurable_data(::Type{<:AbstractComponent}, ::Val)::SplitDef = error()
component_macro_configurable_execute(::Type{<:AbstractComponent},::AbstractComponent, ::Val,
                                     args...; kw_args...) = error()

macro component(title, elements...)
    # Parse the component's name and parent type.
    title_is_valid = @capture(title, name_Symbol)
    local supertype_t
    if title_is_valid
        supertype_t = nothing
    else
        title_is_valid = @capture(title, name_<:parent_)
        supertype_t = parent
    end
    if !title_is_valid
        error("Invalid component name. Must be a name or 'name<:supertype'. Got '", title, "'")
    end
    component_name = name

    # We need to know about the supertype at compile-time.
    if exists(supertype_t)
        supertype_t = __module__.eval(supertype_t)
    end

    # Extract the other parts of the declaration.
    attributes = elements[1:(end-1)]
    if length(elements) < 1
        error("No body found for the component '", component_name, "'")
    end
    statements = (elements[end]::Expr).args

    return macro_impl_component(component_name, supertype_t, attributes, statements)
end

function macro_impl_component(component_name::Symbol, supertype_t::Optional{Type},
                              attributes, statements)
    if exists(supertype_t) && !(supertype_t <: AbstractComponent)
        error("Component '", component_name, "'s parent type '", supertype_t, "' is not a component itself!")
    end
    component_name_quote = QuoteNode(component_name)

    # Parse the attributes.
    is_abstract::Bool = false
    is_entity_singleton::Bool = false
    is_world_singleton::Bool = false
    requirements::Vector = [ ]
    for attribute in attributes
        if @capture(attribute, {abstract})
            if is_abstract
                @warn "{abstract} attribute given more than once, which is redundant"
            end
            is_abstract = true
        elseif @capture(attribute, {entitySingleton})
            if is_entity_singleton
                @warn "{entitySingleton} attribute given more than once, which is redundant"
            end
            if is_world_singleton
                @warn "{entitySingleton} is redundant when {worldSingleton} is already given"
            else
                is_entity_singleton = true
            end
        elseif @capture(attribute, {worldSingleton})
            if is_entity_singleton
                @warn "{entitySingleton} is redundant when {worldSingleton} is already given"
                is_entity_singleton = false
            end
            if is_world_singleton
                @warn "{worldSingleton} given more than once, which is redundant"
            else
                is_world_singleton = true
            end
        elseif @capture(attribute, {require:a_, b__})
            requirements = [a, b...]
        else
            error("Unexpected attribute: ", attribute)
        end
    end

    # Inherit things from the supertype.
    if exists(supertype_t)
        if is_entitysingleton_component(supertype_t)
            if is_entity_singleton
                @warn "Component $component_name already inherits {entitySingleton}, so its own attribute is redundant"
            else
                is_entity_singleton = true
            end
        end

        if is_worldsingleton_component(supertype_t)
            if is_world_singleton
                @warn "Component $component_name already inheritis {worldSingleton}, so its own attribute is redundant"
            else
                is_world_singleton = true
            end
            if is_entity_singleton
                @warn "Component $component_name has both {entitySingleton} and {worldSingleton} (at least one through inheritance), which is redundant"
            end
        end

        # Append the parent requirements after the child's
        #    (because the child's requirements are probably more specific, to satisfy the parent).
        append!(requirements, :( $(@__MODULE__).require_components($supertype_t)... ))
    end
    if isnothing(supertype_t)
        supertype_t = AbstractComponent
    end

    # Get the full chain of component inheritance.
    all_supertypes_youngest_first = [ component_name, get_component_types(supertype_t)... ]
    all_supertypes_oldest_first = reverse(all_supertypes_youngest_first)
    if !all(isabstracttype, drop_last(all_supertypes_oldest_first))
        error("You may only inherit from abstract components due to Julia's type system")
    end

    # Take fields from the parent.
    field_data = Vector{Tuple{Symbol, Any}}()
    if supertype_t != AbstractComponent
        append!(field_data, component_macro_fields(supertype_t))
    end

    # Parse the declarations.
    constructor::Optional{@NamedTuple{args::Vector{SplitArg}, kw_args::Vector{SplitArg}, body}} = nothing
    destructor::Optional{@NamedTuple{b_name::Symbol, body}} = nothing
    defaultor = nothing # Just the body
    tick = nothing # Just the body
    finish_tick = nothing # Just the body
    new_promises = Vector{Tuple{LineNumberNode, SplitDef}}() # Expected to have no body
    new_configurables = Vector{Tuple{LineNumberNode, SplitDef}}() # Expected to have a body
    implemented_promises = Vector{SplitDef}()
    implemented_configurables = Vector{SplitDef}()
    for statement in statements
        # Strip out the doc-string so we can identify the underlying statement.
        # We'll put it back in later.
        doc_string::Optional{AbstractString} = nothing
        if isexpr(statement, :macrocall) && (statement.args[1] == GlobalRef(Core, Symbol("@doc")))
            doc_string = statement.args[3]
            statement = statement.args[4]
        end

        # Figure out what this statement is and respond accordingly.
        if is_function_decl(statement)
            func_data = SplitDef(statement)
            func_data.doc_string = doc_string

            if func_data.name == :CONSTRUCT
                if exists(constructor)
                    error("More than one CONSTRUCT() provided!")
                elseif exists(func_data.return_type) || func_data.generated
                    error("CONSTRUCT() should be a simple function, and with no return type")
                elseif any(arg -> arg.is_splat, @view func_data.args[1:(end-1)])
                        error("CONSTRUCT() has invalid arguments; splat ('a...') must come at the end",
                              " of the ordered parameters")
                end
                constructor = (
                    args=copy(func_data.args),
                    kw_args=copy(func_data.kw_args),
                    body=func_data.body
                )
            #TODO: CONSTRUCT_MANUAL(), has to manually invoke BASE() to get base-class construction behavior
            elseif func_data.name == :DESTRUCT
                if exists(destructor)
                    error("More than one DESTRUCT() provided!")
                elseif length(func_data.args) > 1
                    error("DESTRUCT() should have at most one argument ('entity_is_dying::Bool').",
                          " Got: '", combinecall(func_data), "'")
                elseif (length(func_data.args) == 1) && let a = func_data.args[1]
                                                              !isa(a.name, Symbol) ||
                                                              a.is_splat ||
                                                              exists(a.default_value)
                                                         end
                    error("The single parameter to DESTRUCT should be something like ",
                          "'is_entity_dying::Bool'. Got: '", func_data.args[1], "'")
                elseif exists(func_data.return_type) || func_data.generated || !isempty(func_data.where_params)
                    error("DESTRUCT() should be a simple function, and with no return type")
                end
                destructor = (
                    b_name=isempty(func_data.args) ?
                             Symbol("IGNORED:entity_is_dying") :
                             func_data.args[1],
                    body=func_data.body
                )
            elseif func_data.name == :DEFAULT
                if exists(defaultor)
                    error("DEFAULT() provided more than once")
                elseif !isempty(func_data.args) || !isempty(func_data.kw_args)
                    error("DEFAULT() should have no arguments")
                elseif !isempty(func_data.where_params) || func_data.generated
                    error("DEFAULT() should be a simple function, no type params or @generated")
                end
                defaultor = Some(func_data.body)
            elseif func_data.name == :TICK
                if exists(tick)
                    error("TICK() provided more than once")
                elseif !isempty(func_data.args) || !isempty(func_data.kw_args)
                    error("TICK() should have no arguments")
                elseif !isempty(func_data.where_params) || func_data.generated
                    error("TICK() should be a simple function, no type params or @generated")
                end
                tick = func_data.body
            elseif func_data.name == :FINISH_TICK
                if exists(finish_tick)
                    error("FINISH_TICK() provided more than once")
                elseif !isempty(func_data.args) || !isempty(func_data.kw_args)
                    error("FINISH_TICK() should have no arguments")
                elseif !isempty(func_data.where_params) || func_data.generated
                    error("FINISH_TICK() should be a simple function, no type params or @generated")
                end
                finish_tick = func_data.body
            elseif func_data.name in component_macro_unimplemented_promises(supertype_t)
                push!(implemented_promises, func_data)
            elseif func_data.name in component_macro_overridable_configurables(supertype_t)
                push!(implemented_configurables, func_data)
            end
        elseif @capture(statement, fieldName_Symbol) || @capture(statement, fieldName_::fieldType_)
            if fieldName in (:entity, :world)
                error("Can't name a component field 'this', 'entity', or 'world'")
            end
            if exists(doc_string)
                #TODO: Append field doc-strings into the component's somehow. Include parent fields' doc-strings.
            end
            push!(field_data, (fieldName, isnothing(fieldType) ? Any : fieldType))
        elseif is_macro_invocation(statement)
            macro_data = SplitMacro(statement)
            if macro_data.name == :promise
                if !is_abstract
                    error("Only {abstract} components can @promise things")
                elseif (length(macro_data.args) != 1) || !is_function_decl(macro_data.args[1])
                    error("A @promise should include a single function signature")
                end

                func_data = SplitDef(:( $(macro_data.args[1]) = nothing ))
                func_data.doc_string = doc_string
                if func_data.name in (component_macro_unimplemented_promises(supertype_t)..., component_macro_overridable_configurables(supertype_t))
                    error("@promise hides inherited @promise or @configurable: '", func_data.name, "'")
                end

                push!(new_promises, (
                    macro_data.source,
                    func_data
                ))
            elseif macro_data.name == :configurable
                if (length(macro_data.args) != 1) || !is_function_decl(macro_data.args[1])
                    error("A @configurable should include a single function definition")
                end

                func_data = SplitDef(macro_data.args[1])
                func_data.doc_string = doc_string
                if func_data.name in (component_macro_unimplemented_promises(supertype_t)..., component_macro_overridable_configurables(supertype_t))
                    error("@configurable hides inherited @promise or @configurable: '", func_data.name, "'")
                end

                push!(new_configurables, (
                    macro_data.source,
                    SplitDef(:( $(macro_data.args[1]) = nothing ))
                ))
            else
                error("Unexpected macro in component '$component_name': $statement")
            end
        elseif statement isa LineNumberNode
            # Ignore it.
            #TODO: Is every statement preceded by a LineNumberNode? If so, pass them through (e.x. to implemented_promises/configurables)
        else
            error("Unexpected syntax in body of component '$component_name': \"$statement\"")
        end
    end

    # Post-process the statement data.
    if isnothing(constructor)
        constructor = (
            args = [ ],
            kw_args = [ ],
            body = nothing
        )
    end
    if isnothing(destructor)
        destructor = (
            b_name=Symbol("UNUSED: entity is dying?"),
            body=nothing
        )
    end
    #TODO: Check that the implemented promises and configurations kept the same signature.

    # Process promise data for code generation.
    all_promises::Vector{Symbol} = [
        component_macro_all_promises(supertype_t)...,
        (tup[2].name for tup in new_promises)...
    ]
    # If some promises are missing, and this component isn't abstract, throw an error.
    missing_promises = filter(p_name -> none(p -> p.name == p_name, implemented_promises),
                              component_macro_unimplemented_promises(supertype_t))
    if !isempty(missing_promises) && !is_abstract
        error("Component '$component_name' doesn't follow through on some promises: [ ",
                  join(missing_promises, ", "), "]")
    end

    # Process configurable data for code generation.
    #TODO: Allow for marking configurables as "final", meaning they can't be overridden by child components.
    all_configurables::Vector{Symbol} = [
        component_macro_overridable_configurables(supertype_t)...,
        (tup[2].name for tup in new_configurables)...
    ]

    # In the constructor, each successive parent type takes some of the first ordered arguments,
    #    and each successive child type takes some of the named arguments.
    # Figure out what is taken of each argument type.
    parent_arg_ranges = Vector{UnitRange{Int}}() # Range end of -1 means "slurp"
    parent_kwargs = Vector{Optional{Set{Symbol}}}() # Nothing means "slurp"
    for parent_t in all_supertypes_oldest_first
        local constructor_arg_data
        if parent_t == component_name
            constructor_arg_data = (
                args = constructor.args,
                kw_args = constructor.kw_args
            )
        else
            constructor_arg_data = component_macro_init_parent_args(parent_t)
            if !isempty(constructor.args) &&
               any(a::SplitArg -> exists(a.default_value), constructor_arg_data.args)
            #begin
                error("Parent '", parent_t, "' of component '", component_name, "' has optional arguments, ",
                      "so '", component_name, "' can't have any arguments ",
                      "(otherwise how do we decide where to route them?)")
            end
        end
        take_all_args::Bool = (!isempty(constructor_arg_data.args) &&
                                constructor_arg_data.args[end].is_splat) ||
                              any(a -> exists(a.default_value), constructor_arg_data.args)

        # Calculate ordered arguments.
        arg_start = isempty(parent_arg_ranges) ? 1 : (last(parent_arg_ranges[end]) + 1)
        arg_size = length(constructor_arg_data.args)
        arg_end = take_all_args ? -1 : (arg_start + arg_size - 1)
        arg_range = arg_start:arg_end
        if !isempty(arg_range) && any(r -> last(r) < first(r), parent_arg_ranges)
            error("A parent type of component $component_name slurps ordered arguments in the constructor,",
                  " so no child type can have any ordered arguments in *its* constructor")
        end
        push!(parent_arg_ranges, arg_range)

        # Calculate named arguments.
        # Any arguments named by the child are left out of the parents.
        # This means if the child is slurping all kw-args, *nothing* passes on to the parents.
        #TODO: Allow annotating a kw-arg to also pass through to the parents.
        if any(ka -> ka.is_splat, constructor_arg_data.kw_args)
            for i in 1:length(parent_kwargs)
                parent_kwargs[i] = Set{Symbol}()
            end
            push!(parent_kwargs, nothing)
        else
            new_kw_args = Set{Symbol}()
            for kw_data::SplitArg in constructor_arg_data.kw_args
                push!(new_kw_args, kw_data.name)
                # Remove the keyword param from any parent constructor calls.
                for parent_kwarg in parent_kwargs
                    if exists(parent_kwarg)
                        delete!(parent_kwarg, kw_name)
                        if parent_t == component_name
                            @info "Component $component_name's constructor parameter '$kw_name' will not pass on to a parent who could have taken it"
                        end
                    end
                end
            end
            push!(parent_kwargs, new_kw_args)
        end
    end
    # Generate calls to each supertype's constructor, from oldest to newest.
    constructor_calls = [ ]
    for (parent_t, arg_range, kw_set) in zip(all_supertypes_oldest_first,
                                             parent_arg_ranges,
                                             parent_kwargs)

        range_start = first(arg_range)
        range_end = (last(arg_range) < first(arg_range)) ? :end : last(arg_range)
        keyword_gets = [ ]
        for kw_name in kw_set
            push!(keyword_gets, :(
                (haskey(kw_args, $(QuoteNode(kw_name))) ?
                     ($kw_name=kw_args[kw_name], ) :
                     ()
                )...
            ))
        end
        push!(constructor_calls, :(
            $(@__MODULE__).component_macro_init($(esc(parent_t)), this,
                                                args[$range_start : $range_end]...
                                                ; $(keyword_gets...))
        ))
    end

    final_decls = [ ]

    # Generate the type definition.
    push!(final_decls, if is_abstract
        :( Core.@__doc__ abstract type $(esc(component_name)) <: $supertype_t end )
    else
        :(
            Core.@__doc__ mutable struct $(esc(component_name)) <: $supertype_t
                entity::Entity
                world::World

                $(( :( $f::$t ) for (f, t) in field_data )...)

                $component_name(entity, world) = let this = new()
                    this.entity = entity
                    this.world = world
                    this
                end
            end
        )
    end)

    # Implement the internal macro interface.
    push!(final_decls, quote
        $(@__MODULE__).component_macro_fields(::Type{$(esc(component_name))}) = $(tuple(
            n => T for (n, T) in field_data
        ))
        $(@__MODULE__).component_macro_init_parent_args(::Type{$(esc(component_name))}) = (
            n_args = $(if any(a -> a.is_splat, constructor.args)
                           -1
                       else
                           length(constructor.args)
                       end),
            kw_args = $(Tuple(constructor.kw_args))
        )
        $(@__MODULE__).component_macro_init(::Type{$(esc(component_name))}, $(esc(:this))::$(esc(component_name)),
                                            $((esc(combinearg(a)) for a in constructor.args)...)
                                            ; $((esc(combinearg(a)) for a in constructor.kw_args)...)) = begin
            $(esc(:entity))::Entity = $(esc(:this)).entity
            $(esc(:world))::World = $(esc(:this)).world
            $(esc(constructor.body))
        end
        $(@__MODULE__).component_macro_cleanup(::Type{$(esc(component_name))}, $(esc(:this))::$(esc(component_name)),
                                               $(destructor.b_name)::Bool) = begin
            $(esc(:entity))::Entity = $(esc(:this)).entity
            $(esc(:world))::World = $(esc(:this)).world
            $(esc(destructor.body))
        end
        $(@__MODULE__).component_macro_tick(::Type{$(esc(component_name))}, $(esc(:this))::$(esc(component_name))) = begin
            $(esc(:entity))::Entity = $(esc(:this)).entity
            $(esc(:world))::World = $(esc(:this)).world
            $(esc(tick))
        end
        $(@__MODULE__).component_macro_finish_tick(::Type{$(esc(component_name))}, $(esc(:this))::$(esc(component_name))) = begin
            $(esc(:entity))::Entity = $(esc(:this)).entity
            $(esc(:world))::World = $(esc(:this)).world
            $(esc(finish_tick))
        end

        # Promises
        $(@__MODULE__).component_macro_all_promises(::Type{$(esc(component_name))}) = tuple($(QuoteNode.(all_promises)...))
        $(@__MODULE__).component_macro_unimplemented_promises(::Type{$(esc(component_name))}) = $(Tuple(missing_promises))
        $(map(implemented_promises) do p; :(
            $(@__MODULE__).component_macro_implements_promise(::Type{<:$(esc(component_name))}, ::Val{$(QuoteNode(p.name))}) = true
        ) end...)
        $(map(new_promises) do (source, def); :(
            $(@__MODULE__).component_macro_promise_data(::Type{<:$(esc(component_name))}, ::Val{$(QuoteNode(def.name))}) = $def
        ) end...)
        $(map(implemented_promises) do def
            promise_name = def.name
            name_quote = QuoteNode(promise_name)

            # The implementation actually goes into one of our internal macro interface functions.
            def = SplitDef(def)
            def.name = :( $(@__MODULE__).component_macro_promise_execute )
            def.args = vcat(SplitArg.([ :( ::Type{<:$(esc(component_name))}),
                                        :( this::$(esc(component_name)) ),
                                        :( ::Val{$name_quote} ) ]),
                            def.args)

            # Provide access to SUPER(), which invokes the supertype's implementation.
            first_implementing_parent_idx = findfirst(
                t -> component_macro_implements_promise(t, Val(promise_name)),
                Iterators.drop(all_supertypes_youngest_first, 1)
            )
            def.body = :(
                let SUPER = if $(exists(first_implementing_parent_idx))
                                @inline (args2...; kw_args2...) -> if isempty(args2) && isempty(kw_args2)
                                    $(@__MODULE__).component_macro_promise_execute(
                                        $supertype_t, this, Val($name_quote),
                                        args...; kw_args...
                                    )
                                else
                                    $(@__MODULE__).component_macro_promise_execute(
                                        $supertype_t, this, Val($name_quote),
                                        args2...; kw_args2...
                                    )
                                end
                            else
                                (a...; b...) -> error("No supertype implementation exists for ",
                                                      promise_name)
                            end
                  $(def.body)
                end
            )

            # If the original promise specified a return type, explicitly check for that.
            promised_return_type = component_macro_promise_data(supertype_t, promise_name).return_type
            if exists(promised_return_type)
                def.body = :( let result = $(def.body)
                    if !isa(result, $promised_return_type)
                        error("$component_name.$promise_name doesn't return a $promised_return_type!",
                              " It returned a ", typeof(result))
                    else
                        result
                    end
                end )
            end

            # Set up the local variables 'entity' and 'world'.
            def.body = :( let entity = this.entity,
                              world = this.world
                            $(def.body)
                        end )

            def.body = esc(def.body)
            return combinedef(def)
        end...)

        # Configurables
        $(@__MODULE__).component_macro_overridable_configurables(::Type{$(esc(component_name))}) = $(Tuple(all_configurables))
        $(map(implemented_configurables) do def; :(
            $(@__MODULE__).component_macro_overrides_configurable(::Type{$(esc(component_name))}, ::Val{$(QuoteNode(def.name))}) = true
        ) end...)
        $(map(new_configurables) do (source, def); :(
            $(@__MODULE__).component_macro_configurable_data(::Type{$(esc(component_name))}, ::Val{$(QuoteNode(def.name))}) = $def
        ) end...)
        $(map([implemented_configurables..., (tup[2] for tup in new_configurables)...]) do def
            configurable_name = def.name
            name_quote = QuoteNode(configurable_name)

            # The implementation actually goes into one of our internal macro interface functions.
            def = SplitDef(def)
            def.name = :( $(@__MODULE__).component_macro_configurable_execute )
            def.args = vcat(SplitArg.([ :( ::Type{<:$(esc(component_name))}),
                                        :( this::$(esc(component_name)) ),
                                        :( ::Va{$name_quote} ) ]),
                            def.args)

            # Provide access to SUPER(), which invokes the supertype's implementation.
            first_implementing_parent_idx = findfirst(
                t -> component_macro_implements_promise(t, Val(promise_name)),
                Iterators.drop(all_supertypes_youngest_first, 1)
            )
            def.body = :(
                let SUPER = if $(exists(first_implementing_parent_idx))
                                @inline (args2...; kw_args2...) -> if isempty(args2) && isempty(kw_args2)
                                    $(@__MODULE__).component_macro_configurable_execute(
                                        $supertype_t, this, Val($name_quote),
                                        args...; kw_args...
                                    )
                                else
                                    $(@__MODULE__).component_macro_configurable_execute(
                                        $supertype_t, this, Val($name_quote),
                                        args2...; kw_args2...
                                    )
                                end
                            else
                                (a...; b...) -> error("No supertype implementation exists for ",
                                                      configurable_name)
                            end
                  $(def.body)
                end
            )

            # If the original specified a return type, explicitly check for that.
            configured_return_type = component_macro_configurable_data(supertype_t, configurable_name).return_type
            if exists(configured_return_type)
                def.body = :( let result = $(def.body)
                    if !isa(result, $configured_return_type)
                        error("$component_name.$configurable_name() doesn't return a $configured_return_type",
                              ". It returned a ", typeof(result))
                    else
                        result
                    end
                end )
            end

            # Set up the local variables 'entity' and 'world'.
            def.body = :( let entity = this.entity,
                              world = this.world
                            $(def.body)
                        end )

            def.body = esc(def.body)
            return combinedef(def)
        end...)
    end)

    # Implement promises and configurables as properties.
    push!(final_decls, quote
        @inline $Base.getproperty(c::$(esc(component_name)), name::Symbol) = getproperty(c, Val(name))
        @inline $Base.getproperty(c::$(esc(component_name)), ::Val{Name}) where {Name} = getfield(c, Name)
        @inline $Base.propertynames(c::$(esc(component_name))) = tuple(
            $((QuoteNode(f) for (f, _) in field_data)...),
            $((QuoteNode(p.name) for p in all_promises)...),
            $((QuoteNode(c.name) for c in all_configurables)...)
        )

        $(map(implemented_promises) do promise
            quoted = QuoteNode(promise.name)
            return :(
                @inline $Base.getproperty(c::$(esc(component_name)), ::Val{$quoted}) =
                    (args...; kw_args...) -> begin
                        $(@__MODULE__).component_macro_promise_execute(
                            $(esc(component_name)), c, Val($quoted),
                            args...; kw_args...
                        )
                    end
            )
        end...)

        $(map(implemented_configurables) do def
            quoted = QuoteNode(def.name)
            return :(
                @inline $Base.getproperty(c::$(esc(component_name)), ::Val{$quoted}) =
                    (args...; kw_args...) -> begin
                        $(@__MODULE__).component_macro_configurable_execute(
                            $(esc(component_name)), c, Val($quoted),
                            args...; kw_args...
                        )
                    end
            )
        end...)
    end)

    # Implement the external interface.
    push!(final_decls, quote
        $(@__MODULE__).is_entitysingleton_component(::Type{$(esc(component_name))})::Bool = $is_entity_singleton
        $(@__MODULE__).is_worldsingleton_component(::Type{$(esc(component_name))})::Bool = $is_world_singleton
        $(@__MODULE__).require_components(::Type{$(esc(component_name))}) = tuple($(esc.(requirements)...))
        $(@__MODULE__).create_component(::Type{$(esc(component_name))}, entity::$Entity, args...; kw_args...) = begin
            if $is_abstract
                if $(isnothing(defaultor))
                    error("Can't create a '", $(string(component_name)), "'; it's abstract")
                else
                    world = entity.world
                    return $defaultor
                end
            else
                this = $(esc(component_name))(entity, entity.world)
                $(constructor_calls...)
                return this
            end
        end
        $(@__MODULE__).destroy_component(c::$(esc(component_name)), e::$Entity, is_entity_dying::Bool) = begin
            @bp_ecs_assert(c.entity == e, "Given the wrong entity")
            $((
                :( $(@__MODULE__).component_macro_cleanup($(((parent_t isa Symbol) ? esc(parent_t) : parent_t)),
                                                          c, is_entity_dying) )
                  for parent_t in all_supertypes_youngest_first
            )...)
            return nothing
        end
        $(@__MODULE__).tick_component(c::$(esc(component_name)), e::$Entity) = begin
            @bp_ecs_assert(c.entity == e, "Given the wrong entity")
            $((
                :( $(@__MODULE__).component_macro_tick($parent_t, c) )
                  for parent_t in all_supertypes_youngest_first
            )...)
            $((
                :( $(@__MODULE__).component_macro_finish_tick($parent_t, c) )
                  for parent_t in all_supertypes_oldest_first
            )...)
            return nothing
        end
    end)

    final_expr = quote $(final_decls...) end
    @info "\n$(MacroTools.prettify(final_expr))\n\nCOMPONENT $component_name\n"
    return final_expr
end