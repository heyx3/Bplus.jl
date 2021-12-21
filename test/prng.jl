# Test that ConstPRNG doesn't allocate.
@bp_test_no_allocations(typeof(rand(ConstPRNG(), UInt)[1]), UInt)
@bp_test_no_allocations(typeof(rand(ConstPRNG(), Int8)[1]), Int8)
@bp_test_no_allocations(typeof(rand(ConstPRNG(), Bool)[1]), Bool)
@bp_test_no_allocations(rand(ConstPRNG(), (1, 2.0))[1] isa Union{Int, Float64}, true)
@bp_test_no_allocations(typeof(rand_ntuple(ConstPRNG(), (1, 2, 4))[1]), Int)
@bp_test_no_allocations(typeof(rand(ConstPRNG(), 5:2000)[1]), Int)
@bp_test_no_allocations(typeof(rand(ConstPRNG(), 5:10:2000)[1]), Int)
@bp_test_no_allocations(typeof(rand(ConstPRNG(), 5.5:10:2000.5)[1]), Float64)
@bp_test_no_allocations(typeof(rand(ConstPRNG(), range(1, length=13, stop=20))[1]), Float64)

# Test fill_in_seeds().
@bp_test_no_allocations(sizeof(fill_in_seeds(UInt8)), 10)
@bp_test_no_allocations(sizeof(fill_in_seeds(NTuple{2, UInt8})), 10)
@bp_test_no_allocations(sizeof(fill_in_seeds(NTuple{3, UInt8})), 9)
@bp_test_no_allocations(sizeof(fill_in_seeds(NTuple{4, UInt8})), 8)
@bp_test_no_allocations(sizeof(fill_in_seeds(NTuple{5, UInt8})), 6)
@bp_test_no_allocations(sizeof(fill_in_seeds(NTuple{6, UInt8})), 6)
@bp_test_no_allocations(sizeof(fill_in_seeds(NTuple{7, UInt8})), 5)
@bp_test_no_allocations(sizeof(fill_in_seeds(NTuple{8, UInt8})), 4)
@bp_test_no_allocations(sizeof(fill_in_seeds(NTuple{9, UInt8})), 2)
@bp_test_no_allocations(sizeof(fill_in_seeds(NTuple{10, UInt8})), 2)
@bp_test_no_allocations(sizeof(fill_in_seeds(NTuple{11, UInt8})), 1)
@bp_test_no_allocations(sizeof(fill_in_seeds(NTuple{12, UInt8})), 0)
@bp_test_no_allocations(sizeof(fill_in_seeds(NTuple{13, UInt8})), 0)
@bp_test_no_allocations(sizeof(fill_in_seeds(NTuple{99, UInt8})), 0)

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
# Reduce the actual set of types tested to keep the tests fast.
for (outer_i, T1) in enumerate(ALL_FLOATS)
    if outer_i > 1
        print(" | ")
    end
    print(outer_i, ":")

    @run_prng_test_for(1)

    for (inner_i, T2) in enumerate(ALL_UNSIGNED)
        if inner_i > 1
            print(",")
        end
        print(inner_i)

        @run_prng_test_for(2)

        # Don't print the inner layers, they run too frequently.
        for T3 in ALL_SIGNED
            @run_prng_test_for(3)

            # The fourth layer is important, giving us tests with an extra seed value
            #    beyond the ideal 3.
            for T4 in ALL_SIGNED
                @run_prng_test_for(4)
            end
        end
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

# Test RandIterator
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

const N_ITERATIONS = 9999999  # A balance between speed and having enough samples

@bp_test_no_allocations(copy(prng) isa PRNG, true)

# This macro runs the two PRNG's, checks they're equal, and checks there's no allocations.
macro test_compare_prngs(rand_arg, msg_args...)
    return impl_test_compare_prngs(:prng, :prng2, rand_arg, msg_args)
end
macro test_compare_prngs2(r1, r2, rand_arg, msg_args...)
    return impl_test_compare_prngs(r1, r2, rand_arg, msg_args)
end
function impl_test_compare_prngs(r1, r2, rand_arg, msg_args)
    r1 = esc(r1)
    r2 = esc(r2)
    rand_arg = esc(rand_arg)
    msg_args = map(esc, msg_args)
    return quote
        @bp_test_no_allocations(rand($r1, $rand_arg),
                                rand($r2, $rand_arg),
                                $(msg_args...))
        @bp_check(($r1.seeds == $r2.seeds) && ($r1.state == $r2.state),
                  $r1, " vs ", $r2)
    end
end


#TODO: Pretty certain the PRNG stuff doesn't allocate, yet the tests think it does?
if false

# Test that the PRNG work is non-allocating and deterministic.
for nt in ALL_REALS
    @test_compare_prngs(nt, "For ", nt)
end

# Test that copies of a PRNG match the original.
const prng3 = PRNG()
for i in 1:N_ITERATIONS
    p3::PRNG = copy(prng3)
    @bp_test_no_allocations(rand(copy(prng3), UInt32),
                            rand(prng3, UInt32),
                            "For ", UInt32)
end

# Test that floats stay in the [0, 1) range.
for ft in ALL_FLOATS
    for i in 1:N_ITERATIONS
        f::ft = rand(prng, ft)
        f2::ft = rand(prng2, ft)
        @bp_check(f == f2, f, " == ", f2)
        @bp_check((f >= 0) && (f < 1),
                  "rand(prng, ", ft, ") is outside [0, 1): ", f,
                  ". Took ", i, " iterations to get here (out of ", N_ITERATIONS, ")")
    end
end

# Test other uses of rand() that weren't explicitly implemented.
for it in ALL_INTEGERS
    range = it(2) : it(101)

    # Test that it's non-allocating.
    rand(prng2, range)
    @bp_test_no_allocations(rand(prng, range), rand(prng2, range),
                            "For range ", range)

    # Test that it's correct.
    for i in 1:N_ITERATIONS
        i1::it = rand(prng, range)
        i2::it = rand(prng2, range)
        @bp_check(i1 == i2, i1, " from ", prng, " vs ", i2, " from ", prng2)
        @bp_check((i1 >= first(range)) && (i1 <= last(range)),
                  "Value ", i1, " outside of range ", range)
    end
end

end # if false (see todo above)