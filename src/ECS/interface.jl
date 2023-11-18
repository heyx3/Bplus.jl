"
Gets whether an entity can hold more than one of the given type of component.

If your components inherit from another abstract component type,
    it's illegal for the abstract type to return a different value than the concrete child types.
"
is_entitysingleton_component(::Type{<:AbstractComponent})::Bool = false
"
Gets whether a world can have more than one of the given type of component.

If your components inherit from another abstract component type,
    it's illegal for the abstract type to return a different value than the concrete child types.
"
is_worldsingleton_component(::Type{<:AbstractComponent})::Bool = false

"
Gets the types of components required by the given component.

If your components inherit from another abstract component type,
    it's illegal for the abstract type to specify requirements
    unless the concrete child types also specify them.

If you name an abstract component type as a requirement,
    make sure to define `create_component()` for that abstract type
    (i.e. a default to use if the component doesn't already exist).
"
require_components(::Type{<:AbstractComponent})::Tuple = ()


"
Creates a new component that will be attached to the given entity.

Any dependent components named in `require_components()` will already be available,
    except in recursive cases where multiple components require each other.

By default, invokes the component's constructor with any provided arguments.
"
create_component(T::Type{<:AbstractComponent}, e::Entity, args...; kw_args...)::T = T(args...; kw_args...)

"Cleans up a component that was attached to the given entity"
destroy_component(::AbstractComponent, ::Entity, is_entity_dying::Bool) = nothing


"
Updates the given component attached to the given entity.

Note that the entity reference is only given for convenience;
    the component will always have the same Entity owner that it did when it was created.
"
tick_component(::AbstractComponent, ::Entity) = nothing

export is_entitysingleton_component, require_components,
       create_component, destroy_component,
       tick_component