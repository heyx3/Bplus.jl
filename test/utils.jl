# Test that unzip2() can be inlined/folded into something that doesn't require the heap.
@bp_test_no_allocations(begin
        # To prevent compiling into a simple "return [constant]",
        #    run unzip2 in a big loop and perform some mixing of tuple data.
        outp::Tuple{NTuple{3, Int}, NTuple{3, Int}} = ((0, 0, 0), (0, 0, 0))
        for i in 1:2000
            z = zip((1, i, 3), (-i, 2, 6))
            mm = (minmax(a, b) for (a,b) in z)
            (a, b) = unzip2(mm)
            outp = (b, outp[1])
        end
        outp # Return the data so it doesn't get compiled out
    end,
    # I have no idea what this actually computed
    ((1, 2000, 6), (1, 1999, 6))
)

# Test UpTo{N, T}:
@bp_test_no_allocations(collect(UpTo{4, Int}()), ())
@bp_test_no_allocations(collect(UpTo{4, Int}((3, 2))), (3, 2))
# Conversion:
@inline function f_upto(i::T)::UpTo{3, Int} where {T}
    return i
end
@bp_test_no_allocations(f_upto(()), UpTo{3, Int}(( )))
@bp_test_no_allocations(f_upto(2), UpTo{3, Int}((2, )))
@bp_test_no_allocations(f_upto((3, 4)), UpTo{3, Int}((3, 4)))
# Equality with raw data:
@bp_test_no_allocations(UpTo{3, Int}((2, 3, 4)), (2, 3, 4))
@bp_test_no_allocations(UpTo{3, Int}((2, )), (2, ))
@bp_test_no_allocations(UpTo{3, Int}((2, )), 2)

# Test reduce_some()
@bp_test_no_allocations(reduce_some(+, b->(b%2==0), 1:10),
                        reduce(+, 2:2:10))
@bp_test_no_allocations(reduce_some(*, b->(b%2==0), 1:10; init=1),
                        reduce(*, 2:2:10))

# Test @unionspec
@bp_check(@unionspec(Vector{_}, Int, Float64) ==
            Union{Vector{Int}, Vector{Float64}},
          @unionspec(Vector{_}, Int, Float64))
@bp_check(@unionspec(Vector{_}, Tuple{Int, Float64}, String) ==
            Union{Vector{Tuple{Int, Float64}}, Vector{String}},
          @unionspec(Vector{_}, Tuple{Int, Float64}, String))
@bp_check(@unionspec(Array{_, 2}, Int, Float64) ==
            Union{Array{Int, 2}, Array{Float64, 2}},
          @unionspec(Array{_, 2}, Int, Float64))
@bp_check(@unionspec(Array{Int, _}, 4, 5) ==
            Union{Array{Int, 4}, Array{Int, 5}},
          @unionspec(Array{Int, _}, 4, 5))

# Test ConstVector:
@bp_check((4.0, 3.0, 1.0, 4.0) isa ConstVector{Float64})

# Test Contiguous:
@bp_check(v2f <: Contiguous{Float32})
@bp_check(v4f <: Contiguous{Float32})
@bp_check(!(v4f <: Contiguous{Float64}))
@bp_check([ 4, 5.0, 6, 7 ] isa Contiguous{Float64})
@bp_check(!([ 4, 5, 6, :hi ] isa Contiguous{Float64}))
@bp_check([ 4 5.0 ; 6 7 ] isa Contiguous{Float64})
@bp_check(Vec{2, v2f} <: Contiguous{Float32})
@bp_check(Vec{20, v2f} <: Contiguous{Float32})
@bp_check(Int16 <: Contiguous{Int16})
@bp_check(Tuple{Int16} <: Contiguous{Int16})
@bp_check(NTuple{400, Int16} <: Contiguous{Int16})
@bp_check(Vec{20, NTuple{4, Int8}} <: Contiguous{Int8})
@bp_check([ v2f(), v2f() ] isa Contiguous{Float32})
@bp_check([ (v2b(), v2b()), (v2b(), v2b()) ] isa Contiguous{Bool})
@bp_check(NTuple{5, Vec{2, Float64}} <: Contiguous{Float64})
@bp_check(Vec{1, Vec{2, Vec{3, Bool}}} <: Contiguous{Bool})
@bp_check(Vec{1, NTuple{5, SVector{6, Float16}}} <: Contiguous{Float16})
@bp_check(Vector{Vec{1, NTuple{5, SVector{6, Float16}}}} <: Contiguous{Float16})
@bp_check(MMatrix{6, 17, Vec{1, NTuple{5, SArray{Tuple{6}, Float16, 1, 6}}}, 6*17} <: Contiguous{Float16})
@bp_check(@view([4 5.0 ; 6 7][1:1, 2:2]) isa Contiguous{Float64})
@bp_check(!(Set([4, 5, 6.0]) isa Contiguous{Float64}))
@bp_check(!(keys(Dict(1.0=>5, 4.0=>7, 2.345354=>123.123)) isa Contiguous{Float64}))
@bp_check(!(keys(Dict(1.0=>5, 4.0=>7, 2.345354=>"hi")) isa Contiguous{Float64}))
@bp_check(contiguous_length(5.3, Float64) == 1)
@bp_check(contiguous_length([ 5 ], Int) == 1)
@bp_check(contiguous_length([ 5, 4, 3 ], Int) == 3)
@bp_check(contiguous_length([ v2f(), v2f() ], Float32) == 4)
@bp_check(contiguous_length([ (v2f(), v2f()), (v2f(), v2f()) ], Float32) == 8)
@bp_check(contiguous_length([ (Vec((1, 2), (3, 4)), Vec((5, 6), (7, 8))) ], Int) == 8)
contiguous_data = [ (Vec((1, 2), (3, 4)), Vec((5, 6), (7, 8))),
                    (Vec((9, 10), (11, 12)), Vec((13, 14), (15, 16))) ]
contiguous_data_ref = contiguous_ref(contiguous_data, Int)
@bp_check(contiguous_data isa Contiguous{Int})
@bp_check(contiguous_length(contiguous_data, Int) == 16)
@bp_check(ntuple(i -> unsafe_load(contiguous_ptr(contiguous_data_ref, Int, i)),
                 16) == (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16))

# Test find_matching():
@bp_test_no_allocations_setup(
    vec = [ 4, 5, 6, 7 ],
    find_matching(5, vec),
    2
)

# Test @do_while:
@bp_test_no_allocations_setup(
    i::Int = 0,
    @do_while (i += 1) (i < 5),
    5
)
@bp_test_no_allocations_setup(
    i::Int = 0,
    @do_while (i += 1) false,
    1
)

# Test @optional
@bp_check(tuple(@optional 4>0   3 4.0 true "hi" :world) ==
           (3, 4.0, true, "hi", :world))
@bp_check(tuple(@optional 4<0   3 4.0 true "hi" :world) == ())