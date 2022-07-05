# Test that ConstPRNG doesn't allocate.
@bp_test_no_allocations(typeof(rand(ConstPRNG(0x12345678), UInt)[1]), UInt)
@bp_test_no_allocations(typeof(rand(ConstPRNG(0x12345678), Int8)[1]), Int8)
@bp_test_no_allocations(typeof(rand(ConstPRNG(0x12345678), Bool)[1]), Bool)
@bp_test_no_allocations(rand(ConstPRNG(0x12345678), (1, 2.0))[1] isa Union{Int, Float64}, true)
@bp_test_no_allocations(typeof(rand_ntuple(ConstPRNG(0x12345678), (1, 2, 4))[1]), Int)
@bp_test_no_allocations(typeof(rand(ConstPRNG(0x12345678), 5:2000)[1]), Int)
@bp_test_no_allocations(typeof(rand(ConstPRNG(0x12345678), 5:10:2000)[1]), Int)
@bp_test_no_allocations(typeof(rand(ConstPRNG(0x12345678), 5.5:10:2000.5)[1]), Float64)
@bp_test_no_allocations(typeof(rand(ConstPRNG(0x12345678), range(1, length=13, stop=20))[1]), Float64)

"
Runs the following tests for an RNG with N input seeds of any scalar type:
    1. Tests that constructing it doesn't allocate
    2. Tests that each of the N seed values has an effect on the output
Assumes the existence of DataTypes T1, T2, ..., TN representing the seed types to test with.
"
macro run_prng_test_for(n::Int)
    types = map(i -> esc(Symbol(:T, i)), 1:n)
    # Randomly jumble the orders of the seed types, to broaden the test coverage.
    type_permute = :( type_permute::Vector = shuffle([ $(types...) ]) )

    make_rand_seeds = :( tuple(map(t -> rand(t), type_permute)...) )
    return quote
        $type_permute

        # Make sure the constructor doesn't allocate.
        seeds::Tuple = $make_rand_seeds
        @bp_test_no_allocations(typeof(ConstPRNG(seeds...)), ConstPRNG,
                                "Using ", $n, " seeds: ", join(type_permute, ", "))

        # Make sure each seed value is important.
        for seed_idx::Int in 1:$n
            seeds1 = $make_rand_seeds

            TT = tuple($(types...))[seed_idx]
            seeds2 = seeds1
            while seeds2[seed_idx] == seeds1[seed_idx]
                seeds2 = @set(seeds1[seed_idx] = rand(TT))
            end

            @bp_check(rand(ConstPRNG(seeds1...), UInt32) !=
                        rand(ConstPRNG(seeds2...), UInt32),
                      "No change detected from changing seed ", seed_idx, " in the param set ",
                        join([$(types...)], ","), ":\n\t", seeds1, " vs ", seeds2)
        end
    end
end
# Test every combination of bits types with up to 4 seeds.
print("\tRunning $(length(ALL_FLOATS)) sets of tests...\n\t\t")
# Simplify the sets of types tested to keep the tests fast.
type_combos = [
    map(tuple, ALL_FLOATS)...,
    Iterators.product(ALL_FLOATS, ALL_UNSIGNED)...,
    Iterators.product(ALL_FLOATS, ALL_UNSIGNED, ALL_SIGNED)...,
    Iterators.product(ALL_FLOATS, ALL_UNSIGNED, ALL_SIGNED, ALL_SIGNED)...
]
# Print some progress indicators to make it clear that tests are happening.
const INNER_LENGTH = length(ALL_SIGNED) * length(ALL_SIGNED)
const OUTER_LENGTH = length(ALL_UNSIGNED) * INNER_LENGTH
for (i::Int, type_combo::NTuple{N, DataType} where N) in enumerate(type_combos)
    n_types::Int = length(type_combo)
    # Print the progress indicators:
    i_0 = i-1
    if (i_0 % OUTER_LENGTH) == 0
        if (i_0 ÷ OUTER_LENGTH) > 0
            print(" | ")
        end
        print((i_0 ÷ OUTER_LENGTH) + 1, ":")
    elseif (i_0 % INNER_LENGTH) == 0
        count = (i_0 ÷ INNER_LENGTH) % length(ALL_UNSIGNED)
        if count > 1
            print(",")
        end
        print(count + 1) 
    end
    # Run the test:
    expr_make_seeds = map(t -> :( rand($t) ), collect(type_combo))
    @eval begin
        # Make sure the constructor doesn't allocate.
        @bp_test_no_allocations(typeof(ConstPRNG($(expr_make_seeds...))), ConstPRNG,
                                "Using ", $n_types, " seeds: ", join($type_combo, ", "))
    end
    # Make sure each seed value is important.
    for seed_idx::Int in 1:n_types
        seeds1 = eval(:( tuple($(expr_make_seeds...)) ))

        TT = type_combo[seed_idx]
        seeds2 = seeds1
        while seeds2[seed_idx] == seeds1[seed_idx]
            seeds2 = @set(seeds1[seed_idx] = rand(TT))
        end

        @bp_check(rand(ConstPRNG(seeds1...), UInt32) !=
                      rand(ConstPRNG(seeds2...), UInt32),
                  "No change detected from changing seed ", seed_idx, " in the param set ",
                    join(type_combo, ","), ":\n\t", seeds1, " vs ", seeds2)
    end
