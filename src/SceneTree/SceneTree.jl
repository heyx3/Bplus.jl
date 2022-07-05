"
A scene-tree implementation, agnostic as to its memory layout.
You can embed it in an ECS, or just throw Nodes into a Vector.
"
module SceneTree

using Setfield
using ..Utilities, ..Math

# Define @bp_scene_tree_assert.
@make_toggleable_asserts bp_scene_tree_

include("data.jl")
include("interface.jl")
include("node.jl")

end # module