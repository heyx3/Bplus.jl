@std430 struct A
    f::Float32
    b::Bool
    v::v4f
end

# For debugging, make a very clear print() for A.
function Base.print(io::IO, a::A; indentation="")
    println(io, "A(")
        println(io, indentation, "\tf = ", a.f)
        println(io, indentation, "\tb = ", a.b)
        println(io, indentation, "\tv = ", a.v)
    print(io, indentation, ")")
end

# Test struct A:
@bp_test_no_allocations(sizeof(A),
                        32,
                        "Struct 'A' in std430 layout should be 32 bytes but it's ",
                          sizeof(A))
@bp_test_no_allocations(propertynames(A), (:f, :b, :v))
@bp_test_no_allocations(typeof(A(1.4f0, true, v4f(4.4, 5.5, 6.6, 7.7))),
                        A)
@bp_test_no_allocations(getproperty.(Ref(A(1.4f0, true, v4f(4.4, 5.5, 6.6, 7.7))),
                                     propertynames(A)),
                        (1.4f0, true, v4f(4.4, 5.5, 6.6, 7.7)))
function check_A_field(name::Symbol, type::Type, offset::Int)
    @bp_test_no_allocations(Bplus.GL.property_type(name), type)
    @bp_test_no_allocations(Bplus.GL.property_offset(name), offset)
end
@bp_test_no_allocations(Bplus.GL.property_offset(A, :f), 0,
                        "Expected 0, got ", Bplus.GL.property_offset(A, :f))
@bp_test_no_allocations(Bplus.GL.property_type(A, :f), Float32)
@bp_test_no_allocations(Bplus.GL.property_offset(A, :b), 4,
                        "Expected 4, got ", Bplus.GL.property_offset(A, :b))
@bp_test_no_allocations(Bplus.GL.property_type(A, :b), Bool)
@bp_test_no_allocations(Bplus.GL.property_offset(A, :v), 16,
                        "Expected 16, got ", Bplus.GL.property_offset(A, :v))
@bp_test_no_allocations(Bplus.GL.property_type(A, :v), v4f)
let sprinted = sprint(show, A(1.4f0, true, v4f(4.4, 5.5, 6.6, 7.7)))
    @bp_check(sprinted == "A($(sprint(show, 1.4f0)), true, $(sprint(show, v4f(4.4, 5.5, 6.6, 7.7))))",
              "Actual value: ", sprint(show, A(1.4f0, true, v4f(4.4, 5.5, 6.6, 7.7))), "\n")
end
@bp_test_no_allocations(Bplus.GL.base_alignment(A), 16)


@std430 struct B
    a::A # 32B
    m::fmat4 # 64B = 96
    i::Int32 # 4B = 100
    # Pad 12B to align with dvec2 (16B): 112
    d::dmat3x2 # 48B = 160
    bs::NTuple{6, Bool} # 24B = 184
    # Pad 8B to align with 16B: 192
    backup_as::NTuple{10, A} # 320B = 512
end

# For debugging, make a very clear print() for B.
function Base.print(io::IO, b::B; indentation="")
    println(io, "B(")
        print(io, indentation, "\t", "a = ")
            print(io, b.a; indentation=indentation*"\t")
            println(io)
        println(io, indentation, "\t", "m = ", b.m)
        println(io, indentation, "\t", "i = ", b.i)
        println(io, indentation, "\t", "d = ", b.d)
        println(io, indentation, "\t", "bs = [")
        for bool in b.bs
            println(io, indentation, "\t\t", bool)
        end
        println(io, indentation, "]")
        println(io, indentation, "\t", "backup_as = [")
        for a in b.backup_as
            print(io, indentation, "\t\t")
            print(io, a; indentation=indentation * "\t\t")
            println(io)
        end
        println(io, indentation, "]")
    print(io, indentation, ")")
end

# Test struct B:
@bp_test_no_allocations(sizeof(B), 512)
@bp_test_no_allocations(propertynames(B), (:a, :m, :i, :d, :bs, :backup_as),
                        propertynames(B))
