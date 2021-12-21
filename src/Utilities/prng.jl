using Random, Setfield


##################################

# Define an immutable version of the PRNG first, which the mutable version will wrap.

"
An immutable version of PRNG, to avoid heap allocations.
Any calls to rand() with this PRNG will return a tuple of 1) the result,
    and 2) the new 'mutated' ConstPRNG.

This is a break from the typical AbstractRNG interface,
    so it only supports a specific set of rand() calls.

You can construct a PRNG with its fields (`UInt32` and `NTuple{3, UInt32}`).
You can also construct it with any number of scalar data parameters,
   to be hashed into seeds.
If no seeds are given, one is generated with `rand(UInt32)`.

The ideal number of seed bits is at least 96 (e.x. three 32-bit values);
   less bits means more warmup time to mix the limited seed data
   while extra bits cause a tiny bit of extra work to hash them with the first 96.
"
struct ConstPRNG <: Random.AbstractRNG
    state::UInt32
    seeds::NTuple{3,UInt32}

    function ConstPRNG( state::S,
                        seeds::Tuple{S1, S2, S3}
                      ) where {S<:Integer, S1<:Integer, S2<:Integer, S3<:Integer}
        new(convert(UInt32, state),
            map(s -> convert(UInt32, s), seeds))
    end
end

