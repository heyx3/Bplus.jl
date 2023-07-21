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

type_str(T) = "?$T?"

export type_str


"""
A decorator for numbers that outputs their bit pattern for both `print()` and `show()`.
  E.x. "0b00111011".

The number type `TNum` must support the following:

    * `sizeof()` to determine the max number of bits.
        * If your type doesn't do this, overload `binary_size(n::TNum)`
            or `binary_size(::Type{TNum})`.
    * To check individual bits: bitwise `&` and `<<`;
        `zero(TNum)` that makes a value with all bits set to `0`;
        `one(TNum)` that makes a value with all bits set to `0` *except* the lowest bit.
        * If your type doesn't do this, overload `is_bit_one(a::TNum, bit_idx::Int)::Bool`
"""
struct Binary{TNum}
    value::TNum
    print_leading_zeroes::Bool
    function Binary(n::TNum, print_leading_zeroes::Bool = false) where {TNum}
        return new{TNum}(n, print_leading_zeroes)
    end
end
Base.print(io::IO, b::Binary{TNum}) where {TNum} = show(io, b)
function Base.show(io::IO, b::Binary{TNum}) where {TNum}
    print(io, "0b")

    still_leading_zeroes::Bool = true
    for bit_i in (binary_size(b.value) - 1) : -1 : 0
        if is_bit_one(b.value, bit_i)
            still_leading_zeroes = false
            print(io, '1')
        elseif !still_leading_zeroes || b.print_leading_zeroes
            print(io, '0')
        end
    end

    # Make sure at least one digit is printed.
    if still_leading_zeroes && !b.print_leading_zeroes
        print(io, '0')
    end
end

"Returns whether the given bit of a number (0-based index) is `1`."
@inline function is_bit_one(n::TNum, idx::Int)::Bool where {TNum}
    bit_pattern::TNum = one(TNum) << idx
    return (n & bit_pattern) !== zero(TNum)
end
is_bit_one(n::Float16, idx::Int)::Bool = is_bit_one(reinterpret(UInt16, n), idx)
is_bit_one(n::Float32, idx::Int)::Bool = is_bit_one(reinterpret(UInt32, n), idx)
is_bit_one(n::Float64, idx::Int)::Bool = is_bit_one(reinterpret(UInt64, n), idx)

@inline binary_size(number) = binary_size(typeof(number))
@inline binary_size(T::Type) = sizeof(T) * 8
@inline binary_size(::Union{Type{BigInt}, Type{BigFloat}}) = error("Binary decorator not implemented for BigInt and BigFloat because I'm lazy")

export Binary


"
Combines a Julia UTF-8 string with a null-terminated C-string buffer.

Use `update!()` to set the Julia string or recompute it from the C buffer.
"
mutable struct InteropString
    julia::String
    c_buffer::Vector{UInt8}

    function InteropString(s::String, capacity_multiple::Int = 3)
        s_bytes = codeunits(s)
        is = new(s, Vector{UInt8}(undef, length(s_bytes) * capacity_multiple))

        copyto!(is.c_buffer, s_bytes)
        is.c_buffer[length(s_bytes) + 1] = 0 # Add a null terminator

        return is
    end
end
setproperty!(::InteropString, ::Symbol, ::Any) = error("Don't set an InteropString's fields. Use update!() instead")

"Sets the string value with a Julia string, updating the C buffer accordingly"
function update!(is::InteropString, julia::String)
    setfield!(is, :julia, julia)

    s_bytes = codeunits(julia)
    # Resize if needed; make sure there's room for the null terminator too.
    if length(is.c_buffer) < (length(s_bytes) + 1)
        resize!(is.c_buffer, max(2, length(s_bytes) * 2))
    end

    copyto!(is.c_buffer, s_bytes)
    is.c_buffer[length(s_bytes) + 1] = 0 # Null terminator
end
"Updates the Julia string to reflect the contents of the C buffer"
function update!(is::InteropString)
    null_terminator_idx::Optional{Int} = findfirst(iszero, is.c_buffer)
    if isnothing(null_terminator_idx)
        error("No null terminator found in C string buffer")
    end
    setfield!(is, :julia, String(@view is.c_buffer[1 : null_terminator_idx-1]))
end

export InteropString, update!