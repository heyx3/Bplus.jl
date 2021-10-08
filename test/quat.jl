# Test constructors.
@bp_test_no_allocations(Quaternion(1, 2, 3, 4) isa Quaternion{Int}, true)
@bp_test_no_allocations(Quaternion(1, 2, 3.0, 4) isa dquat, true)
@bp_test_no_allocations(Quaternion(UInt8(1), Int16(-2), 3.0f0, 4) isa fquat, true)
@bp_test_no_allocations(Quaternion(v3f(0, 1, 0), π) isa Quaternion{Float32}, true)
@bp_test_no_allocations(dquat() isa Quaternion{Float64}, true)
@bp_test_no_allocations(fquat(Vec(1, 0, 0), vnorm(Vec(0, 1, 1))) isa fquat, true)

# Test equality.
@bp_test_no_allocations(Quaternion(1, 2, 3, 0), Quaternion(1, 2, 3, 0))
@bp_test_no_allocations(Quaternion{Int}(), Quaternion(0, 0, 0, 1))
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
@bp_test_no_allocations(-Quaternion(1, 0, 0, 0), Quaternion(-1, 0, 0, 0))

# Test combination of multiple rotations.
@bp_test_no_allocations(Quaternion(fquat()),
                        fquat())
@bp_test_no_allocations(Quaternion(fquat(), fquat()),
                        fquat())
@bp_test_no_allocations(Quaternion(fquat(), fquat(), fquat()),
                        fquat())

# Test Quaternion ops.
@bp_test_no_allocations(isapprox(q_apply(fquat(Vec(0, 1, 0), π/2),
                                         Vec(1, 1, 1)),
                                 Vec(1, 1, -1),
                                 0.00001),
                        true)

# Test conversions.
#TODO: Implement