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

    is_cached_self::Bool
    cached_matrix_self::@Mat(4, 4, F)

    is_cached_world_mat::Bool
    cached_matrix_world::@Mat(4, 4, F)
    cached_matrix_world_inverse::@Mat(4, 4, F)

    is_cached_world_rot::Bool
    cached_rot_world::Quaternion{F}
end
export Node


######################
#  Public Interface  #
######################

"Iterates over the siblings of a node, from start to finish, optionally ignoring the given one."
@inline siblings(node::Node, context, ignore_given::Bool = false) = Siblings(node, context, ignore_given)
"Overload that takes a node ID instead of a `Node` instance."
@inline siblings(node_id, context, args...) = siblings(deref_node(node_id, context), context, args...)

"Iterates over the children of a node."
@inline children(node::Node, context) = Children(node, context)
"Overload that takes a node ID instead of a `Node` instance."
@inline children(node_id, context) = children(deref_node(node_id, context), context)

"Iterates over the parent, grand-parent, etc. of a node."
@inline parents(node::Node, context) = Parents(node, context)
"Overload that takes a node ID instead of a `Node` instance."
@inline parents(node_id, context) = parents(deref_node(node_id, context), context)

"
Iterates over all nodes underneath this one, in Depth-First order.
Must take the root node by its ID, not the `Node` instance.

For Breadth-First order (which is slower), see `family_breadth_first`.
"
@inline family(node_id, context, include_self::Bool) = FamilyDFS(node_id, context, include_self)
"
Iterates over all nodes underneath this one, in Breadth-first order.
Must take the root node by its ID, not the `Node` instance.

This search is slower, but sometimes Breadth-First is useful.
By default, a technique is used that has no heap allocations, but scales poorly for large trees.
For a heap-allocating technique that works better on large trees, see `family_breadth_first_deep`.
"
@inline family_breadth_first(node_id, context, include_self::Bool) =
    FamilyBFS_NoHeap(FamilyDFS(node_id, context, include_self))
"
Iterates over all nodes underneath this one, in Breadth-first order.
Must take the root node by its ID, not the `Node` instance.

This search is slower than Depth-First, but sometimes Breadth-First is useful.

In this version, a heap-allocated queue is used to efficiently iterate over large trees.
For smaller trees, consider using `family_breadth_first`, which will not allocate.
"
@inline function family_breadth_first_deep( node_id::TNodeID, context,
                                            include_self::Bool,
                                            buffer::Vector{TNodeID} = preallocated_vector(TNodeID, 20)
                                          ) where {TNodeID}
    error("Not implemented!")
end
println("#TODO: Implement BFS via queue")


"Gets a node, or `nothing` if the handle is null."
@inline function try_deref_node(node_id, context = nothing)::Optional{Node}
    if is_null_id(node_id)
        return nothing
    else
        return deref_node(node_id, context)
    end
end


export siblings, children, parents,
       family, family_breadth_first, family_breadth_first_deep
       try_deref_node


######################
#  Simple Iterators  #
######################

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


struct Parents{TNodeID, TContext, F}
    start::Node{TNodeID, F}
    context::TContext
end
@inline Base.haslength(::Parents) = false
@inline Base.eltype(::Parents{TNodeID}) where {TNodeID} = TNodeID
@inline Base.iterate(p::Parents) = (is_null_id(p.start.parent) ?
                                        nothing :
                                        (p.start.parent, p.start.parent))
@inline function Base.iterate(p::Parents, @specialize prev_id)
    prev_node = deref_node(prev_id, p.context)
    if is_null_id(prev_node.parent)
        return nothing
    else
        return (prev_node.parent, prev_node.parent)
    end
end


##############################
#  "Whole family" iterators  #
##############################

println("#TODO: Depth-first iterator tracks its own depth and supports a min + max depth, to dramatically simplify and speed-up the BFS")

struct FamilyDFS{TNodeID, TContext}
    root_id::TNodeID
    context::TContext
    include_self::Bool
end
struct FamilyDFS_State{TNodeID}
    prev_id::TNodeID

    # Tracks the change in depth between the current element and the previous one.
    # Needed for FamilyBFS_NoHeap.
    previous_depth_delta::Int
end
@inline Base.haslength(::FamilyDFS) = false
@inline Base.eltype(::FamilyDFS{TNodeID}) where {TNodeID} = TNodeID
@inline function Base.iterate(f::FamilyDFS{TNodeID})::Optional{Tuple} where {TNodeID}
    if f.include_self
        return (f.root_id, FamilyDFS_State(f.root_id, 0))
    else
        node = deref_node(f.root_id, context)
        if is_null_id(node.child_first)
            return nothing
        else
            return (node.child_first, FamilyDFS_State(node.child_first, 0))
        end
    end
