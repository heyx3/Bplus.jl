# Test constructors.
@bp_test_no_allocations(Quaternion(1, 2, 3, 4) isa Quaternion{Int}, true)
@bp_test_no_allocations(Quaternion(1, 2, 3.0, 4) isa dquat, true)
@bp_test_no_allocations(Quaternion(UInt8(1), Int16(-2), 3.0f0, 4) isa fquat, true)

# Test equality.
@bp_test_no_allocations(Quaternion(1, 2, 3, 0), Quaternion(1, 2, 3, 0))
@bp_test_no_allocations(Quaternion(1, 2, 3, 0), Quaternion(1, 2.0, 3, UInt8(0)))
@bp_test_no_allocations(isapprox(Quaternion(1, 2, 3, 4),
                                 Quaternion(1, 2, 3, 4)),
                        true)
@bp_test_no_allocations(isapprox(Quaternion(1, 2, 3, 4),
                                 Quaternion(1, 0, 3, 4)),
                        false)
@bp_test_no_allocations(isapprox(Quaternion(1, 2, 3, 4),
                                 Quaternion(1, 2, 3, 0)),
                        false)
@bp_test_no_allocations(isapprox(Quaternion(1, 2, 3, 4),
                                 Quaternion(0, 2, 3, 4)),
                        false)
@bp_test_no_allocations(isapprox(Quaternion(1.0, 2, 3, 4),
                                 Quaternion(ntuple(i->nextfloat(Float64(i)), 4))),
                        true)
@bp_test_no_allocations(isapprox(Quaternion(1.0, 2.0, 3.0, 4.0),
                                 Quaternion(1.4, 2.4, 2.6, 4.4)),
                        false)
@bp_test_no_allocations(isapprox(Quaternion(1.0, 2.0, 3.0, 4.0),
                                 Quaternion(1.4, 2.4, 2.6, 4.15),
                                 0.5),
                        true)
@bp_test_no_allocations(isapprox(Quaternion(1.0, 2.0, 3.0, 4.0),
                                 Quaternion(1.0, 2.0, 3.0, 4.4),
                                 0.15),
                        false)

# Test arithmetic.
@bp_test_no_allocations(-Quaternion(1, 2, 3, 4), Quaternion(-1, -2, -3, 4))

# Test combination of multiple rotations.
@bp_test_no_allocations(Quaternion(fquat(), fquat(), fquat()),
                        fquat())
#TODO: Finish

# Test Quaternion ops.
#TODO: Implement

# Test conversions.
#TODO: Implement