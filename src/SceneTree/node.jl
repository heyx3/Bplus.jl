"
A node in a scene tree. Offers the following interface:

"
struct Node{TNodeID, F<:AbstractFloat}
    #TODO: Generalize to any number of dimensions.
    parent::TNodeID
    sibling_prev::TNodeID
    sibling_next::TNodeID
    child_first::TNodeID

    n_children::Int

    local_pos::Vec3{F}
    local_rot::Quaternion{F}
    local_scale::Vec3{F}

    is_cache_valid::Bool
    cached_matrix_self::@Mat(4, 4, F)
    cached_matrix_world::@Mat(4, 4, F)
    cached_rot::Quaternion{F}
    cached_matrix_world_inverse::@Mat(4, 4, F)
end
export Node


######################
#  Public Interface  #
######################

"Iterates over the siblings of a node, from start to finish, optionally ignoring the given one."
@inline function siblings( node::Node{TNodeID, F},
                           context = nothing,
                           ignore_given::Bool = false
                         ) where {TNodeID, F}
    return Siblings{TNodeID, typeof(context), F}(node, context, ignore_given)
end
"Overload that takes a node ID instead of a `Node` instance."
siblings(node_id, context = nothing, args...) = siblings(deref_node(node_id, context), context, args...)

"Iterates over the children of a node."
@inline function children( node::Node,
                           context = nothing
                         )
    return Siblings{TNodeID, typeof(context), F}(node.child_first, context)
end
"Overload that takes a node ID instead of a `Node` instance."
children(node_id, context = nothing) = children(deref_node(node_id, context), context)

"Gets a node, or `nothing` if the handle is null."
@inline function try_deref_node(node_id, context = nothing)::Optional{Node}
    if is_null_id(node_id)
        return nothing
    else
        return deref_node(node_id, context)
    end
end


export try_deref_node, siblings, children


####################################
#  Siblings and children iterator  #
####################################

struct Siblings{TNodeID, TContext, F}
    root::Node{TNodeID, F}
    context::TContext
    ignore_root::Bool
end
struct Siblings_State{TNodeID, F}
    prev_idx::Int
    next_node::TNodeID
    idx_to_ignore::Int
end

Base.haslength(::Siblings) = false
Base.eltype(::Siblings{TNodeID}) where {TNodeID} = TNodeID

@inline Base.iterate(s::Siblings) = iterate(s, start_sibling_iter(s))
@inline Base.iterate(::Siblings, ::Nothing) = nothing
@inline function Base.iterate( s::Siblings{TNodeID, TContext, F},
                               state::Siblings_State{TNodeID, F}
                             ) where {TNodeID, TContext, F}
    if is_null_id(state.next_node)
        return nothing
    else
        next_node = deref_node(state.next_node, s.context)
        new_state = Siblings_State(
            state.prev_idx + 1,
            next_node.sibling_next,
            state.idx_to_ignore
        )
        return (next_node, new_state)
    end
end
function start_sibling_iter( s::TSiblings,
                             offset::Int = 1  # Which index from here
                                              #    is the actual root of the search?
                           )::Optional{Siblings_State} where {TNodeID, TSiblings<:Siblings{TNodeID}}
    # Is the starting node the first sibling?
    if is_null_id(s.root.sibling_prev)
        # Start with the next sibling.
        # If there's no "next" sibling, then there aren't any siblings at all.
        if is_null_id(s.root.sibling_next)
            return nothing
        else
            idx_to_ignore = (s.ignore_root ? offset : -1)
            return Siblings_State(0, s.root.sibling_next, idx_to_ignore)
        end
    # Otherwise, look backwards for the first sibling.
    else
        next_iter = Siblings(deref_node(s.root.sibling_prev, s.context),
                             s.context, s.ignore_root)
        return start_sibling_iter(next_iter, offset + 1)
    end
end


struct Children{TNodeID, TContext, F}
    parent::Node{TNodeID, F}
    context::TContext
end
@inline Base.length(c::Children) = c.parent.n_children
@inline Base.eltype(::Children{TNodeID}) where {TNodeID} = TNodeID
@inline Base.iterate(c::Children) = (is_null_id(c.parent.child_first) ?
                                         nothing :
                                         (c.parent.child_first, c.parent.child_first))
@inline function Base.iterate(c::Children, @specialize prev_id)
    prev_node = deref_node(prev_id, c.context)
    if is_null_id(prev_node.sibling_next)
        return nothing
    else
        return (prev_node.sibling_next, prev_node.sibling_next)
    end
end


####################
#  Implementation  #
####################

#invalidate_world_matrix