end
function Base.iterate( f::FamilyDFS{TNodeID},
                       state::FamilyDFS_State{TNodeID}
                     )::Optional{Tuple} where {TNodeID}
    prev_id = state.prev_id
    prev_node = deref_node(prev_id, f.context)

    depth_delta::Int = 0

    # If the current node has a child, keep going deeper.
    next_id::TNodeID = prev_node.child_first
    # If this node has no child, try a sibling instead.
    # Don't look at siblings/parents of the root node though,
    #    since that's outside the scope of the search.
    if is_null_id(next_id) && (prev_id != f.root_id)
        next_id = prev_node.sibling_next
        # If there's no next sibling, go up one level and try *that* parent's sibling.
        # Keep moving up until we can find an "uncle", or we hit the root node.
        while is_null_id(next_id) && (prev_node.parent != f.root_id)
            # Move up one level:
            prev_id = prev_node.parent
            prev_node = deref_node(prev_id, f.context)

            # Try to select the "uncle":
            next_id = prev_node.sibling_next
        end
    end

    return is_null_id(next_id) ?
               nothing :
               (next_id, FamilyDFS_State(next_id, depth_delta))
end


# The no-heap BFS implementation is based on Iterative-Deepening,
#    using successive applications of DFS.
# The downside is a lot of redundant visits to early nodes.
struct FamilyBFS_NoHeap{TNodeID, TContext}
    dfs::FamilyDFS{TNodeID, TContext}
end
struct FamilyBFS_NoHeap_State{TNodeID}
    dfs::FamilyDFS_State{TNodeID}
    current_target_depth::Int

    # As we iterate across a depth level, we need to remember
    #    whether the next depth level has any nodes.
    next_depth_any_nodes::Bool
end
@inline Base.haslength(::FamilyBFS_NoHeap) = false
@inline Base.eltype(::FamilyBFS_NoHeap{TNodeID}) where {TNodeID} = TNodeID
@inline function Base.iterate(f::FamilyBFS_NoHeap{TNodeID})::Optional{Tuple} where {TNodeID}
    first_iter::Optional{Tuple{TNodeID, FamilyDFS_State{TNodeID}}} =
        iterate(f.dfs)
    if isnothing(first_iter)
        return nothing
    else
        (first_element::TNodeID, dfs_state::FamilyDFS_State{TNodeID}) = first_iter
        initial_target_depth = (f.dfs.include_self ? 0 : 1)
        return (first_element, FamilyBFS_NoHeap_State(dfs_state, initial_target_depth, false))
    end
end
@inline function Base.iterate( f::FamilyBFS_NoHeap{TNodeID},
                               state::FamilyBFS_NoHeap_State{TNodeID}
                             )::Optional{Tuple} where {TNodeID}
    # Split out the fields of the iterator state,
    #    since it's immutable and we'll be using it a lot.
    (dfs_state, current_target_depth, next_depth_any_nodes) =
        (state.dfs, state.current_target_depth, state.next_depth_any_nodes)

    # Remember whether the node we just looked at has children.
    if !next_depth_any_nodes
        next_depth_any_nodes = (deref_node(dfs_state.prev_id, f.context).n_children > 0)
    end

    # Move the DFS forward until reaching another node at our current depth.
    dfs_depth::Int = current_target_depth
    local next_iter::Optional{Tuple{TNodeID, FamilyDFS_State{TNodeID}}}
    @do_while begin
        next_iter = iterate(f.dfs, dfs_state)
        # If the iterator finished, restart it with a deeper target depth.
        if isnothing(next_iter)
            # Are there any deeper nodes?
            # If so, restart the DFS search at the next depth level.
            if next_depth_any_nodes
                # We know we can skip the root node at this point, so make sure to do that
                #    to save a little time.
                inner_search = FamilyDFS(f.root_id, f.context, false)
                next_iter = iterate(inner_search)
                @bp_scene_tree_assert(exists(new_first_iter),
                                      "'next_depth_any_nodes' was true, yet there's nothing to iterate?????")

                # Update this iterator's state data.
                dfs_depth = 1
                next_depth_any_nodes = false
                current_target_depth += 1
            else
                return nothing
            end
        # The DFS didn't finish, so acknowledge it and continue.
        else
            dfs_depth += dfs_state.previous_depth_delta
            @bp_scene_tree_assert(dfs_depth >= 0,
                                  "An iteration of DFS within BFS just went above the root")
        end
    end (dfs_depth != current_target_depth)

    return isnothing(next_iter) ?
               nothing :
               (next_iter[1], FamilyBFS_NoHeap_State(dfs_state, current_target_depth, next_depth_any_nodes))
