using Random


##################################

# Define an immutable version of the PRNG first, which the mutable version will wrap.

"
An immutable version of PRNG, to avoid heap allocations.
Any calls to rand() with this PRNG will return a tuple of
   1) the result, and 2) the new 'mutated' ConstPRNG.
This is a break from the typical AbstractRNG interface,
    so it only supports a specific set of rand() calls.
"
struct ConstPRNG <: Random.AbstractRNG
    state::UInt32
    seeds::NTuple{3,UInt32}
    ConstPRNG(state, seeds) = new(state, seeds)
end
function ConstPRNG(seed::UInt32 = rand(UInt32))
    rng::ConstPRNG = ConstPRNG(0xf1ea5eed, (seed, seed, seed))

    # Run some iterations before-hand.
    # I believe the reason for this is to weed out strange initial behavior,
    #    since we initialize 3 of the 4 state variables to the same value.
    for _ in 1:20
        (x, rng) = rand(rng, UInt32)
    end

    return rng
end

Base.copy(r::ConstPRNG) = r

export ConstPRNG

#####################################

"
A fast, strong PRNG, taken from http://burtleburtle.net/bob/rand/smallprng.html.
Works natively in 32-bit math, other bit sizes require extra work.
"
mutable struct PRNG <: Random.AbstractRNG
    rng::ConstPRNG

    PRNG(rng::ConstPRNG) = new(rng)
    PRNG(args...) = new(ConstPRNG(args...))
end
export PRNG

Base.copy(r::PRNG) = PRNG(r.rng)

# Implement Random.rand() by calling into the underlying immutable version.
@inline function Random.rand(r::PRNG, t::Type{<:Number})
    (result, r.rng) = rand(r.rng, t)
    return result
end


##############################################


# The core algorithm is for generating UInt32 data.
@inline function Random.rand(r::ConstPRNG, ::Type{UInt32})
    (state, seeds) = (r.state, r.seeds)
    seed4::UInt32 = state - prng_rot(seeds[1], UInt32(27))
    state = seeds[1] ⊻ prng_rot(seeds[2], UInt32(17))
    seeds = (
        seeds[2] + seeds[3],
        seeds[3] + seed4,
        seed4 + state
    )
    return (seeds[3], ConstPRNG(state, seeds))
end


"Mixes the bits in a 32-bit number, based on another one"
prng_rot(val::UInt32, amount::UInt32)::UInt32 = (
    (val << amount) |
    (val >> (UInt32(32) - amount))
)


# For signed integers, use the unsigned math and reinterpret the bits.
function Random.rand(r::ConstPRNG, ::Type{S}) where {S <: Signed}
    (u_result, r) = rand(r, unsigned(S))
    return (signed(u_result), r)
end

# For larger uints, concatenate smaller ones together.
function Random.rand(r::ConstPRNG, ::Type{UInt64})
    (a, r) = rand(r, UInt32)
    (b, r) = rand(r, UInt32)
    value = (UInt64(a) << 32) | UInt64(b)
    return (value, r)
end
function Random.rand(r::ConstPRNG, ::Type{UInt128})
    (a, r) = rand(r, UInt64)
    (b, r) = rand(r, UInt64)
    value = (UInt128(a) << 64) | UInt128(b)
    return (value, r)
end

# For smaller uints, mix both halves of the larger ones.
function Random.rand(r::ConstPRNG, ::Type{UInt16})
    (a::UInt32, r) = rand(r, UInt32)
    value = UInt16(a >> 16) ⊻
            UInt16(a % UInt16)  # The '%' operator here truncates the int's largest bits
                                #    to fit it into a UInt16.
    return (value, r)
end
function Random.rand(r::ConstPRNG, ::Type{UInt8})
    (a::UInt16, r) = rand(r, UInt16)
    value = UInt8(a >> 8) ⊻
            UInt8(a % UInt8)  # The '%' operator here truncates the int's largest bits
                              #      to fit it into a UInt8.
    return (value, r)
end

# For floats, keep a constant sign/exponent and randomize the other bits
#    to get a uniform-random value between 1 and 2.
function Random.rand(r::ConstPRNG, ::Type{Float16})
    (u, r) = rand(r, UInt16)
    value = -1 + reinterpret(Float16,
        0b0011110000000000 |
       (0b0000001111111111 & u)
    )
    return (value, r)
end
function Random.rand(r::ConstPRNG, ::Type{Float32})
    (u, r) = rand(r, UInt32)
    value = -1 + reinterpret(Float32, 
        0b00111111100000000000000000000000 |
       (0b00000000011111111111111111111111 & u)
    )
    return (value, r)
