"
A data representation of continuous math.
Useful for procedural generation and shader stuff.
"
module Fields

using Setfield, StaticArrays, DataStructures

using ..Utilities, ..Math, ..GL

# Define @bp_fields_assert.
@make_toggleable_asserts bp_fields_


"
A function of (`Vec{NIn, F} -> Vec{NOut, F}`.
For example, 3D perlin noise could be a `Field{3, 1, Float32}`.
"
abstract type AbstractField{NIn, NOut, F<:AbstractFloat} end
export AbstractField

field_input_size(f::AbstractField) = field_input_size(typeof(f))
field_input_size(::Type{<:AbstractField{NIn}}) where {NIn} = NIn

field_output_size(f::AbstractField) = field_output_size(typeof(f))
field_output_size(::Type{<:AbstractField{NIn, NOut}}) where {NIn, NOut} = NOut

field_component_type(f::AbstractField) = field_component_type(typeof(f))
field_component_type(::Type{<:AbstractField{NIn, NOut, F}}) where {NIn, NOut, F} = F


include("interface.jl")
include("dsl.jl")

include("basics.jl")
include("modifiers.jl")
include("math.jl")
#TODO: Vector ops
#TODO: Logical ops
#TODO: Noise
#TODO: Signed distance fields
#TODO: Useful CG curves (e.x. https://iquilezles.org/articles/functions/)
#TODO: Think of more

#TODO: Shader generation

include("outputs.jl")

end # module