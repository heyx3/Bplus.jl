"Ticks all components of the given type"
function tick_components(world::World, ::Type{T}) where {T<:AbstractComponent}
    for (component, entity) in @inline(get_components(world, T))
        tick_component(component, entity)
    end
    return nothing
end

function tick_world(world::World, delta_seconds::Float32)
    # Handle timing.
    if world.time_scale <= 0
        return nothing
    end
    world.delta_seconds = delta_seconds * world.time_scale
    world.elapsed_seconds += world.delta_seconds

    # Tick components in groups by component type,
    #    so that dynamic dispatch is only needed once per type.
    for component_type in keys(world.component_counts)
        if !isabstracttype(component_type)
            tick_components(world, component_type)
        end
    end

    return nothing
end