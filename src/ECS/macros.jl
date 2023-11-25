# An internal interface is used to reference inherited behavior from parent components.
#    Metadata:
component_macro_fields(T::Type{<:AbstractComponent}) = error() # iteration of Pair{name, typeExpr}
component_macro_has_custom_constructor(::Type{<:AbstractComponent})::Bool = error()
#    Standard events:
component_macro_init(T::Type{<:AbstractComponent}, ::AbstractComponent,
                     entity::Entity, world::World,
                     args...; kw_args...) = error(
    "Arguments do not match in call: ", T, ".CONSTRUCT(",
        join(("::$(typeof(a))" for a in args), ", "),
        " ; ", join(("$n=::$(typeof(v))" for (n, v) in kw_args), ", "),
    ")"
)
component_macro_cleanup(    ::Type{<:AbstractComponent}, ::AbstractComponent, ::Bool) = error()
component_macro_tick(       ::Type{<:AbstractComponent}, ::AbstractComponent        ) = error()
component_macro_finish_tick(::Type{<:AbstractComponent}, ::AbstractComponent        ) = error()
#    Promises:
component_macro_all_promises(::Type{<:AbstractComponent}) = () # Symbols
component_macro_unimplemented_promises(::Type{<:AbstractComponent}) = () # Symbols
component_macro_implements_promise(::Type{<:AbstractComponent}, ::Val)::Bool = false
component_macro_promise_data(::Type{<:AbstractComponent}, ::Val)::SplitDef = error()
component_macro_promise_execute(T::Type{<:AbstractComponent}, ::AbstractComponent, v::Val,
                                args...; kw_args...) = error(
    "Arguments do not match promise: ", T, ".", val_type(v), "(",
        join(("::$(typeof(a))" for a in args), ", "),
        " ; ", join(("$n=::$(typeof(v))" for (n, v) in kw_args), ", "),
    ")"
)
#    Configurables:
component_macro_overridable_configurables(::Type{<:AbstractComponent}) = () # Symbols
component_macro_overrides_configurable(::Type{<:AbstractComponent}, ::Val)::Bool = false
component_macro_configurable_data(::Type{<:AbstractComponent}, ::Val)::SplitDef = error()
component_macro_configurable_execute(::Type{<:AbstractComponent},::AbstractComponent, v::Val,
                                     args...; kw_args...) = error(
    "Arguments do not match configurable: ", T, ".", val_type(v), "(",
        join(("::$(typeof(a))" for a in args), ", "),
        " ; ", join(("$n=::$(typeof(v))" for (n, v) in kw_args), ", "),
    ")"
)
#    Utilities:
component_field_print(component, field_name::Val, field_value                   , io) = print(io, field_value)
component_field_print(component, field_name::Val, field_value::AbstractComponent, io) = print(io, "<", typeof(field_value), ">")

"Set to 'true' to see the printout of new defined components"
PRINT_COMPONENT_CODE::Bool = false

