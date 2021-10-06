# Test that vector constructors work as expected.
@bp_test_no_allocations(Vec(1, 2, 3) isa Vec{3, Int}, true)
@bp_test_no_allocations(Vec(1, 2.0, UInt8(3)) isa Vec{3, Float64}, true)
@bp_test_no_allocations(Vec{Float32}(1, -2, 3) isa Vec{3, Float32}, true)

# Test our ability to construct aliases.
@bp_test_no_allocations(typeof(Vec3{Int8}(1, 2, 3)),
                        Vec{3, Int8})
@bp_test_no_allocations(typeof(v3u(3, 4, 5)),
                        Vec{3, UInt32})
@bp_test_no_allocations(typeof(Vec(UInt8(2), UInt8(5), UInt8(10))),
                        Vec{3, UInt8})

# Test properties.
@bp_test_no_allocations(Vec(2, 3, 4).x, 2)
@bp_test_no_allocations(Vec(2, 3, 4).y, 3)
@bp_test_no_allocations(Vec(2, 3, 4).z, 4)
@bp_test_no_allocations(Vec(2, 3, 4, 5).w, 5)
@bp_test_no_allocations(Vec(2, 3, 4).r, 2)
@bp_test_no_allocations(Vec(2, 3, 4).g, 3)
@bp_test_no_allocations(Vec(2, 3, 4).b, 4)
@bp_test_no_allocations(Vec(2, 3, 4, 5).a, 5)
@bp_test_no_allocations(Vec(2, 3, 4, 5).data, (2, 3, 4, 5))

# Test equality.
@bp_test_no_allocations(Vec(1, 2, 3), Vec(1, 2, 3), true)
@bp_test_no_allocations(Vec(1, 2, 3), Vec(1, 2.0, 3), true)

# Test arithmetic.
@bp_test_no_allocations(Vec(1, 2, 3, 4, 5) + 2, Vec(3, 4, 5, 6, 7))
@bp_test_no_allocations(Vec(1, 2, 3, 4, 5) - 2, Vec(-1, 0, 1, 2, 3))
@bp_test_no_allocations(Vec(1, 2, 3, 4, 5) * 1, Vec(1, 2, 3, 4, 5))
@bp_test_no_allocations(Vec(1, 2, 3, 4, 5) * 2, Vec(2, 4, 6, 8, 10))
@bp_test_no_allocations(isapprox(Vec(1, 2, 3, 4, 5) / 2,
                                 Vec(0.5, 1, 1.5, 2, 2.5)),
                        true)
@bp_test_no_allocations(2 + Vec(1, 2, 3, 4, 5), Vec(3, 4, 5, 6, 7))
@bp_test_no_allocations(2 - Vec(1, 2, 3, 4, 5), Vec(1, 0, -1, -2, -3))
@bp_test_no_allocations(2 * Vec(1, 2, 3, 4, 5), Vec(2, 4, 6, 8, 10))
@bp_test_no_allocations(isapprox(2 / Vec(1, 2, 3, 4, 5),
                                 Vec(2.0, 1.0, 0.6666666666666, 0.5, 0.4)),
                        true)
@bp_test_no_allocations(Vec(1, 2, 3, 4, 5) + Vec(7, 5, 3, 6, 4), Vec(8, 7, 6, 10, 9))
@bp_test_no_allocations(Vec(1, 2, 3, 4, 5) - Vec(7, 5, 3, 6, 4), Vec(-6, -3, 0, -2, 1))
@bp_test_no_allocations(Vec(1, 2, 3, 4, 5) * Vec(2, 3, 4, 5, 6), Vec(2, 6, 12, 20, 30))
@bp_test_no_allocations(isapprox(Vec(1, 2, 3, 4, 5) / Vec(2, 4, 5, 12, 4),
                                 Vec(0.5, 0.5, 0.6, 0.3333333333333333333333333, 1.25)),
                        true)
@bp_test_no_allocations(-Vec(2, -3, 4, -5, 6, -7), Vec(-2, 3, -4, 5, -6, 7))

# Test the dot product.
@bp_test_no_allocations(vdot(Vec(1, 2), Vec(4, 5)),  14)
@bp_test_no_allocations(Vec(1, 2) ⋅ Vec(4, 5), vdot(Vec(1, 2), Vec(4, 5)))

