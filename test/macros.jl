const BASIC_EXPRS = [
    # 1-5: Short function definitions.
    :( f(i) = 4 ),
    :( f(i)::Int = i ),
    :( f(i::I) where {I} = i ),
    :( f(i::I) where {I<:Integer} = i ),
    :( f(i::Int)::Int = i ),
    # 6-10: Long function definitions.
    :( function f(i); return i; end ),
    :( function f(i)::Int; return i; end ),
    :( function f(i::I) where {I}; return i; end ),
    :( function f(i::I) where {I<:Integer}; return i; end ),
    :( function f(i::I)::Integer where {I<:Integer}; return i; end ),
    # 11-15: Function calls.
    :( f(i) ),
    :( f(i)::Int ),
    :( f(i::I) where {I} ),
    :( f(i::I) where {I<:Integer} ),
    :( f(i::Int)::Int ),
    # 16-19: Field declarations.
    :( i ),
    :( i::Int ),
    :( i = nothing ),
    :( i::Int = 5 )
]
@assert map(is_function_call, BASIC_EXPRS) == [
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    1, 1, 1, 1, 1,
    0, 0, 0, 0
]
@assert map(is_function_def, BASIC_EXPRS) == [
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,
    0, 0, 0, 0, 0,
    0, 0, 0, 0
]
@assert map(is_field_def, BASIC_EXPRS) == [
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    1, 1, 1, 1
]