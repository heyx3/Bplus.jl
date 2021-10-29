using Random

"
A fast, strong PRNG, taken from http://burtleburtle.net/bob/rand/smallprng.html.
Works natively in 32-bit math, other bit sizes require extra work.
"
mutable struct PRNG <: Random.AbstractRNG
    state::UInt32
    seeds::NTuple{3,UInt32}

    PRNG(state::UInt32, seeds::NTuple{3,UInt32}) = new(state, seeds)
    function PRNG(seed::UInt32 = rand(UInt32))
        rng::PRNG = new(0xf1ea5eed, (seed, seed, seed))

        # Run some iterations before-hand.
        # I believe this is needed to weed out strange initial behavior,
        #    since we initialize 3 of the 4 state variables to the same value.
        for i::Int in 1:20
            rand(rng, UInt32)
        end

        return rng
    end
end
export PRNG

Base.copy(r::PRNG) = PRNG(r.state, r.seeds)

# The core algorithm is for generating UInt32 data.
@inline function Base.rand(r::PRNG, ::Type{UInt32})
    seed4::UInt32 = r.state - prng_rot(r.seeds[1], UInt32(27))
    r.state = r.seeds[1] ⊻ prng_rot(r.seeds[2], UInt32(17))
    r.seeds = (
        r.seeds[2] + r.seeds[3],
        r.seeds[3] + seed4,
        seed4 + r.state
    )
    return r.seeds[3]
end


"Mixes the bits in a 32-bit number, based on another one"
prng_rot(val::UInt32, amount::UInt32)::UInt32 = (
    (val << amount) |
    (val >> (UInt32(32) - amount))
)


# For signed integers, use the unsigned math and reinterpret the bits.
Base.rand(r::PRNG, ::Type{S}) where {S <: Signed} = signed(rand(r, unsigned(S)))

# For larger uints, concatenate smaller ones together.
Base.rand(r::PRNG, ::Type{UInt64}) = (
    (UInt64(rand(r, UInt32)) << 32) |
    (UInt64(rand(r, UInt32)))
)
Base.rand(r::PRNG, ::Type{UInt128}) = (
    (UInt128(rand(r, UInt64)) << 64) |
    (UInt128(rand(r, UInt64)))
)

# For smaller uints, mix both halves of the larger ones.
function Base.rand(r::PRNG, ::Type{UInt16})
    a::UInt32 = rand(r, UInt32)
    return UInt16(a >> 16) ⊻
           UInt16(a % UInt16)  # The '%' operator here truncates the int's largest bits
                               #    to fit it into a UInt16.
end
function Base.rand(r::PRNG, ::Type{UInt8})
    a::UInt16 = rand(r, UInt16)
    return UInt8(a >> 8) ⊻
           UInt8(a % UInt8)  # The '%' operator here truncates the int's largest bits
                             #      to fit it into a UInt8.
end

# For floats, keep a constant sign/exponent and randomize the other bits
#    to get a uniform-random value between 1 and 2.
Base.rand(r::PRNG, ::Type{Float16}) = -1 + reinterpret(Float16,
    0b0011110000000000 |
   (0b0000001111111111 & rand(r, UInt16))
)
Base.rand(r::PRNG, ::Type{Float32}) = -1 + reinterpret(Float32, 
    0b00111111100000000000000000000000 |
   (0b00000000011111111111111111111111 & rand(r, UInt32))
)
Base.rand(r::PRNG, ::Type{Float64}) = -1 + reinterpret(Float64,
    0b0011111111110000000000000000000000000000000000000000000000000000 |
   (0b0000000000001111111111111111111111111111111111111111111111111111 & rand(r, UInt64))
)

# For boolean, I probably don't need to explain this.
Base.rand(r::PRNG, ::Type{Bool}) = convert(Bool, 0x1 & rand(r, UInt32))


# Based on the docs, all other rand() overloads should work automatically, given the above ones.