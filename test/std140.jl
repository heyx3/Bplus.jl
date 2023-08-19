@std140 struct A
    f::Float32
    b::Bool
    v::v4f
end

# Test struct A:
@bp_test_no_allocations(sizeof(A),
                        32,
                        "Struct 'A' in std140 layout should be 32 bytes but it's ",
                          sizeof(A))
@bp_test_no_allocations(propertynames(A), (:f, :b, :v))
@bp_test_no_allocations(typeof(A(1.4f0, true, v4f(4.4, 5.5, 6.6, 7.7))),
                        A)
@bp_test_no_allocations(getproperty.(Ref(A(1.4f0, true, v4f(4.4, 5.5, 6.6, 7.7))),
                                     propertynames(A)),
                        (1.4f0, true, v4f(4.4, 5.5, 6.6, 7.7)))
@bp_test_no_allocations(Bplus.GL.property_offset(A, :f), 0,
                        "Expected 0, got ", Bplus.GL.property_offset(A, :f))
@bp_test_no_allocations(Bplus.GL.property_type(A, :f), Float32)
@bp_test_no_allocations(Bplus.GL.property_offset(A, :b), 4,
                        "Expected 0, got ", Bplus.GL.property_offset(A, :b))
@bp_test_no_allocations(Bplus.GL.property_type(A, :b), Bool)
@bp_test_no_allocations(Bplus.GL.property_offset(A, :v), 16,
                        "Expected 0, got ", Bplus.GL.property_offset(A, :v))
@bp_test_no_allocations(Bplus.GL.property_type(A, :v), v4f)
@bp_check(sprint(show, A(1.4f0, true, v4f(4.4, 5.5, 6.6, 7.7))) ==
            "A($(sprint(show, 1.4f0)), true, $(sprint(show, v4f(4.4, 5.5, 6.6, 7.7))))",
          "Actual value: ", sprint(show, A(1.4f0, true, v4f(4.4, 5.5, 6.6, 7.7))), "\n")


@std140 struct B
    a::A # 32B
    m::fmat4 # 64B = 96
    i::Int32 # 4B = 100
    # Pad 28B to align with dvec4 (32B): 128
    d::dmat2x3 # 48B = 176
    bs::NTuple{6, Bool} # 96B = 272
    backup_as::NTuple{10, A} # 320B = 592
    # Pad to align with dvec4(32B): 608
end

# Test struct B:
@bp_test_no_allocations(sizeof(B), 608,
                        "Struct 'B' in std140 layout should be 608 bytes but is ", sizeof(B))
@bp_test_no_allocations(propertynames(B), (:a, :m, :i, :d, :bs, :backup_as),
                        propertynames(B))
Random.rand(::Type{A}) = A(rand(Float32), rand(Bool), rand(v4f))
Random.seed!(0x57483829)
let in_data = (
                   rand(A),
                   fmat4(4.1, 4.2, 4.3, 4.4,
                         4.5, 4.6, 4.7, 4.8,
                         4.9, 4.01, 4.02, 4.03,
                         4.04, 4.05, 4.06, 4.07),
                   Int32(-666),
                   dmat2x3(7.7, 7.8, 7.9,
                           8.7, 8.8, 8.9),
                   ntuple(i -> rand(Bool), 6),
                   ntuple(i -> rand(A), 10)
              ),
    make_b = () -> B(in_data...)
    @bp_test_no_allocations(make_b(), make_b(),
                            "Constructor should create an equal value without allocation")
    let b = Ref(make_b())
        @bp_test_no_allocations(getproperty.(b, propertynames(B)),
                                in_data,
                                "Reading all properties: ",
                                    getproperty.(b, propertynames(B)))
    end
end
@bp_test_no_allocations(Bplus.GL.property_offset(B, :m), 32)
@bp_test_no_allocations(Bplus.GL.property_type(B, :m), fmat4)
@bp_test_no_allocations(Bplus.GL.property_offset(B, :d), 128)
@bp_test_no_allocations(Bplus.GL.property_type(B, :d), dmat2x3)
@bp_test_no_allocations(Bplus.GL.property_offset(B, :backup_as), 288)
@bp_test_no_allocations(Bplus.GL.property_type(B, :backup_as), NTuple{10, A})


#TODO: Test more arrays, including arrays of matrices
#TODO: Test VecT{Bool}