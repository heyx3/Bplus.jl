# BplusTools — ECS, SceneTree, Fields

---

# ECS

A simple, friendly entity-component system loosely modelled on Unity3D's `GameObject`/`MonoBehaviour`. Optimized for ease of iteration, not throughput — if you need high performance, use an external Julia ECS package such as *Ark.jl* or *Overseer.jl*.

- A `World` is a list of `Entity` plus lookup structures.
- An `Entity` is a list of `AbstractComponent`.
- A component carries data and behavior, mostly a `TICK()` the world invokes.

`tick_world(world, delta_seconds::Float32)` updates everything. `reset_world(world)` destroys all entities and resets other fields — important when components own resources and you need every destructor to run.

Timing lives on the world: `delta_seconds`, `elapsed_seconds`, and a settable `time_scale`.

Enable extra error-checking while loading your code (never in release): `BplusTools.ECS.bp_ecs_asserts_enabled() = true`.

## Managing entities and components

```julia
add_entity(world)::Entity
remove_entity(world, entity)
add_component(entity, T, constructor_args...; kw_args...)::T   # args go to CONSTRUCT()
remove_component(entity, c::AbstractComponent)
```

Queries work on an `Entity` or the whole `World`:

| Function | Returns |
| --- | --- |
| `has_component(entity_or_world, T)` | `Bool` |
| `get_component(entity, T)` | `Optional{T}`; errors if the entity has more than one |
| `get_component(world, T)` | `Optional{Tuple{T, Entity}}`; errors if the world has more than one |
| `get_components(entity, T)` | iterator over `T` |
| `get_components(world, T)` | iterator over `Tuple{T, Entity}` |
| `count_components(entity_or_world, T)` | `Int` |
| `get_component_types(T)` | `Tuple{Vararg{Type}}` — T, its parent, grandparent, … stopping before `AbstractComponent` |

## `@component`

Components have an OOP structure: abstract parents contribute fields, behavior, "promises" (abstract functions) and "configurables" (virtual functions) to their children.

```julia
@component Name [<: Parent] [attributes...] begin
    field1::Float32
    field2

    # Default constructor takes every field in order, new fields before inherited ones.
    # Override it like this:
    function CONSTRUCT(f)
        # Every function in a @component can use `this`, `entity`, and `world`.
        this.field1 = Float32(f)
        this.field2 = length(world.entities)
        SUPER(3, f+4)      # required: invokes the parent constructor
    end
    function DESTRUCT()    # may take a Bool: is the owning entity being destroyed?
        println("Delta entities: ", length(world.entities) - this.field2)
    end

    function TICK()
        this.field1 += world.delta_seconds
    end
    # FINISH_TICK() also exists, but is only useful on abstract components.
end
```

Attributes, in braces:

| Attribute | Meaning |
| --- | --- |
| `{abstract}` | Abstract type — can have children, can't be instantiated |
| `{entitySingleton}` | At most one per entity |
| `{worldSingleton}` | At most one in the world |
| `{require: a, b, c}` | These components must exist on the entity; they're added if missing |

### Abstract components

```julia
"Some kind of animated motion using float position data. Only one maneuver can run at a time."
@component Maneuver{F<:AbstractFloat} {abstract} {entitySingleton} {require: Position} begin
    pos_component::Position{F}
    duration::F
    progress_normalized::F

    # Because this type has a custom constructor, every child must too,
    # and each must invoke SUPER(...) exactly once with these arguments.
    function CONSTRUCT(duration_seconds::F)
        this.progress_normalized = 0
        this.duration = duration_seconds
        this.pos_component = get_component(entity, Position)
    end

    # Optional: the instance to create when this component is required but absent.
    DEFAULT() = TurningManeuver(3.5, has_component(entity, PreferLeft) ? -90 : 90)

    # Concrete children MUST implement this. Abstract children MAY.
    @promise finish_maneuver(last_time_step::F)

    # Children MAY override this. Must return a Bool.
    @configurable should_stop()::Bool = false

    # Parent TICK() runs BEFORE children's TICK().
    function TICK()
        this.progress_normalized += world.delta_seconds / this.duration
    end
    # Parent FINISH_TICK() runs AFTER children's FINISH_TICK().
    function FINISH_TICK()
        if this.should_stop()
            remove_component(entity, this)
        elseif this.progress_normalized >= 1
            overshoot_seconds = (@f32(1) - this.progress_normalized) * this.duration
            this.finish_maneuver(-overshoot_seconds)
            remove_component(entity, this)
        end
    end
end
```

Inside a promise or configurable override, `SUPER()` calls the parent implementation; called with no arguments it forwards the child's arguments automatically. When a child implements one with different parameter types, parent and child both participate in overload resolution — so children can *extend or specialize* rather than only replace.

### A concrete child

