"A descriptive short-hand for number types ('i8', 'u64', 'f32', etc)"
type_str(::Type{Int8}) = "i8"
type_str(::Type{Int16}) = "i16"
type_str(::Type{Int32}) = "i32"
type_str(::Type{Int64}) = "i64"
type_str(::Type{Int128}) = "i128"

type_str(::Type{UInt8}) = "u8"
type_str(::Type{UInt16}) = "u16"
type_str(::Type{UInt32}) = "u32"
type_str(::Type{UInt64}) = "u64"
type_str(::Type{UInt128}) = "u128"

type_str(::Type{Float16}) = "f16"
type_str(::Type{Float32}) = "f32"
type_str(::Type{Float64}) = "f64"

type_str(::Type{Bool}) = "b"

export type_str