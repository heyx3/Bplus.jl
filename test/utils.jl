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

# Test ConstVector
@bp_check((4.0, 3.0, 1.0, 4.0) isa ConstVector{Float64})

# Test @optional
@bp_check(tuple(@optional 4>0   3 4.0 true "hi" :world) ==
           (3, 4.0, true, "hi", :world))
@bp_check(tuple(@optional 4<0   3 4.0 true "hi" :world) == ())