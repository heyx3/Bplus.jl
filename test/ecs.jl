# Component1 is not a singleton, and requires a Component2.
@component Component1 {require: Component2} begin
    i::Int
    function CONSTRUCT(i = -1)
        this.i = i
    end
end

# Component2 is a singleton and has no requirements.
@component Component2 {entitySingleton} begin
    s::String
    function CONSTRUCT(s = "")
        this.s = s
    end
end

# Component3 is a singleton, and requires a Component4.
@component Component3 {entitySingleton} {require: Component2, Component4} begin end
ECS.is_entitysingleton_component(::Type{Component3}) = true
ECS.require_components(::Type{Component3}) = (Component2, Component4)

# Component4 is not a singleton, and requires a Component3.
@component Component4 {require: Component3} begin end

# Component5 and Component6 are subtypes of an abstract component type.
@component Component_5_or_6 {abstract} begin
    f::Float32
    is_dead::Bool
    CONSTRUCT(f) = begin
        this.f = convert(Float32, f)
        this.is_dead = false
    end
    function DESTRUCT()
        @bp_check(!this.is_dead, "It's already dead??")
        this.is_dead = true
    end
end
@component Component5 <: Component_5_or_6 begin
    i5::Int
    CONSTRUCT(f) = begin
        this.i5 = -5
        SUPER(f)
    end
end
@component Component6 <: Component_5_or_6 begin
    CONSTRUCT(f) = SUPER(f)
    DESTRUCT() = nothing
end

#TODO: Test worldSingleton.


# Define references to all the testable data.
world = World()
entities = Entity[ ]
c1s = Component1[ ]
c2 = Ref{Component2}()
c2_2 = Ref{Component2}()
c3 = Ref{Component3}()
c4s = Component4[ ]
c5s = Component5[ ]
c6s = Component6[ ]
EMPTY_ENTITY_COMPONENT_LOOKUP = Dict{Type{<:AbstractComponent}, Set{AbstractComponent}}()

# Add entities to the world.
push!(entities, add_entity(world))
@bp_check world.entities == entities
@bp_check world.component_lookup ==
        Dict(e=>EMPTY_ENTITY_COMPONENT_LOOKUP for e in entities)
push!(entities, add_entity(world))
@bp_check world.entities == entities
Dict(e=>EMPTY_ENTITY_COMPONENT_LOOKUP for e in entities)
push!(entities, add_entity(world))
@bp_check world.entities == entities
Dict(e=>EMPTY_ENTITY_COMPONENT_LOOKUP for e in entities)
remove_entity(world, entities[2])
deleteat!(entities, 2)
@bp_check world.entities == entities
Dict(e=>EMPTY_ENTITY_COMPONENT_LOOKUP for e in entities)
# Test that other stuff is unaffected by new entities.
@bp_check isempty(world.component_counts)
@bp_check world.time_scale == 1

# Check component type hierarchies.
@bp_check collect(get_component_types(Component2)) == [ Component2 ]
@bp_check collect(get_component_types(Component5)) == [ Component5, Component_5_or_6 ]
@bp_check collect(get_component_types(Component_5_or_6)) == [ Component_5_or_6 ]

# Add a C2 to E1.
c2[] = add_component(entities[1], Component2, "hi world")
@bp_check c2[].s == "hi world"
@bp_check entities[1].components == [ c2[] ]
@bp_check world.component_lookup == Dict(
            entities[1] => Dict(Component2=>Set([ c2[] ])),
            (e => EMPTY_ENTITY_COMPONENT_LOOKUP for e in entities[2:end])...
        )
@bp_check world.entity_lookup == Dict(
            Component2 => Set([ entities[1] ])
        )
@bp_check world.component_counts == Dict(Component2 => 1)

# Add multiple C1's to E1.
push!(c1s, add_component(entities[1], Component1))
push!(c1s, add_component(entities[1], Component1))
push!(c1s, add_component(entities[1], Component1))
@bp_check entities[1].components == [ c2[], c1s... ]
@bp_check world.component_lookup == Dict(
            entities[1] => Dict(
                Component2 => Set([ c2[] ]),
                Component1 => Set(c1s)
            ),
            (e => EMPTY_ENTITY_COMPONENT_LOOKUP for e in entities[2:end])...
        )
