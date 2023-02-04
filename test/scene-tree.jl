# Create a simple test scene-tree backed by a set of reference-types.

######################
#  Data Definitions  #
######################

# The node is wrapped in a mutable struct called an "entity".
# A node's ID is a nullable reference to that entity.
# We can't use a plain union, because that union type won't persist through function calls.

# The ID type isn't defined yet, so it has to be a type parameter.
mutable struct ST_Entity_{IDType}
    transform::SceneTree.Node{IDType, Float32}
    is_root::Bool
end

struct ST_NodeID
    ref::Optional{ST_Entity_{ST_NodeID}}
    ST_NodeID() = new(nothing)
    ST_NodeID(ref::ST_Entity_{ST_NodeID}) = new(ref)
end
const ST_Entity = ST_Entity_{ST_NodeID}

Base.print(io::IO, e::ST_Entity) = print(io, v3i(e.transform.local_pos), " => ", e.transform.parent)
Base.show(io::IO, e::ST_Entity) = print(io, e)


const ST_Node = SceneTree.Node{ST_NodeID, Float32}


Base.show(io::IO, id::ST_NodeID) = isnothing(id.ref) ?
                                       print(io, "{null}") :
                                       show(io, id.ref)
Base.show(io::IO, en::ST_Entity) = print(io,
    '{', en.transform.local_pos.x,
    ", ", en.transform.local_pos.y,
    ", ", en.transform.local_pos.z,
    '}'
)

Base.print(io::IO, id::ST_NodeID) = show(io, id)


##############################
#  Interface Implementation  #
##############################

@inline SceneTree.null_node_id(::Type{ST_NodeID}) = ST_NodeID()
@inline SceneTree.is_null_id(id::ST_NodeID) = isnothing(id.ref)

@inline SceneTree.deref_node(id::ST_NodeID) = (id.ref::ST_Entity).transform
@inline SceneTree.update_node(id::ST_NodeID, data::ST_Node) = ((id.ref::ST_Entity).transform = data)

SceneTree.on_rooted(id::ST_NodeID) = (id.ref::ST_Entity).is_root = true
SceneTree.on_uprooted(id::ST_NodeID) = (id.ref::ST_Entity).is_root = false


######################
#  Scene Definition  #
######################

# Create the nodes. They will be assigned parents later.
const SCENE_TREE = map(node -> ST_Entity(node, true), [
    # The root nodes:
    ST_Node(v3f(1, 0, 0)),
    ST_Node(v3f(2, 0, 0)),
    ST_Node(v3f(3, 0, 0)),
    ST_Node(v3f(4, 0, 0)),

    # The direct children:
    ST_Node(v3f(1, 1, 0)), # 5
    ST_Node(v3f(1, 2, 0)),
    ST_Node(v3f(1, 3, 0)),
    ST_Node(v3f(2, 1, 0)),
    ST_Node(v3f(3, 1, 0)),
    ST_Node(v3f(3, 2, 0)), # 10

    # Some grand-children:
    ST_Node(v3f(1, 1, 1)), # 11
    ST_Node(v3f(1, 3, 1)),
    ST_Node(v3f(1, 3, 2)),
    ST_Node(v3f(3, 1, 1)),
    ST_Node(v3f(3, 1, 2)), # 15
    ST_Node(v3f(3, 1, 3)),
    ST_Node(v3f(3, 1, 4)),
    ST_Node(v3f(3, 1, 5)),
    ST_Node(v3f(3, 1, 6))  # 19
])
node_index(n::ST_NodeID) = let n_val = n.ref
    findfirst(e -> (e == n_val), SCENE_TREE)
end


