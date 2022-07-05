const rng = PRNG()

# Test that perlin() doesn't allocate, and generates values in a valid range.
for F in ALL_FLOATS
    @eval begin
        const N_ITERS = 99999
        for i in 1:N_ITERS
            @bp_test_no_allocations(typeof(perlin(rand(rng, $F))), $F)

            f::$F = perlin(rand(rng, $F))
            @bp_check(f >= 0,
                    "Perlin value < 0 at iteration $i of $N_ITERS: $($F) $f")
            @bp_check(f <= 1,
                    "Perlin value > 1 at iteration $i of $N_ITERS: $($F) $f")
        end
    end
end