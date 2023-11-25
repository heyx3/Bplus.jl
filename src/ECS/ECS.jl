"A simple ECS modeled after Unity3D' "
module ECS

using MacroTools
using ..Utilities, ..Math

@make_toggleable_asserts bp_ecs_

include("types.jl")
include("interface.jl")
include("operations.jl")
include("execution.jl")
include("macros.jl")

export World, Entity, AbstractComponent,
       add_entity, remove_entity,
       add_component, remove_component,
       has_component, get_component, get_components,
       tick_world, reset_world,
       @component, get_component_types

end # module