"""
Game math is mostly done with 32-bit floats,
   especially when interacting with computer graphics.
This is a quick short-hand for making a 32-bit float.
"""
macro f32(value)
    return :(Float32($(esc(value))))
end
export @f32


const Scalar8 = Union{UInt8, Int8}
const Scalar16 = Union{UInt16, Int16, Float16}
const Scalar32 = Union{UInt32, Int32, Float32}
const Scalar64 = Union{UInt64, Int64, Float64}
const Scalar128 = Union{UInt128, Int128}
const ScalarBits = Union{Scalar8, Scalar16, Scalar32, Scalar64, Scalar128}
export Scalar8, Scalar16, Scalar32, Scalar64, Scalar128,
       ScalarBits