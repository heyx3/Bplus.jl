#=
@std140 struct A
    f::Float32
    b::Bool
    v::v3f
end

# @std140 struct B
#     a::A
#     m::fmat4
#     i::Int32
#     d::dmat2x3
#     bs::NTuple{6, Bool}
#     backup_as::NTuple{10, A}
# end

# Test struct A:
@bp_check(sizeof(A) == 32,
          "Struct 'A' in std140 layout should be 32 bytes but it's ", sizeof(A))
@bp_check(propertynames(A) == (:f, :b, :v))
@bp_check(getproperty.(Ref(A(1.4f0, true, v3f(4.4, 5.5, 6.6))),
                       propertynames(A)) ==
           (1.4f0, true, v3f(4.4, 5.5, 6.6)))
@bp_check(fieldoffset(A, Base.fieldindex(A, :f)) == 0,
          "Expected 0, got ", fieldoffset(A, Base.fieldindex(A, :f)))
@bp_check(fieldoffset(A, Base.fieldindex(A, :b)) == 4,
          "Expected 4, got ", fieldoffset(A, Base.fieldindex(A, :b)))
@bp_check(fieldoffset(A, Base.fieldindex(A, :v)) == 16,
          "Expected 16, got ", fieldoffset(A, Base.fieldindex(A, :v)))
@bp_check(sprint(show, A(1.4f0, true, v3f(4.4, 5.5, 6.6))) ==
            "A($(sprint(show, 1.4f0)), true, $(sprint(show, v3f(4.4, 5.5, 6.6))))",
          "Actual value: ", sprint(show, A(1.4f0, true, v3f(4.4, 5.5, 6.6))), "\n")

# Test struct B:
# @bp_check(sizeof(B) == 608,
#           "Struct 'B' in std140 layout should be 608 bytes but is ", sizeof(B))
# @bp_check(propertynames(B) == (:a, :m, :i, :d, :bs, :backup_as),
#           propertynames(B))
# rand(rng, ::Type{A}) = A(rand(rng, Float32), rand(rng, Bool), rand(rng, v3f))
# Random.seed!(0x57483829)
# let in_data = (
#                    rand(A),
#                    fmat4(4.1, 4.2, 4.3, 4.4,
#                          4.5, 4.6, 4.7, 4.8,
#                          4.9, 4.01, 4.02, 4.03,
#                          4.04, 4.05, 4.06, 4.07),
#                    Int32(-666),
#                    dmat2x3(7.7, 7.8, 7.9,
#                            8.7, 8.8, 8.9),
#                    ntuple(i -> rand(Bool), 6)
#                    ntuple(i -> rand(A), 10)
#               )
#     @bp_check(getproperty.(Ref(B(in_data...)), propertynames(B)) == in_data,
#                 getproperty.(Ref(B(in_data...)), propertynames(B)))
# end
# @bp_check(fieldoffset(B, Base.fieldindex(B, :a) == 0))
# @bp_check(fieldoffset(B, Base.fieldindex(B, :i) == 96))
# @bp_check(fieldoffset(B, Base.fieldindex(B, :bs) == 192))
# @bp_check(fieldoffset(B, Base.fieldindex(B, :backup_as) == 289))


#TODO: Test more arrays, including arrays of matrices
#TODO: Test VecT{Bool}

#NOTES:
#=

struct A <: OglBlock_std140
    f::Float32 # 1-4

    b::Bool # 5-5
    var"CONST: 1"::NTuple{3, UInt8} # 6-8

    var"PAD: 1"::NTuple{2*4, UInt8} # 9-16
    j::v3f # 17-28
    var"PAD: 2"::NTuple{1*4, UInt8} # 29-32

    A(f::Float32, b::Bool, j::v3f) = new(
        f,

        b,
        ntuple(i->zero(UInt8), Val(3)),

        ntuple(i->0x01, Val(2*4)),
        j,
        ntuple(i->0x02, Val(1*4))
    )
