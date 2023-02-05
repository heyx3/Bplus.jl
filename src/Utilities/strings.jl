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