end
println()

# Test some ConstPRNG functionality.
for i in 1:1000
    @bp_test_no_allocations(rand_ntuple(ConstPRNG(), (3, 4, 5))[1] in (3, 4, 5), true)
    @bp_check(rand(ConstPRNG(), (3, 4.0, 5))[1] in (3, 4.0, 5))  # Apparently rand((3, 4.0, 5)) inherently has heap-allocations
    @bp_test_no_allocations(rand(ConstPRNG(), 4:21)[1] in 4:21, true)
    arr::Vector{Float64} = [ 4.0, 3.0, 1.0 ]
    @bp_test_no_allocations(rand(ConstPRNG(), arr)[1] in arr, true)
    @bp_test_no_allocations(rand(ConstPRNG(), range(1, length=13, stop=20))[1] >= 1, true)
    @bp_test_no_allocations(rand(ConstPRNG(), range(1, length=13, stop=20))[1] <= 20, true)
    @bp_check(rand(ConstPRNG(), Set([4, 5, 6, 7]))[1] in 4:7, true)
    @bp_check(rand(ConstPRNG(), Dict(4=>50, 5=>10))[1][1] in 4:5, true)
    @bp_check(rand(ConstPRNG(), "ghi")[1] in "ghi", true)
end

# Test RandIterator.
@bp_test_no_allocations(typeof(RandIterator(200)), RandIterator{Int, ConstPRNG})
@bp_test_no_allocations_setup(
    begin
        r::Vector{Int} = Vector{Int}(undef, 10)
    end,
    begin
        for (i, x) in enumerate(RandIterator(length(r)))
            @assert(i <= length(r), "Iterated past $(length(r))")
            r[i] = x
        end
        (r[1] + r[end]) <= ((length(r) * 2) - 1)
    end,
    true
)
# Generate a random iteration over 1:N, N times,
#    and assert that no iterations were the same.
# Statistically, it's impossible for such a thing to happen coincidentally.
orderings = map(i -> collect(RandIterator(1000)),
                1:1000)
@bp_check(all(o -> length(o) == length(orderings), orderings),
          "RandIterator 1:1000 generated ", length(o), " values")
@bp_check(allunique(orderings),
          "RandIterator(1000) managed to generate the same sequence more than once; this is virtually impossible with proper randomness")

const prng = PRNG(0x1234567)
const prng2 = PRNG(0x1234567)

const N_ITERATIONS = 999999  # A balance between speed and having enough samples

@bp_test_no_allocations(copy(prng) isa PRNG, true)

"Run two identical ConstPRNG's, check they're equal, and check there's no allocations"
@inline function test_compare_prngs( prng_args::TArg1,
                                     ::Type{T}
                                   ) where {TArg1<:Tuple, T}
    # Test the ConstPRNG.
    @bp_test_no_allocations(
        begin
            rng1 = ConstPRNG(prng_args...)
            rand(rng1, T)
        end,
        begin
            rng2 = ConstPRNG(prng_args...)
            rand(rng2, T)
        end,
        "Testing determinism of ConstPRNG(", join(prng_args, ", "), ")"
    )

    # Test the PRNG.
    @bp_test_no_allocations_setup(
        begin
            rng1 = PRNG(prng_args...)
            rng2 = PRNG(prng_args...)
        end,
        rand(rng1, T),
        rand(rng2, T),
        "Testing determinism of ConstPRNG(", join(prng_args, ", "), ")"
    )
end


# Test that the PRNG work is non-allocating and deterministic.
for nt in ALL_REALS
    test_compare_prngs((0x5678768, ), nt)
end

# Test that copies of a PRNG match the original.
const prng3 = PRNG()
for i in 1:N_ITERATIONS
    p3::PRNG = copy(prng3)
    @bp_test_no_allocations(rand(copy(prng3), UInt32),
                            rand(prng3, UInt32),
                            "For ", UInt32)
end

# Test that random floats stay in the [0, 1) range.
for ft in ALL_FLOATS
    for i in 1:N_ITERATIONS
        f::ft = rand(prng, ft)
        @bp_check((f >= 0) && (f < 1),
                  "rand(prng, $ft) is outside [0, 1): $f.",
                    " Took $i iterations to get here (out of $N_ITERATIONS)")
        # Check determinism too.
        f2::ft = rand(prng2, ft)
        @bp_check(f == f2, f, " == ", f2)
    end
end

# Test other uses of rand() that weren't explicitly implemented.
test_rand_mode(data::T) where {T} = @bp_test_no_allocations(
    rand(prng, data),
    rand(prng2, data),
    "rand(", typeof(T), ": ", data, ")"
)
for it in setdiff(ALL_INTEGERS, [ Int128, UInt128 ]) # 128-bit ints have some kind of different interface
    range = it(2) : it(101)

    # Test that it's non-allocating.
    test_rand_mode(range)

    # Test that it's correct.
    for i in 1:N_ITERATIONS
        i1::it = rand(prng, range)
        i2::it = rand(prng2, range)
        @bp_check(i1 == i2, i1, " from ", prng, " vs ", i2, " from ", prng2)
        @bp_check((i1 >= first(range)) && (i1 <= last(range)),
                  "Value ", i1, " outside of range ", range)
    end
end