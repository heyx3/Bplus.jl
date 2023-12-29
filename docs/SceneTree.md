# SceneTree


## Defining your memory layout

To use this module, you should first decide how you store your nodes. You can see examples of this in the unit test file *test/scene-tree.jl*. In particular, you need:
* A unique ID for each node. Call this data type `ID`.
* [Optional] a "context" that can retrieve the node for an ID. Call this type `TContext`. If you don't need a context to retrieve nodes, use `Nothing`.
* A custom node type which somehow owns an instance of the immutable struct `BplusTools.SceneTree.Node{ID, F}`. You should pick a specific `F` (and, of course, an `ID`). Call this node type `TNode`.

## Implementing the memory layout interface

Once you have an architecture in mind, you can integrate it with `SceneTree` by implementing the following interface with your types:

* `null_node_id(::Type{ID})::ID`: a special ID value representing 'null'.
* `is_null_id(id::ID)::Bool`: by default, compares with `null_node_id(ID)` using `===` (triple-equals, a.k.a. egality).
* `deref_node(id::ID[, context::TContext])::TNode`: gets the node with the given ID. Assume the ID is not null, and the node exists.
* `update_node(id::ID[, context::TContext], new_value::SceneTree.Node{ID})`: should replace your node's associated `SceneTree.Node` with the given new value.
* `on_rooted(id::ID[, context::TContext])` and/or `on_uprooted([same args])` to be notified when a node loses its parent (a.k.a. becomes a "root") or gains a parent (a.k.a. becomes "uprooted"). This is useful if you have a special way of storing root nodes separate from child nodes.

## Examples

### SceneTree as a flat list of nodes

A really simple and dumb memory layout would be to put each node in a list, assume nodes are never deleted, and reference each node using its index in the list. Let's also say the component type used is Float32. In this case, you can define the following architecture:
* `const MyID = Int`
* `const MyContext = Vector{SceneTree.Node{MyID, Float32}}`
* `null_node_id(::Type{MyID}) = 0`
* `deref_node(i::MyID, context::MyContext) = context[i]`
* `update_node(i::Int, context::Vector, value::SceneTree.Node) = (context[i] = value)`.

Now your list of nodes can participate in a scene graph!

NOTE: the use of `Int` and `Vector{SceneTree.Node}` in your custom SceneTree implementation is type piracy, and you should wrap one or both of them in a custom decorator type to avoid this.

### SceneTree as a group of garbage-collected nodes

Another way to layout memory for a scene tree would be to define a custom mutable struct, `MyEntity`, which has the field `node::SceneTree.Node{ID, Float32}`. Then `ID` is simply a reference to `MyEntity`, because mutable structs are reference types, and no context is needed! However, to handle null ID's, wrap the ID type in Julia's `Ref` type, which boxes it and allows for null values. So the final implementation looks like:
* `const MyID = Ref{MyEntity}`
* `deref_node(id::MyID) = id[].node`
* `update_node(id::MyID, data::SceneTree.Node) = (id[].node = data)`
* `null_node_id(::Type{MyID}) = MyID()`
* `is_null_id(id::MyID) = isassigned(id)`

NOTE: in practice, this is not easy to implement in Julia due to a circular reference. `MyID`'s definition references `MyEntity`, but `MyEntity` also has to reference `MyID` in its `node` field. This can be solved with an extra layer of indirection using type parameters; check out the unit tests to see how it's done.

## Usage

*Refer to function definitions for more info on the optional params within each function.*

Creating nodes and adding them to the scene graph is something you do yourself, since you chose the memory representation.

Once you have some nodes, you can parent them to each other with `set_parent(child::ID, parent::ID[, context]; ...)`. To unparent a node, pass a 'null' ID for the parent (the specific representation of 'null' depends on how you defined node ID's).

You can get the transform matrix of a node with `local_transform(id[, context])` or `world_transform(id[, context])`. The "world transform" means the transform in terms of the root of the tree.

Functions to *set* transform, or get individual position/rotation/scale values, are still to be implemented.

You can iterate over different parts of a tree:
* `siblings(id[, context], include_self=true)` gets an iterator over a node's siblings in order.
* `children(id[, context])` gets an iterator over a node's direct children (no grand-children).
* `parents(id[, context])` gets an iterator over a node's parent, grand-parent, etc. The last element will be a root node.
* `family(id[, context], include_self=true)` gets an iterator over the entire tree underneath a node. The iteration is Depth-First, and this is the most efficient iteration if you don't care about order.
* `family_breadth_first(id[, context], include_self=true)` gets a breadth-first iterator over the entire tree underneath a node. This implementation avoids any heap allocations, at the cost of some processing overhead. It's recommended for small trees.
* `family_breadth_first_deep(id[, context], include_self=true)` gets a breadth-first iterator over the entire tree underneath a node. This implementation makes heap allocations, but iterates more efficiently than the other breadth-first implementation. It's recommended for large trees.

Some other helper functions:
* `try_deref_node(id[, context])` gets a node, or `nothing` if the ID is null.
* `is_deep_child_of(parent_id, child_id[, context])` gets whether a node appears somewhere underneath another node.