"""
A very covenient way to define a component.

The basic syntax is this:

````
@component Name [<: Parent] [attributes...] begin
    field1::Float32
    field2

    # By default, a component is constructed by providing each of its fields, in order.
    # Parent-type's fields come before child-type's fields.
    # However, you can override this like so:
    function CONSTRUCT(f)
        # All functions within a @component can reference "this", "entity", and "world".
        this.field1 = Float32(f)
        this.field2 = length(world.entities)
    end
    function DESTRUCT() # Optionally take a Bool for whether the owning entity is being destroyed
        println("Delta entities: ", length(world.entities) - this.field2)
    end

    function TICK()
        this.field1 += world.delta_seconds
    end
    # FINISH_TICK() exists, but is only useful for abstract components (see below).
````

Attributes are enclosed in braces. Here are the supported ones:
  * `{abstract}`: this component is an abstract type (see below).
  * `{entitySingleton}`: only one of this component can exist on an entity.
  * `{worldSingleton}`: only one of this component can exist in the entire world.
  * `{require: a, b, c}`: other components must exist on this entity, and will be added if they aren't already

This macro also provides an Object-Oriented architecture, where abstract components can add
    fields, behavior, "Promises" (abstract functions) and "Configurables" (virtual functions)
    to their concrete children.
Here is a detailed example of an abstract component:

````
"Some kind of animated motion. Only one maneuver can run at a time."
@component Maneuver {abstract} {entitySingleton} {require: Position} begin
    pos_component::Position
    duration::Float32
    progress_normalized::Float32

    # Because this type has a custom constructor, all child types must have one too.
    # They also must invoke SUPER(...) exactly once, with these arguments.
    function CONSTRUCT(duration_seconds::Float32)
        this.progress_normalized = 0
        this.duration = duration_seconds
        this.pos_component = get_component(entity, Position)
    end

    # If you want to provide a default instance of this component,
    #    for when it's required but not already existing on an entity,
    #    you can implement this (pretend a child maneuver exists called 'TurningManeuver'):
    DEFAULT() = TurningManeuver(3.5, has_component(entity, PreferLeft) ? -90 : 90)

    # Non-abstract child components must implement this.
    # Abstract child components may choose to implement this for their children.
    # In the latter case, the concrete children can further override the behavior,
    #    and invoke `BASE()` to get the original parent implementation.
    @promise finish_maneuver(last_time_step::Float32)

    # Child components may choose to override this.
    # It must return a bool.
    # It indicates whether to cut off the maneuver early.
    # When overriding this, you can invoke BASE() to get the implementation of your parent.
    @conigurable should_stop()::Bool = false

    # This abstract component handles the timing for its children.
    function TICK()
        this.progress_normalized += world.delta_seconds / this.duration
    end
    # After all tick logic is done (including children), check whether the animation is finished.
    function FINISH_TICK()
        if this.should_stop()
            remove_component(entity, this)
        elseif this.progress_normalized >= 1
            # Handle any overshoot of the target, then destroy self:
            overshoot_seconds = (@f32(1) - this.progress_normalized) * this.duration
            this.finish_maneuver(-overshoot_seconds)
            remove_component(entity, this)
        end
    end
end
````

Finally, here is an example of `StrafingManeuver`, a child of `Maneuver`:

````
@component StrafingManeuver <: Maneuver {require: MovementSpeed} begin
    speed_component::MovementSpeed
    dir::v3f
    function CONSTRUCT(duration::Float32, dir::v3f)
        SUPER(duration)
        this.speed_component = get_component(entity, MovementSpeed)
        this.dir = dir
    end

    # Implement the maneuver's interface.
    finish_maneuver(last_time_step::Float32) =
        strafe(this, last_time_step) # Uses a helper function defined below
    should_stop() =
        do_collision_checking(entity) # Some function that checks for this entity colliding with geometry

    TICK() = strafe(this) # Uses a helper function defined below
end

# Helper function for applying the maneuver.
function strafe(s::StrafingManeuver, time_step)
    # In a normal Julia function, things are a bit less convenient.
    entity = s.entity
    world = s.world

    if isnothing(time_step)
        time_step = world.delta_seconds
    end
    s.pos_component.value += s.speed_component.value * time_step * s.dir
end
````
"""
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
            requirements = Any[a, b...]
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
        push!(requirements, :( $(@__MODULE__).require_components($supertype_t)... ))
    else
        supertype_t = AbstractComponent
    end

    # Get the full chain of component inheritance.
    all_supertypes_youngest_first = [ component_name, get_component_types(supertype_t)... ]
    all_supertypes_oldest_first = reverse(all_supertypes_youngest_first)
    if !all(isabstracttype, drop_last(all_supertypes_oldest_first))
        error("You may only inherit from abstract components due to Julia's type system")
    end

    # Take fields from the parent before adding our own.
    field_data = Vector{Pair{Symbol, Any}}()
    if supertype_t != AbstractComponent
        append!(field_data, component_macro_fields(supertype_t))
    end

    # Parse the declarations.
    constructor::Optional{SplitDef} = nothing
    destructor::Optional{@NamedTuple{b_name::Symbol, body}} = nothing
    defaultor::Optional{Tuple{Any, Any, Any}} = nothing # Concrete component type and its constructor args/kw-args
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
                constructor = func_data
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
                             func_data.args[1].name,
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
                if @capture(func_data.body, (concreteName_(args__; kw_args__)) | (concreteName_(args__)))
                    defaultor = (concreteName, args, isnothing(kw_args) ? [ ] : kw_args)
                else
                    error("DEFAULT() clause should be of the form 'ChildComponent(args...; kw_args...)'")
                end
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
            else
                error("Unexpected function in component: '", func_data.name, "'")
            end
        elseif @capture(statement, fieldName_Symbol) || @capture(statement, fieldName_::fieldType_)
            if fieldName in (:entity, :world)
                error("Can't name a component field 'entity', or 'world'")
            end
            push!(field_data, fieldName => (isnothing(fieldType) ? Any : fieldType))
        elseif is_macro_invocation(statement)
            macro_data = SplitMacro(statement)
            if macro_data.name == Symbol("@promise")
                if !is_abstract
                    error("Only {abstract} components can @promise things")
                elseif length(macro_data.args) != 1
                    error("A @promise should include a single function signature. ",
                            "Got ", length(macro_data.args), " expressions instead")
                elseif !is_function_call(macro_data.args[1])
                    error("A @promise should be a function signature. Got: ",
                              macro_data.args[1])
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
            elseif macro_data.name == Symbol("@configurable")
                if length(macro_data.args) != 1
                    error("A @configurable should include a single function definition. ",
                            "Got ", length(macro_data.args), " expressions instead")
                elseif !is_function_decl(macro_data.args[1])
                    error("A @configurable should be a function declaration. Got: ",
                            macro_data.args[1])
                end

                func_data = SplitDef(macro_data.args[1])
                func_data.doc_string = doc_string
                if func_data.name in (component_macro_unimplemented_promises(supertype_t)..., component_macro_overridable_configurables(supertype_t))
                    error("@configurable hides inherited @promise or @configurable: '", func_data.name, "'")
                end

                push!(new_configurables, (
                    macro_data.source,
                    func_data
                ))
                push!(implemented_configurables, func_data)
            else
                error("Unexpected macro in component '$component_name': $statement")
            end
        elseif statement isa LineNumberNode
            # Ignore it.
        else
            error("Unexpected syntax in body of component '$component_name': \"$statement\"")
        end
    end

    # Post-process the statement data.
    if isnothing(destructor)
        destructor = (
            b_name=Symbol("UNUSED: entity is dying?"),
            body=nothing
        )
    end


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
    all_configurables::Vector{Symbol} = [
        component_macro_overridable_configurables(supertype_t)...,
        (tup[2].name for tup in new_configurables)...
    ]

    # Look for duplicate property names: fields, promises, and configurables.
    property_name_counts = Dict{Symbol, Int}()
    for name in Iterators.flatten(((name for (name, _) in field_data),
                                   all_configurables,
                                   all_promises))
        if !haskey(property_name_counts, name)
            property_name_counts[name] = 0
        end
        property_name_counts[name] += 1
    end
    duplicate_property_names = [name for (name, count) in property_name_counts if (count > 2)]
    if !isempty(duplicate_property_names)
        error("Name collisions between fields, promises, and configurables: ",
                join(duplicate_property_names, ", "))
    end

    # Generate the type definition.
    type_decl = if is_abstract
        :( Core.@__doc__(abstract type $(esc(component_name)) <: $supertype_t end) )
    else
        :( Core.@__doc__(mutable struct $(esc(component_name)) <: $supertype_t
                entity::Entity
                world::World

                $(( esc(:( $f::$t )) for (f, t) in field_data )...)

                $component_name(entity, world) = let this = new()
                    this.entity = entity
                    this.world = world
                    this
                end
        end) )
    end

    global_decls = [ ]

    # Implement the internal macro interface.
    push!(global_decls, quote
        $(@__MODULE__).component_macro_fields(::Type{$(esc(component_name))}) = $(Tuple(field_data))
        $(@__MODULE__).component_macro_has_custom_constructor(::Type{$(esc(component_name))}) = $(exists(constructor))
        $(if isnothing(constructor)
          @bp_check((supertype_t == AbstractComponent) ||
                      !component_macro_has_custom_constructor(supertype_t),
                    "Parent type '", supertype_t, "' has a custom CONSTRUCT, so you need one too")
          :( @inline $(@__MODULE__).component_macro_init(::Type{$(esc(component_name))}, $(esc(:this))::$(esc(component_name)),
                                                         # 'entity' and 'world' are needed here
                                                         #    so that the user's parameters can reference them
                                                         #    as part of the default values.
                                                         $(esc(:entity))::$Entity, $(esc(:world))::$World,
                                                         $(esc.(name for (name, type) in field_data)...)
                                                        ) = begin
                $(map(field_data) do (name, type); :(
                    $(esc(:( this.$name = convert($type, $name) )))
                ) end...)

                # Note that all fields, including inherited ones, are handled by the above loop.
                # No need to call into a parent constructor.
            end
        ) else :(
            @inline $(@__MODULE__).component_macro_init(::Type{$(esc(component_name))}, $(esc(:this))::$(esc(component_name)),
                                                        # 'entity' and 'world' are needed here
                                                        #    so that the user's parameters can reference them
                                                        #    as part of the default values.
                                                        $(esc(:entity))::$Entity, $(esc(:world))::$World,
                                                        $((esc(combinearg(a)) for a in constructor.args)...)
                                                        ; $((esc(combinearg(a)) for a in constructor.kw_args)...)) = begin

                # If there is a parent component type, offer SUPER() and count how many times it's invoked.
                if $(supertype_t != AbstractComponent)
                    n_super_calls::Int = 0
                    @inline $(esc(:SUPER))(args...; kw_args...) = begin
                        n_super_calls += 1
                        $(@__MODULE__).component_macro_init($supertype_t,
                                                            $(esc(:this)), $(esc(:entity)), $(esc(:world)),
                                                            args...; kw_args...)
                    end
                end

                $(esc(constructor.body))

                # Make sure SUPER was called exactly once, if it was available.
                if $(supertype_t != AbstractComponent) && (n_super_calls != 1)
                    error("Your custom constructor for '", $(string(component_name)),
                            "' should call SUPER exactly once, but it called it ",
                            n_super_calls, " times")
                end
            end
        ) end )
        $(@__MODULE__).component_macro_cleanup(::Type{$(esc(component_name))}, $(esc(:this))::$(esc(component_name)),
                                               $(esc(destructor.b_name))::Bool) = begin
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
        $(@__MODULE__).component_macro_unimplemented_promises(::Type{$(esc(component_name))}) = $(tuple(
            missing_promises...,
            (p[2].name for p in new_promises)...
        ))
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
            def.args = vcat(SplitArg.([ :( ::Type{<:$((component_name))}),
                                        :( $((:this))::$((component_name)) ),
                                        :( ::Val{$name_quote} ) ]),
                            def.args)

            # Provide access to SUPER(), which invokes the supertype's implementation.
            first_implementing_parent_idx = findfirst(
                t -> component_macro_implements_promise(t, Val(promise_name)),
                enumerate_as_pair(Iterators.drop(all_supertypes_youngest_first, 1))
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
            promised_return_type = if supertype_t == AbstractComponent
                nothing
            else
                component_macro_promise_data(supertype_t, Val(promise_name)).return_type
            end
            if exists(promised_return_type)
                def.body = :( let result = $(def.body)
                    if !isa(result, $promised_return_type)
                        error($(string(component_name)), ".",
                                $(string(promise_name)), " doesn't return a ",
                                $(string(promised_return_type)),
                              "! It returned a ", typeof(result))
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

            def.body = def.body
            return esc(combinedef(def))
        end...)

        # Configurables
        $(@__MODULE__).component_macro_overridable_configurables(::Type{$(esc(component_name))}) = $(Tuple(all_configurables))
        $(map(implemented_configurables) do def; :(
            $(@__MODULE__).component_macro_overrides_configurable(::Type{$(esc(component_name))}, ::Val{$(QuoteNode(def.name))}) = true
        ) end...)
        $(map(new_configurables) do (source, def); :(
            $(@__MODULE__).component_macro_configurable_data(::Type{$(esc(component_name))}, ::Val{$(QuoteNode(def.name))}) = $def
        ) end...)
        $(map([implemented_configurables...]) do def
            configurable_name = def.name
            name_quote = QuoteNode(configurable_name)

            # The implementation actually goes into one of our internal macro interface functions.
            def = SplitDef(def)
            def.name = :( $(@__MODULE__).component_macro_configurable_execute )
            def.args = vcat(SplitArg.([ :( ::Type{<:$((component_name))}),
                                        :( $((:this))::$((component_name)) ),
                                        :( ::Val{$name_quote} ) ]),
                            def.args)

            # Provide access to SUPER(), which invokes the supertype's implementation.
            first_implementing_parent_idx = findfirst(
                t -> component_macro_overrides_configurable(t, Val(configurable_name)),
                enumerate_as_pair(Iterators.drop(all_supertypes_youngest_first, 1))
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
            configured_return_type = if supertype_t == AbstractComponent
                nothing
            else
                component_macro_configurable_data(supertype_t, Val(configurable_name)).return_type
            end
            if exists(configured_return_type)
                def.body = :( let result = $(def.body)
                    if !isa(result, $configured_return_type)
                        error($(string(component_name)), ".", $(string(configurable_name)),
                                " doesn't return a ", $(string(configured_return_type)),
                                "! It returned a ", typeof(result))
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

            def.body = (def.body)
            return esc(combinedef(def))
        end...)
    end)

    # Implement promises and configurables as properties.
    push!(global_decls, quote
        @inline $Base.propertynames(c::$(esc(component_name))) = tuple(
            :world, :entity,
            $((QuoteNode(n) for (n, T) in field_data)...),
            $((QuoteNode(p) for p in all_promises)...),
            $((QuoteNode(c) for c in all_configurables)...)
        )

        @inline $Base.getproperty(c::$(esc(component_name)), name::Symbol) = getproperty(c, Val(name))
        @inline $Base.getproperty(c::$(esc(component_name)), ::Val{:entity}) = getfield(c, :entity)
        @inline $Base.getproperty(c::$(esc(component_name)), ::Val{:world}) = getfield(c, :world)

        $(map(field_data) do (name, type)
            :( @inline $Base.getproperty(c::$(esc(component_name)), ::Val{$(QuoteNode(name))}) =
                   getfield(c, $(QuoteNode(name))) )
        end...)

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
    push!(global_decls, quote
        $(@__MODULE__).is_entitysingleton_component(::Type{$(esc(component_name))})::Bool = $is_entity_singleton
        $(@__MODULE__).is_worldsingleton_component(::Type{$(esc(component_name))})::Bool = $is_world_singleton
        $(@__MODULE__).require_components(::Type{$(esc(component_name))}) = tuple($(esc.(requirements)...))
        $(@__MODULE__).create_component(::Type{$(esc(component_name))}, entity::$Entity, args...; kw_args...) = begin
            if $is_abstract
                $(if isnothing(defaultor)
                      :( error("Can't create a '", $(string(component_name)),
                                 "'; it's abstract and no DEFAULT() was provided") )
                  else
                      :( return $(@__MODULE__).create_component(
                        $(esc(defaultor[1])), entity,
                        $(esc.(defaultor[2])...)
                        ; $(esc.(defaultor[3])...)
                      ) )
                  end)
            else
                this = $(esc(component_name))(entity, entity.world)
                $(@__MODULE__).component_macro_init($(esc(component_name)),
                                                    this, entity, entity.world,
                                                    args...; kw_args...)
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
                :( $(@__MODULE__).component_macro_tick($(esc(parent_t)), c) )
                  for parent_t in all_supertypes_youngest_first
            )...)
            $((
                :( $(@__MODULE__).component_macro_finish_tick($(esc(parent_t)), c) )
                  for parent_t in all_supertypes_oldest_first
            )...)
            return nothing
        end
    end)

    # Add some other useful stuff.
    !is_abstract && push!(global_decls, :(
        function $Base.print(io::IO, c::$(esc(component_name)))
            print(io, $(string(component_name)), "(<entity>, <world>" )
            $(map(field_data) do (name, declared_type); quote
                print(io, ", ")
                $(@__MODULE__).component_field_print(
                    c,
                    Val($(QuoteNode(name))), getfield(c, $(QuoteNode(name))),
                    io
                )
            end end...)
            print(io, ")")
        end
    ))

    final_expr = quote
        $type_decl
        $(global_decls...)
    end
    if PRINT_COMPONENT_CODE
        @info "\n$(MacroTools.prettify(final_expr))\n\nCOMPONENT $component_name\n"
    end
    return final_expr
end
