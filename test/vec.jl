# Test that vector constructors work as expected.
@bp_test_no_allocations(Vec(1, 2, 3) isa Vec{3, Int}, true)
@bp_test_no_allocations(Vec(1, 2.0, UInt8(3)) isa Vec{3, Float64}, true)
@bp_test_no_allocations(Vec{Float32}(1, -2, 3) isa Vec{3, Float32}, true)

# Test equality.
@bp_test_no_allocations(Vec(1, 2, 3), Vec(1, 2, 3), true)
@bp_test_no_allocations(Vec(1, 2, 3), Vec(1, 2.0, 3), true)

# Test the dot product.
@bp_test_no_allocations(vdot(Vec(1, 2), Vec(4, 5)),  14)
@bp_test_no_allocations(Vec(1, 2) ⋅ Vec(4, 5), vdot(Vec(1, 2), Vec(4, 5)))

# Test swizzling.
@bp_test_no_allocations(Vec(1, 2, 3, 4, 5).x, 1)
@bp_test_no_allocations(Vec(1, 2, 3, 4, 5).y, 2)
@bp_test_no_allocations(Vec(1, 2, 3, 4, 5).z, 3)
@bp_test_no_allocations(Vec(1, 2, 3, 4, 5).w, 4)
@bp_test_no_allocations(Vec(1, 2, 3, 4).xyz, Vec(1, 2, 3))
@bp_test_no_allocations(Vec(1, 2, 3, 4).xyz0, Vec(1, 2, 3, 0))
@bp_test_no_allocations(Vec(1, 2, 3, 4).xyz1, Vec(1, 2, 3, 1))
@bp_check(Vec(1, 2, 3, 4).xxyyz0101w === Vec(1, 1, 2, 2, 3, 0, 1, 0, 1, 4))
@bp_test_no_allocations(Vec(1, 2, 3, 4, 5).r, 1)
@bp_test_no_allocations(Vec(1, 2, 3, 4, 5).g, 2)
@bp_test_no_allocations(Vec(1, 2, 3, 4, 5).b, 3)
@bp_test_no_allocations(Vec(1, 2, 3, 4, 5).a, 4)