@bp_check world.entity_lookup == Dict(
            Component2 => Set([ entities[1] ]),
            Component1 => Set([ entities[1] ])
        )
@bp_check world.component_counts == Dict(
            Component1 => 3,
            Component2 => 1
        )

# Remove the second of the three C1's.
remove_component(entities[1], c1s[2])
deleteat!(c1s, 2)

@bp_check entities[1].components == [ c2[], c1s... ]
@bp_check entities[2].components == [ ]

@bp_check world.component_lookup == Dict(
            entities[1] => Dict(
                Component2 => Set([ c2[] ]),
                Component1 => Set(c1s)
            ),
            (e => EMPTY_ENTITY_COMPONENT_LOOKUP for e in entities[2:end])...
        )
@bp_check world.entity_lookup == Dict(
            Component2 => Set([ entities[1] ]),
            Component1 => Set([ entities[1] ])
        )
@bp_check world.component_counts == Dict(
            Component1 => 2,
            Component2 => 1
        )

# Do some component queries.
@bp_check has_component(entities[1], Component1)
@bp_check has_component(entities[1], Component2)
@bp_check has_component(world, Component1)
@bp_check has_component(world, Component2)
@bp_check !has_component(entities[1], Component3)
@bp_check !has_component(entities[1], Component4)
@bp_check !has_component(world, Component3)
@bp_check !has_component(world, Component4)
@bp_check !has_component(entities[2], Component1)
@bp_check !has_component(entities[2], Component2)
@bp_check !has_component(entities[2], Component3)
@bp_check !has_component(entities[2], Component4)
@bp_check get_component(entities[1], Component2) == c2[]
@bp_check Set(get_components(entities[1], Component1)) ==
                Set(c1s)
@bp_check get_component(world, Component2) == (c2[], entities[1])
@bp_check Set(get_components(world, Component1)) ==
                Set(zip(c1s, Iterators.repeated(entities[1])))

# Test the use of recursive component requirements.
push!(c4s, add_component(entities[2], Component4))
@bp_check count(x->true, get_components(world, Component4)) === 1
@bp_check count(x->true, get_components(entities[2], Component4)) === 1
# Adding Component4 should have added Component3 to the same entity.
@bp_check count(x->true, get_components(world, Component3)) === 1
@bp_check count(x->true, get_components(entities[2], Component3)) === 1
c3[] = get_component(world, Component3)[1]
# It should also have added Component2.
@bp_check has_component(entities[2], Component2)
c2_2[] = get_component(entities[2], Component2)
@bp_check entities[1].components == [ c2[], c1s... ]
@bp_check entities[2].components == [ c2_2[], c3[], c4s... ]
@bp_check Set(get_components(world, Component2)) ==
                Set([ (c2[], entities[1]), (c2_2[], entities[2]) ])
@bp_check world.component_lookup == Dict(
            entities[1] => Dict(
                Component2 => Set([ c2[] ]),
                Component1 => Set(c1s)
            ),
            entities[2] => Dict(
                Component3 => Set([ c3[] ]),
                Component4 => Set(c4s),
                Component2 => Set([ c2_2[] ])
            )
        )
@bp_check world.entity_lookup == Dict(
            Component2 => Set([ entities[1], entities[2] ]),
            Component1 => Set([ entities[1] ]),
            Component3 => Set([ entities[2] ]),
            Component4 => Set([ entities[2] ]),
        )
@bp_check world.component_counts == Dict(
            Component1 => 2,
            Component2 => 2,
            Component3 => 1,
            Component4 => 1
        )