end


####################
#  Implementation  #
####################

"
Invalidates the cached world-space data.
May or may not include the rotation;
    for example if the node moved but didn't rotate,
    then there's no need to invalidate its world rotation.
Returns a copy of this node (does not update it in the context),
    but its children *will* be modified.
"
function invalidate_world_space( node::Node{TNodeID, F},
                                 context::TContext,
                                 include_rotation::Bool
                               )::Node{TNodeID, F} where {TNodeID, F, TContext}
    # Skip the work if caches are already invalidated.
    if !node.is_cached_world_mat && (!include_rotation || !node.cached_rot_world)
        # If this node doesn't have a cached world transform,
        #    assert that its children don't either.
        @bp_scene_tree_debug begin
            for child_id::TNodeID in children(node, context)
                child::Node{TNodeID, F} = deref_node(child_id, context)
                @bp_check(!child.is_cached_world_mat,
                          "Child node has a cached world matrix while the direct parent doesn't")
                @bp_check(!include_rotation || !child.is_cached_world_rot,
                          "Child node has a cached world rotation while the direct parent doesn't")
            end
        end
    else
        # Our world matrix is about to be invalidated; assert that our parent's already is.
        @bp_scene_tree_debug if !is_null_id(node.parent)
            parent_node::Node{TNodeID, F} = deref_node(node.parent, context)
            @bp_check(!parent_node.is_cached_world_mat,
                      "Parent node's cached world matrix is still valid",
                      " while this node's is being invalidated")
            @bp_check(!include_rotation || !parent_node.is_cached_world_rot,
                      "Parent node's cached world rotation is still valid",
                      " while this node's is being invalidated")
        end

        # Invalidate this node's caches.
        @set! node.is_cached_world_mat = false
        if include_rotation
            @set! node.is_cached_world_rot = false
        end

        # Invalidate all childrens' caches.
        for child_id::TNodeID in children(node, context)
            updated_child = invalidate_world_space(
                deref_node(child_id, context),
                context,
                include_rotation
            )
            update_node(child_id, context, updated_child)
        end
    end

    return node
end

"
Removes the given node from under its parent.
Returns a copy of this node (does not update it in the context),
    but its parent and siblings *will* be modified.
This is part of the implementation for some other functions, and it has some quirks:
    1. It takes the current parent node (if one exists) instead of retrieving it manually.
    2. It won't invalidate the node's cached world-space data like you'd expect.
"
function disconnect_parent( node::Node{TNodeID, F},
                            node_id::TNodeID,
                            parent_data::Optional{Node{TNodeID, F}},
                            context::TContext
                          )::Node{TNodeID, F} where {TNodeID, F, TContext}
    @bp_scene_tree_assert(deref_node(node_id, context) == node,
                          "Node and its ID don't match!",
                          "\nID ", node_id, " is:\n\t", deref_node(node_id, context),
                          "\nGiven:\n\t", node)
    # Update the parent.
    if exists(parent_data)
        @bp_scene_tree_assert(deref_node(node.parent, context) == parent_data,
                              "Passed in a node other than the parent!",
                              "\nGiven:\n\t", parent_data,
                              "\nActual parent:\n\t", deref_node(node.parent, context))
        if parent_data.child_first == node_id
            @bp_scene_tree_assert(is_null_id(node.sibling_prev),
                                  "I am my parent's child, but I have a previous sibling??")
            @set! parent_data.child_first = node.sibling_next
        end

        @set! parent_data.n_children -= 1

        update_node(node.parent, context, parent_data)
    end

    # Update the previous sibling in the tree.
    if !is_null_id(node.sibling_prev)
        sibling_data::Node{TNodeID, F} = deref_node(node.sibling_prev, context)
        @bp_scene_tree_assert(sibling_data.sibling_next == node_id,
                              "My 'previous' sibling has a different 'next' sibling; it isn't me")

        @set! sibling_data.sibling_next = node.sibling_next
        update_node(node.sibling_prev, context, sibling_data)
    end

    # Update the next sibling in the tree.
    if !is_null_id(node.sibling_next)
        sibling_data::Node{TNodeID, F} = deref_node(node.sibling_next, context)
        @bp_scene_tree_assert(sibling_data.sibling_prev == node_id,
                              "My 'next' sibling has a different 'previous' sibling; it isn't me")

        @set! sibling_data.sibling_prev = node.sibling_prev
        update_node(node.sibling_next, context, sibling_data)
    end
end