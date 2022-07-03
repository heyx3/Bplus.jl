module Input

using GLFW, StructTypes, MacroTools
using ..Utilities, ..Math

@make_toggleable_asserts bp_input_


"Manages all inputs for a GLFW window."
mutable struct InputSystem
end
export InputSystem

"
The per-context input systems currently in existence,
    indexed by their GLFW window.
"
const SYSTEMS = Dict{GLFW.Window, InputSystem}()

end