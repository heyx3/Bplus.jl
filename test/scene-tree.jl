# Create a simple test scene-tree backed by an array of reference-types.

const ST_NodeID = Int
const ST_Node = SceneTree.Node{Int, Float32}

mutable struct ST_Entity
    transform::Node{Int, Float32}
end

const ST_World = Vector{ST_Entity}


const SCENE_TREE = ST_Entity[
]
println("#TODO: Add SceneTree unit tests once we have more constructor/tree logic")