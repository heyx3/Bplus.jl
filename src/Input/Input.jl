module Input

using GLFW, StructTypes, MacroTools, Setfield
using ..Utilities, ..Math, ..GL

@make_toggleable_asserts bp_input_

@decentralized_module_init

include("mappings.jl")
include("inputs.jl")
include("service.jl")

end # module