const PRNG_INITIAL_STATE = 0xf1ea5eed
@inline @generated function ConstPRNG(seeds...)::ConstPRNG
    is_valid_seed_type(seed_type) = (seed_type <: Union{Scalar8, Scalar16, Scalar32, Scalar64, Scalar128})
    @bp_check(all(is_valid_seed_type, seeds),
              "Unexpected arguments, expected primitive scalar types: ",
                 join(collect(enumerate(filter(is_valid_seed_type, seeds))), ", "))

    # The seeds are natively 32-bit values.
    # If we have any other bit sizes,
    #    combine/split them into 32-bit, then call this function again.
    # If we have any larger-bit values, split them apart.
    if any(T -> !(T <: Scalar32), seeds)
        vars_expr = quote end  # For declaring local variables
        call_params = Expr(:tuple)  # The arguments to the recursive call

        # When a scalar8 or scalar16 is taken from the end of the array,
        #    replace its type with 'nothing' to indicate it's no longer available.
        seeds_vec::Optional = collect(Optional, seeds)

        # Note: If there's an odd number of scalar8's or scalar16's,
        #   remember the last ones, which had no partner.
        # They'll be handled separately.
        lone_s8_idx::Optional{Int} = nothing
        lone_s16_idx::Optional{Int} = nothing

        # We're modifying the elements of 'seeds_vec',
        #    so we have to iterate over the indices and not the values.
        for i::Int in 1:length(seeds_vec)
            T = seeds_vec[i]
            if isnothing(T)
                continue
            elseif T <: Scalar8
                # Find another scalar8 to combine it with.
                other_s8_idx = findfirst(j -> exists(seeds_vec[j]) && (seeds_vec[j] <: Scalar8),
                                         (i+1) : length(seeds_vec))
                if exists(other_s8_idx)
                    other_s8_idx += i  # findfirst() gives the index into the range, and the range starts at i+1
                    push!(call_params.args, :(
                        UInt16(reinterpret(UInt8, seeds[$i])) |
                        (UInt16(reinterpret(UInt8, seeds[$other_s8_idx])) << 8)
                    ))
                    seeds_vec[other_s8_idx] = nothing
                else
                    @bp_utils_assert(isnothing(lone_s8_idx),
                                     "Found two 'lone' s8 indices: $i and $lone_s8_idx")
                    lone_s8_idx = i
                end
            elseif T <: Scalar16
                # Find another scalar16 to combine it with.
                other_s16_idx = findfirst(j -> exists(seeds_vec[j]) && (seeds_vec[j] <: Scalar16),
                                          (i+1) : length(seeds_vec))
                # If one wasn't found, just cast the data to 32-bit.
                if exists(other_s16_idx)
                    other_s16_idx += i  # findfirst() gives the index into the range, and the range starts at i+1
                    push!(call_params.args, :(
                        UInt32(reinterpret(UInt16, seeds[$i])) |
                        (UInt32(reinterpret(UInt16, seeds[$other_s16_idx])) << 16)
                    ))
                    seeds_vec[other_s16_idx] = nothing
                else
                    @bp_utils_assert(isnothing(lone_s16_idx),
                                     "Found two 'lone' s16 indices: $i and $lone_s16_idx")
                    lone_s16_idx = i
                end
            elseif T <: Scalar32
                push!(call_params.args, :( reinterpret(UInt32, seeds[$i]) ))
            elseif T <: Scalar64
                var_name = Symbol(:u64_, i)
                push!(vars_expr.args, :(
                    $var_name::UInt64 = reinterpret(UInt64, seeds[$i])
                ))
                append!(call_params.args, [
                    :( UInt32($var_name & 0x00000000ffffffff) ),
                    :( UInt32($var_name >> 32) ),
                ])
            elseif T <: Scalar128
                var_name = Symbol(:u128, i)
                push!(vars_expr.args, :(
                    $var_name::UInt128 = reinterpret(UInt128, seeds[$i])
                ))
                append!(call_params.args, [
                    :( UInt32(($var_name & 0x000000000000000000000000ffffffff) >> 0) ),
                    :( UInt32(($var_name & 0x0000000000000000ffffffff00000000) >> 32) ),
                    :( UInt32(($var_name & 0x00000000ffffffff0000000000000000) >> 64) ),
                    :( UInt32(($var_name & 0xffffffff000000000000000000000000) >> 96) )
                ])
            else
                error("Unhandled case: ", T)
            end
        end

        # Handle leftover small-bit values.
        if exists(lone_s8_idx) && exists(lone_s16_idx)
            # Combine them together.
            push!(call_params.args, :(
                UInt32(reinterpret(UInt16, seeds[$lone_s16_idx])) |
                (UInt32(reinterpret(UInt8, seeds[$lone_s8_idx])) << 16)
            ))
        elseif exists(lone_s8_idx)
            push!(call_params.args, :( UInt32(reinterpret(UInt8, seeds[$lone_s8_idx])) ))
        elseif exists(lone_s16_idx)
            push!(call_params.args, :( UInt32(reinterpret(UInt16, seeds[$lone_s16_idx])) ))
        end

        return quote
            $vars_expr
            return ConstPRNG($(call_params.args...))
        end
    end

    # If we get here, then all seeds are 32-bit data.

    # If no arguments are given, generate a random starting seed.
    if length(seeds) == 0
        return :( ConstPRNG(rand(UInt32)) )
    # If there's only one argument, use the original initialization logic from the article.
    elseif length(seeds) == 1
        return quote
            seed::UInt32 = reinterpret(UInt32, seeds[1])
            rng::ConstPRNG = ConstPRNG(PRNG_INITIAL_STATE, (seed, seed, seed))
            # I believe the reason for this is to weed out non-random initial behavior,
            #    since we initialize 3 of the 4 state variables to the same value.
            for _ in 1:20
                (_, rng) = rand(rng, UInt32)
            end
            return rng
        end
    # If there's two seeds, we do something similar
    #    but don't need to run as many warm-up iterations.
    elseif length(seeds) == 2
        return quote
            u1::UInt32 = reinterpret(UInt32, seeds[1])
            u2::UInt32 = reinterpret(UInt32, seeds[2])
            rng::ConstPRNG = ConstPRNG(PRNG_INITIAL_STATE, (u1, u2, u1 ⊻ u2))
            #TODO: Test how many iterations are needed to give good results.
            for _ in 1:7
                (_, rng) = rand(rng, UInt32)
            end
            return rng
        end
    # If there's three seeds, we shouldn't need any warm-up iterations.
    elseif length(seeds) == 3
        return :( ConstPRNG(PRNG_INITIAL_STATE, (reinterpret(UInt32, seeds[1]),
                                                 reinterpret(UInt32, seeds[2]),
                                                 reinterpret(UInt32, seeds[3]))) )
    # If there are more than 3 seeds, hash the extras with the first 3.
    else
        make_seeds = quote
            seed1::UInt32 = reinterpret(UInt32, seeds[1])
            seed2::UInt32 = reinterpret(UInt32, seeds[2])
            seed3::UInt32 = reinterpret(UInt32, seeds[3])
        end
        for i in 4:length(seeds)
            seed_i = mod1(i, 3)
            push!(make_seeds.args, :(
                $(Symbol(:seed, seed_i)) ⊻= reinterpret(UInt32, seeds[$i])
            ))
        end
        return quote
            $make_seeds
            return ConstPRNG(PRNG_INITIAL_STATE, (seed1, seed2, seed3))
        end
    end