# Test swizzling.
@bp_test_no_allocations(Vec(1, 2, 3, 4).xyz, Vec(1, 2, 3))
@bp_test_no_allocations(Vec(1, 2, 3, 4).xyz0, Vec(1, 2, 3, 0))
@bp_test_no_allocations(Vec(1, 2, 3, 4).xyz1, Vec(1, 2, 3, 1))
@bp_test_no_allocations(Vec(1, 2, 3, 4).xyz⋀, Vec(1, 2, 3, typemax(Int)))
@bp_test_no_allocations(Vec(1, 2, 3, 4).xyz⋁, Vec(1, 2, 3, typemin(Int)))
@bp_test_no_allocations(v2f(1, 2).xxx⋀, Vec(1, 1, 1, typemax_finite(Float32)))
@bp_test_no_allocations(v2f(3, 4).xxx⋁, Vec(3, 3, 3, typemin_finite(Float32)))
@bp_check(Vec(1, 2, 3, 4).xxyyz0101w === Vec(1, 1, 2, 2, 3, 0, 1, 0, 1, 4))

# Test array-like behavior.
@bp_test_no_allocations(map(f->f*f, Vec(1, 2, 3, 4)) === Vec(1, 4, 9, 16),
                        true)
@bp_test_no_allocations(Vec(3, 4, 5)[3], 5)
@bp_test_no_allocations(Vec(3, 4, 5, 7, 3, 1)[end], 1)
@bp_test_no_allocations(sum(Vec(3, 4, 5)), 3+4+5)
@bp_test_no_allocations(reduce(-, Vec(2, 4, 6)), 2-4-6)
@bp_test_no_allocations(foldl(-, Vec(1, 2, 3)), 1-2-3)
@bp_test_no_allocations(foldr(-, Vec(6, 7, 8)), 8-7-6)

# Test vdist_sqr and vdist:
@bp_test_no_allocations(vdist_sqr(Vec(1, 1, 1), Vec(3, 1, 1)), 4)
@bp_test_no_allocations(vdist_sqr(Vec(1, 1, 1), Vec(1, 3, 1)), 4)
@bp_test_no_allocations(vdist_sqr(Vec(1, 1, 1), Vec(1, 1, 3)), 4)
@bp_test_no_allocations(vdist_sqr(Vec(1, 1, 1), Vec(3, 3, 3)), 12)
@bp_test_no_allocations(isapprox(vdist(Vec(1, 1, 1), Vec(2, 0, 2)),
                                 sqrt(3.0)),
                        true)

# Test vlength_sqr and vlength:
@bp_test_no_allocations(vlength_sqr(Vec(5, 0, 0)), 25)
@bp_test_no_allocations(vlength_sqr(Vec(0, 5, 0)), 25)
@bp_test_no_allocations(vlength_sqr(Vec(0, 0, 5)), 25)
@bp_test_no_allocations(vlength_sqr(Vec(5, 5, 5)), 75)
@bp_test_no_allocations(isapprox(vlength(Vec(1, -1, -1)),
                                 sqrt(3.0)),
                        true)

# Test vnorm and is_normalized:
@bp_test_no_allocations(isapprox(vnorm(Vec(1, -1)),
                                 1/Vec(sqrt(2.0), -sqrt(2.0))),
                        true)
@bp_test_no_allocations(isapprox(vnorm(Vec(1, -1, 1)),
                                 1/Vec(sqrt(3.0), -sqrt(3.0), sqrt(3.0))),
                        true)
@bp_test_no_allocations(is_normalized(1/Vec(sqrt(2.0), -sqrt(2.0))),
                        true)
@bp_test_no_allocations(is_normalized(1/Vec(sqrt(2.1), -sqrt(2.0))),
                        false)

# Test vcross:
@bp_test_no_allocations(vcross(Vec(1, 0, 0), Vec(0, 1, 0)),
                        Vec(0, 0, 1))
@bp_test_no_allocations(Vec(1, 0, 0) × Vec(0, 1, 0),
                        Vec(0, 0, 1))
@bp_test_no_allocations(Vec(0, 0, 1) × Vec(0, 1, 0),
                        Vec(-1, 0, 0))