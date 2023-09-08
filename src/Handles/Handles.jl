"
A B+ Context service that can draw a variety of 3D widgets, some of which are interactable.
They act a lot like other Dear ImGUI functions, but draw behind every window.
"
module Handles

using Setfield

using GLFW, CImGui

using ..Utilities, ..Math, ..GL, ..Input

@decentralized_module_init

"I think this packs info about a handle's editing plane and offset into a v4f"
function build_plane(p::v3f, normal::v3f)::Tuple{v3f, Float32}
    normal = vnorm(normal)
    return (normal, normal ⋅ p)
end

include("enums.jl")
include("style.jl")
include("state.jl")
include("consts.jl")


@bp_service Handles(force_unique) begin
    program::Program
    
end



end # module