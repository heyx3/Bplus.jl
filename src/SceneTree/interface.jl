# To implement this interface, you must first decide on a type for a node's 'ID'.
# It should be some sort of unique and nullable handle,
#    implementing the below interface.
# You should also define some kind of 'Context', the environment the nodes exist in.
# For example, the 'Context' could be a list of entities (each containing a Node),
#    and the node ID could be an index into this list.

export deref_node, null_node_id, is_null_id,
       update_node


"
Given a node's unique identifier, retrieves the node itself.
Optionally takes a user-defined 'context' to help retrieve the node data.
You do not have to handle the case where the ID is 'null'.
"
deref_node(node_id, context) = deref_node(node_id)
deref_node(node_id) = error("deref_node() not defined for ", typeof(node_id))

"
Creates a 'null' node ID.
By default, it will return 0 for unsigned integers, -1 for signed integers,
    or `nothing` for `Union{Nothing, T}`.
"
null_node_id(T::Type) = error("null_node_id() not defined for ", T)
null_node_id(T::Type{<:Signed}) = convert(T, -1)
null_node_id(T::Type{<:Unsigned}) = convert(T, 0)
null_node_id(::Type{Union{Nothing, <:Any}}) = nothing


"
Checks whether a node ID is 'null'.
By default, compares it to `null_node_id()` with triple-equals (`===`).
"
@inline is_null_id(node_id) = (node_id === null_node_id(typeof(node_id)))


"
Updates a node's value in its original storage space.
The `Node` type is immutable, so the original has to be replaced with a new copy.

Optionally takes the user-defined 'context'.
"
update_node(node_id, context, new_node) = update_node(node_id, new_node)
update_node(node_id, new_node) = error(
    "update_node() not defined for node ID '", typeof(node_id), "',",
    " and node type ", typeof(new_node)
)

"Callback for when a node becomes a 'root' node, meaning it's parent-less."
on_rooted(node_id, context) = on_rooted(node_id)
on_rooted(node_id) = nothing
"Callback for when a node gains a parent, and is no longer a 'root'."
on_uprooted(node_id, context) = on_uprooted(node_id)
on_uprooted(node_id) = nothing