# Add Component5 and check that it's also registered under its abstract parent type.
push!(c5s, add_component(entities[1], Component5, 5.5))
@bp_check c5s[end].f == @f32(5.5)
@bp_check c5s[end].i5 == -5
@bp_check c5s[end].is_dead == false
@bp_check count(x->true, get_components(entities[1], Component5)) == 1
@bp_check count(x->true, get_components(entities[1], Component_5_or_6)) == 1
@bp_check count(x->true, get_components(world, Component5)) == 1
@bp_check count(x->true, get_components(world, Component_5_or_6)) == 1
@bp_check entities[1].components == [ c2[], c1s..., c5s... ]
@bp_check entities[2].components == [ c2_2[], c3[], c4s... ]
@bp_check Set(get_components(world, Component5)) ==
        Set([ (c5s[1], entities[1]) ])
@bp_check Set(get_components(world, Component_5_or_6)) ==
        Set([ (c5s[1], entities[1]) ])
@bp_check world.component_lookup == Dict(
    entities[1] => Dict(
        Component2 => Set([ c2[] ]),
        Component1 => Set(c1s),
        Component5 => Set(c5s),
        Component_5_or_6 => Set(c5s)
    ),
    entities[2] => Dict(
        Component3 => Set([ c3[] ]),
        Component4 => Set(c4s),
        Component2 => Set([ c2_2[] ])
    )
)
@bp_check world.entity_lookup == Dict(
    Component2 => Set([ entities[1], entities[2] ]),
    Component1 => Set([ entities[1] ]),
    Component3 => Set([ entities[2] ]),
    Component4 => Set([ entities[2] ]),
    Component5 => Set([ entities[1] ]),
    Component_5_or_6 => Set([ entities[1] ])
)
@bp_check world.component_counts == Dict(
    Component1 => 2,
    Component2 => 2,
    Component3 => 1,
    Component4 => 1,
    Component5 => 1,
    Component_5_or_6 => 1,
)

# Add Component6 and check that it's also registered under its abstract parent type,
#    shared with the Component5 added in the previous test.
push!(c6s, add_component(entities[1], Component6, 70))
@bp_check(c6s[end].f == 70)
@bp_check(c6s[end].is_dead == false)
@bp_check count(x->true, get_components(entities[1], Component6)) == 1
@bp_check count(x->true, get_components(entities[1], Component_5_or_6)) == 2
@bp_check count(x->true, get_components(world, Component6)) == 1
@bp_check count(x->true, get_components(world, Component_5_or_6)) == 2
@bp_check entities[1].components == [ c2[], c1s..., c5s..., c6s... ]
@bp_check entities[2].components == [ c2_2[], c3[], c4s... ]
@bp_check Set(get_components(world, Component6)) ==
            Set([ (c6s[1], entities[1]) ])
@bp_check Set(get_components(world, Component_5_or_6)) ==
            Set([ (c5s[1], entities[1]), (c6s[1], entities[1]) ])
@bp_check world.component_lookup == Dict(
    entities[1] => Dict(
        Component2 => Set([ c2[] ]),
        Component1 => Set(c1s),
        Component5 => Set(c5s),
        Component6 => Set(c6s),
        Component_5_or_6 => Set([ c5s..., c6s... ])
    ),
    entities[2] => Dict(
        Component3 => Set([ c3[] ]),
        Component4 => Set(c4s),
        Component2 => Set([ c2_2[] ])
    )
)
@bp_check world.entity_lookup == Dict(
    Component2 => Set([ entities[1], entities[2] ]),
    Component1 => Set([ entities[1] ]),
    Component3 => Set([ entities[2] ]),
    Component4 => Set([ entities[2] ]),
    Component5 => Set([ entities[1] ]),
    Component6 => Set([ entities[1] ]),
    Component_5_or_6 => Set([ entities[1] ])
)
@bp_check world.component_counts == Dict(
    Component1 => 2,
    Component2 => 2,
    Component3 => 1,
    Component4 => 1,
    Component5 => 1,
    Component6 => 1,
    Component_5_or_6 => 2,
)

# Remove component 5 and check that it updates its "is_dead" field.
remove_component(entities[1], c5s[end])
@bp_check(c5s[end].is_dead)
@bp_check(!c6s[end].is_dead)
deleteat!(c5s, length(c5s))