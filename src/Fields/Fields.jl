"
A data representation of continuous math.
Useful for procedural generation and shader stuff.
"
module Fields

using Setfield, StaticArrays

using ..Utilities, ..Math, ..GL

# Define @bp_fields_assert.
@make_toggleable_asserts bp_fields_


"
A function of (`Vec{NIn, F} -> Vec{NOut, F}`.
For example, 3D perlin noise could be a `Field{3, 1, Float32}`.
"
abstract type AbstractField{NIn, NOut, F<:AbstractFloat} end


include("interface.jl")
#TODO: Shader generation
#TODO: Parsing from a DSL, emitting back to that DSL

include("basics.jl")
include("math.jl")
include("swizzling.jl")
#TODO: Other position modifiers
#TODO: Vector ops
#TODO: Logical ops
#TODO: Signed distance fields
#TODO: "ReusedField" : a mutable field type, that can spawn immutable fields referencing it. For "let" statements in the DSL
#TODO: Think of new ones



end # module