# Set up the hierarchical relationships.
# Keep in mind that new children are inserted at the front of the parent's list.
function test_set_parent(child::Int, parent::Int, child_next_sibling::Int = -1)
    #TODO: Turn into a "no allocations" check.
    # @bp_check(
    #     begin
    #         SceneTree.set_parent(ST_NodeID(SCENE_TREE[child]),
    #                              ST_NodeID(SCENE_TREE[parent]))
    #         (node_index(SCENE_TREE[child].transform.parent),
    #          node_index(SCENE_TREE[parent].transform.child_first))
    #     end ==
    #     map(n -> node_index(ST_NodeID(n)),
    #         (SCENE_TREE[parent], SCENE_TREE[child])),
    #     "Parent/child link is incorrect after calling set_parent()"
    # )
    SceneTree.set_parent(ST_NodeID(SCENE_TREE[child]), ST_NodeID(SCENE_TREE[parent]))
    @bp_check(SCENE_TREE[child].transform.parent ==
                ST_NodeID(SCENE_TREE[parent]),
              "Child isn't referencing the parent correctly")
    @bp_check(SCENE_TREE[parent].transform.child_first ==
                ST_NodeID(SCENE_TREE[child]),
              "Parent isn't referencing the child correctly")

    @bp_check(!SCENE_TREE[child].is_root, "Node ", child, " shouldn't be a root")

    @bp_check(SCENE_TREE[child].transform.sibling_next ==
                  ((child_next_sibling == -1) ? ST_NodeID() :
                                                ST_NodeID(SCENE_TREE[child_next_sibling])),
              "Expected next sibling of new child $child (parent $parent) to be ",
                 " $child_next_sibling but it was $(SCENE_TREE[child].transform.sibling_next))")
end
test_set_parent(7, 1)
test_set_parent(6, 1, 7)
test_set_parent(5, 1, 6)
test_set_parent(8, 2)
test_set_parent(10, 3)
test_set_parent(9, 3, 10)
test_set_parent(11, 5)
test_set_parent(13, 7)
test_set_parent(12, 7, 13)
test_set_parent(19, 9)
test_set_parent(18, 9, 19)
test_set_parent(17, 9, 18)
test_set_parent(16, 9, 17)
test_set_parent(15, 9, 16)
test_set_parent(14, 9, 15)


###############
#  Iteration  #
###############

@inline function test_node_iter(iter, expected_idcs::Vector{Int})
    expected_entities = map(i -> ST_NodeID(SCENE_TREE[i]), expected_idcs)
    #TODO: Detecting allocations during iteration, but I can't figure out if they're real.
    #=@bp_test_no_allocations_setup(
        TEST_nodes = preallocated_vector(ST_NodeID, length(SCENE_TREE)),
        begin
            empty!(TEST_nodes)
            append!(TEST_nodes, iter)
            TEST_nodes
        end,
        expected_entities,
        "\n\nIterator: ", iter,
        "\n\tExpected: ", expected_entities,
        "\n\tActual: ", expected_entities,
        "\n\n"
    )=#
    @bp_check(collect(iter) == expected_entities,
              "Expected ", expected_entities,
              "\n\tActual ", collect(iter))
end


##  Siblings  ##

# Root nodes (shouldn't have any):
test_node_iter(siblings(ST_NodeID(SCENE_TREE[1]), true), [ 1 ])
test_node_iter(siblings(ST_NodeID(SCENE_TREE[1]), false), Int[ ])
#
test_node_iter(siblings(ST_NodeID(SCENE_TREE[2]), true), [ 2 ])
test_node_iter(siblings(ST_NodeID(SCENE_TREE[2]), false), Int[ ])
#
test_node_iter(siblings(ST_NodeID(SCENE_TREE[3]), true), [ 3 ])
test_node_iter(siblings(ST_NodeID(SCENE_TREE[3]), false), Int[ ])
#
test_node_iter(siblings(ST_NodeID(SCENE_TREE[4]), true), [ 4 ])
test_node_iter(siblings(ST_NodeID(SCENE_TREE[4]), false), Int[ ])

# Direct children:
test_node_iter(siblings(ST_NodeID(SCENE_TREE[5]), true), [ 5, 6, 7 ])
test_node_iter(siblings(ST_NodeID(SCENE_TREE[5]), false), [ 6, 7 ])
test_node_iter(siblings(ST_NodeID(SCENE_TREE[6]), true), [ 5, 6, 7 ])
test_node_iter(siblings(ST_NodeID(SCENE_TREE[6]), false), [ 5, 7 ])
test_node_iter(siblings(ST_NodeID(SCENE_TREE[7]), true), [ 5, 6, 7 ])
test_node_iter(siblings(ST_NodeID(SCENE_TREE[7]), false), [ 5, 6 ])
#
test_node_iter(siblings(ST_NodeID(SCENE_TREE[8]), true), [ 8 ])
test_node_iter(siblings(ST_NodeID(SCENE_TREE[8]), false), Int[ ])
#
test_node_iter(siblings(ST_NodeID(SCENE_TREE[9]), true), [ 9, 10 ])
test_node_iter(siblings(ST_NodeID(SCENE_TREE[9]), false), [ 10 ])
test_node_iter(siblings(ST_NodeID(SCENE_TREE[10]), true), [ 9, 10 ])
test_node_iter(siblings(ST_NodeID(SCENE_TREE[10]), false), [ 9 ])