Random.rand(::Type{A})::A = A(rand(Float32), rand(Bool), rand(v4f))
Random.seed!(0x57483829)
let in_data = (
                   rand(A),
                   fmat4(4.1, 4.2, 4.3, 4.4,
                         4.5, 4.6, 4.7, 4.8,
                         4.9, 4.01, 4.02, 4.03,
                         4.04, 4.05, 4.06, 4.07),
                   Int32(-666),
                   dmat3x2(7.7, 7.8, 7.9,
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
function check_B_field(name::Symbol, type::Type, offset::Int)
    @bp_test_no_allocations(Bplus.GL.property_type(B, name), type,
                            "Field B.", name)
    @bp_test_no_allocations(Bplus.GL.property_offset(B, name), offset,
                            "Field B.", name)
end
check_B_field(:m, fmat4, 32)
check_B_field(:d, dmat3x2, 112)
check_B_field(:bs, NTuple{6, Bool}, 160)
check_B_field(:backup_as, NTuple{10, A}, 192)
@bp_test_no_allocations(Bplus.GL.base_alignment(B), 16)


@std430 struct C
    bool_vec::Vec{2, Bool} # 8
    # Pad 8B to align with struct B (16B): 16
    b::B # 512B = 528
    f1::Float32 # 532
    # Pad 4B to align with v2u: 536
    array1::NTuple{10, Vec{2, Bool}} # alignment is v2u
                                     # 80B = 616
    array2::NTuple{5, fmat3x2} # alignment is v2f (8B)
                               # element stride is v2f*3 = 24
                               # 120B = 736
    # Total size is 736
end

# For debugging, make a very clear print() for C.
function Base.print(io::IO, c::C; indentation="")
    println(io, "C(")
        println(io, indentation, "\t", "bool_vec = ", c.bool_vec)
        print(io, indentation, "\t", "b = ")
            print(io, c.b; indentation=indentation*"\t")
            println(io)
        println(io, indentation, "\t", "f1 = ", c.f1)
        println(io, indentation, "\t", "array1 = [")
        for v in c.array1
            println(io, indentation, "\t\t", v)
        end
        println(io, indentation, "]")
        println(io, indentation, "\t", "array2 = [")
        for m in c.array2
            println(io, indentation, "\t\t", m)
        end
        println(io, indentation, "]")
    print(io, indentation, ")")
end

# Test struct C:
@bp_test_no_allocations(sizeof(C), 736)
@bp_test_no_allocations(propertynames(C), (:bool_vec, :b, :f1, :array1, :array2))
Random.rand(::Type{B})::B = B(
    rand(A),
    rand(fmat4),
    rand(Int32),
    rand(dmat3x2),
    ntuple(i -> rand(Bool), 6),
    ntuple(i -> rand(A), 10)
)
Random.seed!(0xbafacada)
let in_data = (
                  v2b(false, true),
                  rand(B),
                  -1.2f0,
                  ntuple(i -> Vec(rand(Bool), rand(Bool)),
                         Val(10)),
                  ntuple(i -> rand(fmat3x2), Val(5))
              )
    make_c = () -> C(in_data...)
    @bp_test_no_allocations(make_c(), make_c(),
                            "Constructor should create an equal value without allocation")
    let c = Ref(make_c())
        get_props = () -> getproperty.(c, propertynames(C))
        for (prop, expected) in zip(propertynames(C), in_data)
            @bp_check(getproperty(c[], prop) == expected,
                      "Property ", prop, " should be:\n\t",
                        expected, "\n",
                      " but was:\n\t", getproperty(c[], prop))
        end
        @bp_test_no_allocations(typeof.(get_props()), typeof.(in_data),
                                "Property types")
        @bp_test_no_allocations(get_props(), in_data,
                                "Property values")
    end
end
@inline function check_C_field(name::Symbol, type::Type, offset::Int)
    @bp_test_no_allocations(Bplus.GL.property_type(C, name), type,
                            "Property C.", name)
    @bp_test_no_allocations(Bplus.GL.property_offset(C, name), offset,
                            "Property C.", name)
end
check_C_field(:bool_vec, Vec{2, Bool}, 0)
check_C_field(:b, B, 16)
check_C_field(:f1, Float32, 528)
check_C_field(:array1, NTuple{10, Vec{2, Bool}}, 536)
check_C_field(:array2, NTuple{5, fmat3x2}, 616)
@bp_test_no_allocations(Bplus.GL.base_alignment(C), 16)