end
function Random.rand(r::ConstPRNG, ::Type{Float64})
    (u, r) = rand(r, UInt64)
    value = -1 + reinterpret(Float64,
        0b0011111111110000000000000000000000000000000000000000000000000000 |
       (0b0000000000001111111111111111111111111111111111111111111111111111 & u)
    )
    return (value, r)
end

# For boolean, I probably don't need to explain this.
function Random.rand(r::ConstPRNG, ::Type{Bool})
    (u, r) = rand(r, UInt32)
    value = convert(Bool, 0x1 & u)
    return (value, r)
end


#########################################

# Re-implement some built-in rand() functions that are very useful.

function Random.rand(rng::ConstPRNG, sp::Random.SamplerRangeNDL{U,T}) where {U,T}
    s = sp.s
    (_x, rng) = rand(rng, U)
    x = widen(_x)
    m = x * s
    l = m % U
    if l < s
        t = mod(-s, s) # as s is unsigned, -s is equal to 2^L - s in the paper
        while l < t
            (_x, rng) = rand(rng, U)
            x = widen(_x)
            m = x * s
            l = m % U
        end
    end
    
    result = (s == 0 ? x : m >> (8*sizeof(U))) % T + sp.a
    return (result, rng)
end
function Random.rand(rng::ConstPRNG, sp::Random.SamplerSimple{<:AbstractArray,<:Random.Sampler})
    (idx, rng) = rand(rng, sp.data)
    @inbounds return (sp[][idx], rng)
end
function Random.rand(rng::ConstPRNG, sp::Random.SamplerSimple{<:Dict,<:Random.Sampler})
    while true
        (i, rng) = rand(rng, sp.data)
        Base.isslotfilled(sp[], i) && @inbounds return ((sp[].keys[i] => sp[].vals[i]), rng)
    end
end
function Random.rand(rng::ConstPRNG, sp::Random.SamplerTag{<:Set,<:Random.Sampler})
    (result, rng) = rand(rng, sp.data)
    return (result.first, rng)
end
function Random.rand(rng::ConstPRNG, sp::Random.SamplerSimple{BitSet,<:Random.Sampler})
    while true
        (n, rng) = rand(rng, sp.data)
        n in sp[] && return (n, rng)
    end
end
function Random.rand(rng::ConstPRNG, sp::Random.SamplerTrivial{<:Union{AbstractDict,AbstractSet}})
    (result, rng) = rand(rng, 1:length(sp[]))
    return (Random.nth(sp[], result), rng)
end
function Random.rand(rng::ConstPRNG, sp::Random.SamplerSimple{<:AbstractString,<:Random.Sampler})::Tuple{Char, ConstPRNG}
    str = sp[]
    while true
        (pos, rng) = rand(rng, sp.data)
        Random.isvalid_unsafe(str, pos) && return (str[pos], rng)
    end
end
function Random.rand(rng::ConstPRNG, sp::Random.SamplerTrivial{Tuple{A}}) where {A}
    @inbounds return (sp[][1], rng)
end
function Random.rand(rng::ConstPRNG, sp::Random.SamplerSimple{Tuple{A,B}}) where {A,B}
    (idx, rng) = rand(rng, Bool)
    @inbounds return (sp[][idx ? 1 : 2], rng)
end
function Random.rand(rng::ConstPRNG, sp::Random.SamplerSimple{Tuple{A,B,C}}) where {A,B,C}
    (idx, rng) = rand(rng, UInt32(1):UInt32(3))
    @inbounds return (sp[][idx], rng)
end
function Random.rand(rng::ConstPRNG, sp::Random.SamplerSimple{T}) where T<:Tuple
    if fieldcount(T) < typemax(UInt32)
        (idx, rng) = rand(rng, UInt32(1):UInt32(fieldcount(T)))
        @inbounds return (sp[][idx], rng)
    else
        (idx, rng) = rand(rng, 1:fieldcount(T))
        @inbounds return (sp[][idx], rng)
    end
end


"
Picks a random element from an ntuple.
Unfortunately, `Random.rand(::ConstPRNG, ::NTuple)` has unavoidable type ambiguity.
"
function rand_ntuple(rng::ConstPRNG, t::NTup) where {NTup<:NTuple{<:Any}}
    (idx, rng) = rand(rng, 1:length(t))
    return (t[idx], rng)
end
export rand_ntuple