# Grand-children:
test_node_iter(siblings(ST_NodeID(SCENE_TREE[11]), true), [ 11 ])
test_node_iter(siblings(ST_NodeID(SCENE_TREE[11]), false), Int[ ])
#
test_node_iter(siblings(ST_NodeID(SCENE_TREE[12]), true), [ 12, 13 ])
test_node_iter(siblings(ST_NodeID(SCENE_TREE[12]), false), [ 13 ])
test_node_iter(siblings(ST_NodeID(SCENE_TREE[13]), true), [ 12, 13 ])
test_node_iter(siblings(ST_NodeID(SCENE_TREE[13]), false), [ 12 ])
#
test_node_iter(siblings(ST_NodeID(SCENE_TREE[14]), true ), [ 14, 15, 16, 17, 18, 19 ])
test_node_iter(siblings(ST_NodeID(SCENE_TREE[14]), false), [ 15, 16, 17, 18, 19 ])
test_node_iter(siblings(ST_NodeID(SCENE_TREE[15]), true ), [ 14, 15, 16, 17, 18, 19 ])
test_node_iter(siblings(ST_NodeID(SCENE_TREE[15]), false), [ 14, 16, 17, 18, 19 ])
test_node_iter(siblings(ST_NodeID(SCENE_TREE[16]), true ), [ 14, 15, 16, 17, 18, 19 ])
test_node_iter(siblings(ST_NodeID(SCENE_TREE[16]), false), [ 14, 15, 17, 18, 19 ])
test_node_iter(siblings(ST_NodeID(SCENE_TREE[17]), true ), [ 14, 15, 16, 17, 18, 19 ])
test_node_iter(siblings(ST_NodeID(SCENE_TREE[17]), false), [ 14, 15, 16, 18, 19 ])
test_node_iter(siblings(ST_NodeID(SCENE_TREE[18]), true ), [ 14, 15, 16, 17, 18, 19 ])
test_node_iter(siblings(ST_NodeID(SCENE_TREE[18]), false), [ 14, 15, 16, 17, 19 ])
test_node_iter(siblings(ST_NodeID(SCENE_TREE[19]), true ), [ 14, 15, 16, 17, 18, 19 ])
test_node_iter(siblings(ST_NodeID(SCENE_TREE[19]), false), [ 14, 15, 16, 17, 18 ])
#

##  Children  ##

# Root nodes:
test_node_iter(children(ST_NodeID(SCENE_TREE[1])), [ 5, 6, 7 ])
test_node_iter(children(ST_NodeID(SCENE_TREE[2])), [ 8 ])
test_node_iter(children(ST_NodeID(SCENE_TREE[3])), [ 9, 10 ])
test_node_iter(children(ST_NodeID(SCENE_TREE[4])), Int[ ])
# Direct children:
test_node_iter(children(ST_NodeID(SCENE_TREE[5])), [ 11 ])
test_node_iter(children(ST_NodeID(SCENE_TREE[6])), Int[ ])
test_node_iter(children(ST_NodeID(SCENE_TREE[7])), Int[ 12, 13 ])
test_node_iter(children(ST_NodeID(SCENE_TREE[8])), Int[ ])
test_node_iter(children(ST_NodeID(SCENE_TREE[9])), [ 14, 15, 16, 17, 18, 19 ])
test_node_iter(children(ST_NodeID(SCENE_TREE[10])), Int[ ])
# Grand-children:
test_node_iter(children(ST_NodeID(SCENE_TREE[11])), Int[ ])
test_node_iter(children(ST_NodeID(SCENE_TREE[12])), Int[ ])
test_node_iter(children(ST_NodeID(SCENE_TREE[13])), Int[ ])
test_node_iter(children(ST_NodeID(SCENE_TREE[14])), Int[ ])
test_node_iter(children(ST_NodeID(SCENE_TREE[15])), Int[ ])
test_node_iter(children(ST_NodeID(SCENE_TREE[16])), Int[ ])
test_node_iter(children(ST_NodeID(SCENE_TREE[17])), Int[ ])
test_node_iter(children(ST_NodeID(SCENE_TREE[18])), Int[ ])
test_node_iter(children(ST_NodeID(SCENE_TREE[19])), Int[ ])