end
# Hide padding in properties:
@inline propertynames(::Type{A}) = (:f, :b, :j)
@inline getproperty(a::A, name::Symbol) = begin
    (name == :f) && return getfield(a, :f)
    (name == :b) && return getfield(a, :b)
    (name == :j) && return getfield(a, :j)
    (name in fieldnames(A)) && error("Do not try to access internal fields of the std140 struct!")
    error("Unknown property: ", name)
end
# AbstractOglBlock interface:
padding_size(::Type{A}) = 12
base_alignment(::Type{A}) = 16 # Alignment of v3f
# Test the struct's layout.
@bp_check(sizeof(A) == 32, "Struct 'A' in std140 layout should be 32 bytes but it's ", sizeof(A))
@bp_check(getproperty.(Ref(A(1.4f0, true, v3f(4.4, 5.5, 6.6))),
                       (:f, :b, :j)) ==
           (1.4f0, true, v3f(4.4, 5.5, 6.6)))
@bp_check(fieldoffset(A, Base.fieldindex(A, :f)) == 0)
@bp_check(fieldoffset(A, Base.fieldindex(A, :b)) == 4)
@bp_check(fieldoffset(A, Base.fieldindex(A, :j)) == 16)
@bp_check(sprint(show, A(1.4f0, true, v3f(4.4, 5.5, 6.6))) ==
            "A($(sprint(show, 1.4f0)), true, $(sprint(show, v3f(4.4, 5.5, 6.6))))")

struct B <: OglBlock_std140
    a::A # 1-32

    var"m: column: 1"::v4f # 33-48
    var"m: column: 2"::v4f # 49-64
    var"m: column: 3"::v4f # 65-80
    var"m: column: 4"::v4f # 81-96

    i::Int32 # 97-100

    var"PAD: 1"      ::NTuple{28, UInt8} # 101-128
    var"d: column: 1"::Vec3{Float64} # 129-152
    var"PAD: 2"      ::NTuple{8, UInt8} # 153-160
    var"d: column: 2"::Vec3{Float64} # 161-184
    var"PAD: 3"      ::NTuple{8, UInt8} # 185-192

    bs::NTuple{6, Bool} # 193-276

    var"PAD: 4"::NTuple{12, UInt8} # 277-288
    backup_as::NTuple{10, A} # 289-608

    B(a, m, i, d, bs, backup_as) = new(
        convert(A, a),
        v4f(m[:, 1]...), v4f(m[:, 2]...), v4f(m[:, 3]...), v4f(m[:, 4]...),
        convert(Int32, i),
        ntuple(i->0x01, Val(28)),
        Vec3{Float64}(m[:, 1]...), ntuple(i->0x02, Val(8)),
        Vec3{Float64}(m[:, 2]...), ntuple(i->0x03, Val(9)),
        (bs isa NTuple{6, Bool}) ?
            bs :
            ntuple(i->convert(Bool, bs[i]), Val(6)),
        ntuple(i->0x04, Val(12)),
        (backup_as isa NTuple{10, A}) ?
            backup_as :
            ntuple(i->convert(A, backup_as[i]), Val(10))
    )
end
# Hide padding in properties:
@inline propertynames(::Type{B}) = (:a, :m, :i, :d, :bs, :backup_as)
@inline getproperty(b::B, name::Symbol) = begin
    (name == :a) && return getfield(b, :a)
    (name == :m) && return fmat4(getfield(b, Symbol("m: column: 1")),
                                 getfield(b, Symbol("m: column: 2")),
                                 getfield(b, Symbol("m: column: 3")),
                                 getfield(b, Symbol("m: column: 4")))
    (name == :i) && return getfield(b, :i)
    (name == :d) && return dmat2x3(getfield(b, Symbol("d: column: 1")),
                                   getfield(b, Symbol("d: column: 2")))
    (name == :bs) && return getfield(b, :bs)
    (name == :backup_as) && return getfield(b, :backup_as)
    (name in fieldnames(B)) && error("Do not try to access internal fields of the std140 struct!")
    error("Unknown property name '", name, "'")
end
# AbstractOglBlock interface:
base_alignment(::Type{B}) = 32 # alignment of dvec3
padding_size(::Type{B}) = 28 + 8 + 8 + 12

=#

=#