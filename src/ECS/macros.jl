# Some internal interface functions are used to connect macro-defined components to each other.
# An internal interface is used to hold onto inherited behavior from parent components.
component_macro_fields(  T::Type{<:AbstractComponent}) = [f=>fieldtype(f, T) for f in fieldnames(T)]
component_macro_init(     ::Type{<:AbstractComponent}, ::AbstractComponent, ::Entity, ::World        ) = nothing
component_macro_cleanup(  ::Type{<:AbstractComponent}, ::AbstractComponent, ::Entity, ::World, ::Bool) = nothing
component_macro_tick(     ::Type{<:AbstractComponent}, ::AbstractComponent, ::Entity, ::World        ) = nothing
component_macro_post_tick(::Type{<:AbstractComponent}, ::AbstractComponent, ::Entity, ::World        ) = nothing

macro component(title, elements...)
    # Parse the component's name and parent type.
    title_is_valid = @capture(title, name_Symbol)
    if title_is_valid
        supertype_t = nothing
    else
        title_is_valid = @capture(title, name_<:supertype_)
    end
    if !title_is_valid
        error("Invalid component name. Must be a name or 'name<:supertype_t'. Got '", title, "'")
    end

    # We need to know about the supertype_t at compile-time.
    if exists(supertype_t)
        supertype_t = __module__.eval(supertype_t)
    end

    # Extract the other parts of the declaration.
    attributes = elements[1:(end-1)]
    if length(elements) < 1
        error("No body found for the component '", name, "'")
    end
    statements = (elements[end]::Expr).args

    return macro_impl_component(name, supertype_t, attributes, statements)
end

function macro_impl_component(name::Symbol, supertype_t::Optional{Type},
                              attributes, statements)
    if !(supertype_t <: AbstractComponent)
        error("Component '", name, "'s parent type '", supertype_t, "' is not a component itself!")
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
            requirements = [a, b...]
        else
            error("Unexpected attribute: ", attribute)
        end
    end

    # Inherit things from the supertype_t.
    if exits(supertype_t)
        if is_entitysingleton_component(supertype_t)
            if is_entity_singleton
                @warn "Component $name already inherits {entitySingleton}, so its own attribute is redundant"
            else
                is_entity_singleton = true
            end
        end

        if is_worldsingleton_component(supertype_t)
            if is_world_singleton
                @warn "Component $name already inheritis {worldSingleton}, so its own attribute is redundant"
            else
                is_world_singleton = true
            end
            if is_entity_singleton
                @warn "Component $name has both {entitySingleton} and {worldSingleton} (at least one through inheritance), which is redundant"
            end
        end

        # Append the parent requirements after the child's
        #    (the child's requirements are probably more specific to satisfy the parent).
        append!(requirements, :( $@__MODULE__.require_components($supertype_t)... ))
    end
    if isnothing(supertype_t)
        supertype_t = :( $@__MODULE__.AbstractComponent )
    end

    # Get the full chain of component inheritance.
    all_supertypes = [ name ]
    ss = supertype_t
    while ss != AbstractComponent
        push!(all_supertypes, ss)
        ss = supertype(ss)
    end

    # Parse the declarations.
    field_decls = is_abstract ? nothing : [ ]
    global_decls = [ ]
    #TODO: Take fields from the parent
    #TODO: Take fields from the declarations

    # Emit the final code.
    type_decl = if is_abstract
        :( abstract type $(esc(name)) <: $supertype_t end )
    else
        :(
            mutable struct $(esc(name)) <: $supertype_t
                $(esc.(field_decls)...)
                $name() = new()
            end
        )
    end
    return quote
        $type_decl
        $(global_decls...)

        # Implement the public interface, with the help of our private macro interface.
        $@__MODULE__.is_entitysingleton_component(::Type{$name})::Bool = $is_entity_singleton
        $@__MODULE__.is_worldsingleton_component(::Type{$name})::Bool = $is_world_singleton
        #TODO: Finish
    end
end