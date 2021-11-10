using Random

"
Iterates through the values 1:N in pseudo-random order,
   without requiring any heap allocations.
"
struct RandIterator{I<:Integer, RNG<:AbstractRNG}
    # Stolen from: https://stackoverflow.com/a/28855799
    max::I
    scale::I
    offset::I

    rng::RNG
    n::I
end

@inline function RandIterator(n::I, initializer::T_RNG = ConstPRNG()) where {I<:Integer, T_RNG}
    m::I = Base._nextpow2(max(I(8), n))

    c_result = rand(initializer, zero(I):((m * I(5)) ÷ I(6)))
    if T_RNG == ConstPRNG
        (c::I, initializer) = c_result
    else
        c = c_result
    end
    c += m ÷ I(6)
    c |= one(I)

    a_result = rand(initializer, zero(I):(m ÷ I(6)))
    if T_RNG == ConstPRNG
        (a::I, initializer) = a_result
    else
        a = a_result
    end
    a *= m ÷ I(12)
    a = (a * I(4)) + one(I)

    return RandIterator{I, T_RNG}(m, a, c, copy(initializer), n)
end

export RandIterator


Base.eltype(::RandIterator{I, RNG}) where {I, RNG} = I
Base.length(it::RandIterator) = it.n

@inline function Base.iterate(it::RandIterator{I, RNG}) where {I, RNG}
    # Pick the initial index.
    _rand_i = rand(it.rng, one(I):it.n)
    if RNG == ConstPRNG  # Our special immutable RNG
        rand_i = _rand_i[1]
    else
        rand_i = _rand_i
    end
    first_val = abs(rand_i) % it.n

    # Convert from 0-based math to 1-based Julia
    first_val += 1

    return (first_val, (first_val, first_val))
end
@inline function Base.iterate(it::RandIterator{I, RNG}, state::NTuple{2, I}) where {I, RNG}
    (prev_val::I, initial_val::I) = state

    # Convert from 1-based Julia to 0-based math, then convert back when done
    prev_val -= 1

    next_val::I = ((prev_val * it.scale) + it.offset) % it.max
    while next_val >= it.n  # Use >= because we're in 0-based math right now
        next_val = ((next_val * it.scale) + it.offset) % it.max
    end
    next_val += 1

    if next_val == initial_val
        return nothing
    else
        return (next_val, (next_val, initial_val))
    end
end



Base.show(io::IO, it::RandIterator) = print(io,
    "RandIterator<1:", it.n, ">"
)