##  Parents  ##

# Root nodes:
test_node_iter(parents(ST_NodeID(SCENE_TREE[1])), Int[ ])
test_node_iter(parents(ST_NodeID(SCENE_TREE[2])), Int[ ])
test_node_iter(parents(ST_NodeID(SCENE_TREE[3])), Int[ ])
test_node_iter(parents(ST_NodeID(SCENE_TREE[4])), Int[ ])
# Direct children:
test_node_iter(parents(ST_NodeID(SCENE_TREE[5])), [ 1 ])
test_node_iter(parents(ST_NodeID(SCENE_TREE[6])), [ 1 ])
test_node_iter(parents(ST_NodeID(SCENE_TREE[7])), [ 1 ])
test_node_iter(parents(ST_NodeID(SCENE_TREE[8])), [ 2 ])
test_node_iter(parents(ST_NodeID(SCENE_TREE[9])), [ 3 ])
test_node_iter(parents(ST_NodeID(SCENE_TREE[10])), [ 3 ])
# Grand-children:
test_node_iter(parents(ST_NodeID(SCENE_TREE[11])), [ 5, 1 ])
test_node_iter(parents(ST_NodeID(SCENE_TREE[12])), [ 7, 1 ])
test_node_iter(parents(ST_NodeID(SCENE_TREE[13])), [ 7, 1 ])
test_node_iter(parents(ST_NodeID(SCENE_TREE[14])), [ 9, 3 ])
test_node_iter(parents(ST_NodeID(SCENE_TREE[15])), [ 9, 3 ])
test_node_iter(parents(ST_NodeID(SCENE_TREE[16])), [ 9, 3 ])
test_node_iter(parents(ST_NodeID(SCENE_TREE[17])), [ 9, 3 ])
test_node_iter(parents(ST_NodeID(SCENE_TREE[18])), [ 9, 3 ])
test_node_iter(parents(ST_NodeID(SCENE_TREE[19])), [ 9, 3 ])

##  Family (all variants)  ##

"Helper function to test both breadth-first and depth-first family iteration."
function test_family(start::Int,
                     # Expected values should exclude the root node.
                     expected_depth_first::Vector{Int},
                     expected_breadth_first::Vector{Int})
    # Test depth-first, both with and without the root node.
    test_node_iter(family(ST_NodeID(SCENE_TREE[start]), false),
                   expected_depth_first)
    test_node_iter(family(ST_NodeID(SCENE_TREE[start]), true),
                   vcat(start, expected_depth_first))

    # Test breadth-first, both with and without the root node.
    test_node_iter(family_breadth_first(ST_NodeID(SCENE_TREE[start]), false),
                   expected_breadth_first)
    test_node_iter(family_breadth_first(ST_NodeID(SCENE_TREE[start]), true),
                   vcat(start, expected_breadth_first))

    #TODO: Also test family_breadth_first_deep, once it's implemented
end

# Root nodes:
test_family(1,
    [  5, 11,    6,    7, 12, 13  ],
    [  5, 6, 7,    11, 12, 13  ]
)
test_family(2, [ 8 ], [ 8 ])
test_family(3,
    [  9, 14, 15, 16, 17, 18, 19,   10  ],
    [  9, 10,    14, 15, 16, 17, 18, 19  ]
)
test_family(4, Int[ ], Int[ ])
# Direct children:
test_family(5, [ 11 ], [ 11 ])
test_family(6, Int[ ], Int[ ])
test_family(7, [ 12, 13 ], [ 12, 13 ])
test_family(8, Int[ ], Int[ ])
test_family(9, [ 14, 15, 16, 17, 18, 19 ], [ 14, 15, 16, 17, 18, 19 ])
test_family(10, Int[ ], Int[ ])
# Grand-children:
test_family(11, Int[ ], Int[ ])
test_family(12, Int[ ], Int[ ])
test_family(13, Int[ ], Int[ ])
test_family(14, Int[ ], Int[ ])
test_family(15, Int[ ], Int[ ])
test_family(16, Int[ ], Int[ ])
test_family(17, Int[ ], Int[ ])
test_family(18, Int[ ], Int[ ])
test_family(19, Int[ ], Int[ ])


println("#TODO: Test coordinate transformations")