end

Base.copy(r::ConstPRNG) = r

export ConstPRNG

#####################################

"
Outputs a tuple of enough data to make 96 bits of PRNG input,
    given the existing data that will be fed in
    and a seed value to customize this output.
"
fill_in_seeds(existing) = fill_in_seeds(Val(sizeof(existing)))
fill_in_seeds(existing, hash_seed::UInt32) = fill_in_seeds(Val(sizeof(existing)), hash_seed)

@inline fill_in_seeds(::Val{1}, hasher::UInt32) = (0x23456789 ⊻ hasher,
                                                   0x34567890 ⊻ hasher,
                                                   0x4567 ⊻ UInt16(hasher & 0x0000ffff) ⊻
                                                             UInt16(hasher >> 16))
@inline fill_in_seeds(::Val{2}, hasher::UInt32) = fill_in_seeds(Val(1), hasher ⊻ 0xabcdef09)
@inline fill_in_seeds(::Val{3}, hasher::UInt32) = (0x34567890 ⊻ hasher,
                                                   0x4567890A ⊻ hasher,
                                                   0x76 ⊻ UInt8(hasher & 0x000000ff) ⊻
                                                          UInt8(hasher >> 24))

@inline fill_in_seeds(::Val{4}, hasher::UInt32) = (0x567890AB ⊻ hasher,
                                                   0x67890ABC ⊻ hasher)
@inline fill_in_seeds(::Val{5}, hasher::UInt32) = (0x7890ABCD ⊻ hasher,
                                                   0x5476 ⊻ UInt16(hasher & 0x0000ffff) ⊻
                                                            UInt16(hasher >> 16))
@inline fill_in_seeds(::Val{6}, hasher::UInt32) = fill_in_seeds(Val(5), hasher ⊻ 0x90fedcba)

@inline fill_in_seeds(::Val{7}, hasher::UInt32) = (0x890ABCDE ⊻ hasher,
                                                   0x67 ⊻ UInt8(hasher & 0x000000ff) ⊻
                                                          UInt8(hasher >> 24))

@inline fill_in_seeds(::Val{8}, hasher::UInt32) = (0x90ABCDEF ⊻ hasher, )
@inline fill_in_seeds(::Val{9}, hasher::UInt32) = (0xCBFE ⊻ UInt16(hasher & 0x0000ffff) ⊻
                                                            UInt16(hasher >> 16),
                                                  )
@inline fill_in_seeds(::Val{10}, hasher::UInt32) = fill_in_seeds(Val(9), hasher ⊻ 0x56473829)
@inline fill_in_seeds(::Val{11}, hasher::UInt32) = (0xAA ⊻ UInt8(hasher & 0x000000ff) ⊻
                                                           UInt8(hasher >> 24),
                                                   )

# Larger input data doesn't need any extra seeds.
fill_in_seeds(::Val, hasher::UInt32) = (hasher, )
@inline function fill_in_seeds(::Val{N}) where {N}
    if (N > 11)
        return ()
    else
        return fill_in_seeds(Val(N), 0x00000000)
    end
end

export fill_in_seeds

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