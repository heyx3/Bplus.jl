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