```julia
@component StrafingManeuver <: Maneuver{Float32} {require: MovementSpeed} begin
    speed_component::MovementSpeed
    dir::v3f

    function CONSTRUCT(duration::Float32, dir::v3f)
        SUPER(duration)
        this.speed_component = get_component(entity, MovementSpeed)
        this.dir = dir
    end

    finish_maneuver(last_time_step::Float32) = strafe(this, last_time_step)
    should_stop() = do_collision_checking(entity)
    TICK() = strafe(this)
end

# In a plain Julia function, the implicit names aren't available:
function strafe(s::StrafingManeuver, time_step)
    entity = s.entity
    world = s.world
    if isnothing(time_step)
        time_step = world.delta_seconds
    end
    s.pos_component.value += s.speed_component.value * time_step * s.dir
end
```

Read the `@component` doc-string for the authoritative version — it's long and worked-through.

### Bypassing the macro

You can define a component by hand: a mutable struct inheriting `AbstractComponent` (or a child), implementing the interface in *ECS/interface.jl*. But it can't be the *parent* of a `@component` unless you also implement the interface functions at the top of *ECS/macros.jl* — at which point you may as well use the macro.

### Printing

Fields referencing an `Entity`, `World`, or `AbstractComponent` are abbreviated by default. Customize one field with `component_field_print(component, Val{:my_field}, field_value, io::IO)`.

Component reflection data (declared `@promise`s and so on) is reachable through the interface at the top of *src/ecs/macros.jl* — an internal interface that may change.

---

# SceneTree

Scene-graph algorithms that are agnostic about how you store nodes. **The architecture is done; some of the API is not** (setting transforms, and getting individual position/rotation/scale, are unimplemented).

## Choose a memory layout first

You need:

- `ID` — a unique node identifier type
- `TContext` — optional; something that can look up a node by ID. Use `Nothing` if you don't need one.
- `TNode` — your node type, which somehow owns an instance of the immutable `BplusTools.SceneTree.Node{ID, F}` for a chosen `F`

## Implement the interface

- `null_node_id(::Type{ID})::ID` — the "null" ID value
- `is_null_id(id::ID)::Bool` — defaults to `===` against `null_node_id(ID)`
- `deref_node(id::ID[, context::TContext])::TNode` — assume the ID is non-null and the node exists
- `update_node(id::ID[, context], new_value::SceneTree.Node{ID})` — replace the node's `SceneTree.Node`
- `on_rooted(id[, context])` / `on_uprooted(id[, context])` — optional notifications when a node loses or gains a parent, useful if roots are stored separately

## Example: flat list

Nodes in a `Vector`, never deleted, referenced by index, `Float32` components:

```julia
const MyID = Int
const MyContext = Vector{SceneTree.Node{MyID, Float32}}
null_node_id(::Type{MyID}) = 0
deref_node(i::MyID, context::MyContext) = context[i]
update_node(i::Int, context::Vector, value::SceneTree.Node) = (context[i] = value)
```

Using bare `Int` and `Vector{SceneTree.Node}` here is type piracy — wrap one or both in a decorator type in real code.

## Example: GC'd nodes

A mutable struct is a reference type, so it can be its own ID and no context is needed. Wrap the ID in `Ref` to get a null representation:

```julia
const MyID = Ref{MyEntity}
deref_node(id::MyID) = id[].node
update_node(id::MyID, data::SceneTree.Node) = (id[].node = data)
null_node_id(::Type{MyID}) = MyID()
is_null_id(id::MyID) = isassigned(id)
```

In practice this circular reference (`MyID` names `MyEntity`, whose `node` field names `MyID`) needs an extra layer of type-parameter indirection — see *test/scene-tree.jl*.

## Usage

You create nodes yourself, since you chose the representation.

```julia
set_parent(child::ID, parent::ID[, context]; ...)   # pass a null parent to unparent
local_transform(id[, context])
world_transform(id[, context])                       # relative to the tree's root
```

Iteration:

| Function | Order |
| --- | --- |
| `siblings(id[, ctx], include_self=true)` | in order |
| `children(id[, ctx])` | direct children only |
| `parents(id[, ctx])` | parent, grandparent, … ending at a root |
| `family(id[, ctx], include_self=true)` | depth-first over the whole subtree — the cheapest if order doesn't matter |
| `family_breadth_first(id[, ctx], include_self=true)` | breadth-first, zero heap allocations, more per-step overhead. Best for small trees. |
| `family_breadth_first_deep(id[, ctx], include_self=true)` | breadth-first, allocates, iterates faster. Best for large trees. |

Helpers: `try_deref_node(id[, ctx])` (`nothing` for a null ID), `is_deep_child_of(parent_id, child_id[, ctx])`.

---

# Fields

A data representation of continuous math: functions from `Vec{NIn, F}` to `Vec{NOut, F}`, with analytical derivatives available. Built for procedural generation and shader work — filling a texture with interesting noise, for instance. 3D Perlin noise is a `Field{3, 1, Float32}`; an RGBA texture generated from UVs is an `AbstractField{2, 4, Float32}`.

The upstream docs link a `Fields.md` that was never written. The authoritative sources are the `@field` / `@multi_field` doc-strings and *BplusTools/test/fields.jl*.

**IMPORTANT NOTE**: This module is unfinished, no longer being worked on, and should not be used. Other Julia packages exist which can transform math code, take derivatives, etc (e.g. Flux). For that reason this module is not documented further.