const Optional{T} = Union{T, Nothing}
exists(x) = !isnothing(x)
export Optional, exists


@inline none(args...) = !any(args...)
export none

"A function that combines a UnionAll and its type parameter(s)"
@inline specify(TOuter, TInner...) = TOuter{TInner...}
export specify

"
Union of some outer type, specialized for many inner types.
Examples:
* `@unionspec(Vector{_}, Int, Float64) == Union{Vector{Int}, Vector{Float64}}`
* `@unionspec(Array{Bool, _}, 2, 4) == Union{Array{Bool, 2}, Array{Bool, 4}}`
"
macro unionspec(TOuter, TInners...)
    # Search through tOut for any underscore tokens,
    #    and replace them with tIn.
    function specialize_unders(tOut, tIn)
        if tOut isa Symbol
            if tOut == :_
                return tIn
            else
                return tOut
            end
        elseif tOut isa Expr
            return Expr(tOut.head, map(arg -> specialize_unders(arg, tIn), tOut.args)...)
        else #LineNumberNode or something else to leave alone
            return tOut
        end
    end

    specializations = map(tIn -> esc(specialize_unders(TOuter, tIn)),
                          TInners)

    return :( Union{$(specializations...)} )
end
export @unionspec


"""
A value (or values) that may or may not exist, based on a condition.
Useful for conditionally passing something into a collection or function.
E.x. `print(a, " : ", b, @optional(i>0, "  i=", i))`
"""
macro optional(condition, exprs...)
    condition = esc(condition)
    exprs = map(esc, exprs)
    return :( ($condition ? tuple($(exprs...)) : ())... )
end
export @optional


"
An immutable alternative to Vector, using tuples.
The size is a type parameter, but you are meant to omit it so that it's 'resizable'.
"
const ConstVector{T, N} = NTuple{N, T}
export ConstVector
#TODO: Change ConstVector to be an actual struct inheriting from AbstractArray



"""
Game math is mostly done with 32-bit floats,
   especially when interacting with computer graphics.
This is a quick short-hand for making a 32-bit float.
"""
macro f32(f64)
    return :(Float32($(esc(f64))))
end
export @f32

const Scalar8 = Union{UInt8, Int8}
const Scalar16 = Union{UInt16, Int16, Float16}
const Scalar32 = Union{UInt32, Int32, Float32}
const Scalar64 = Union{UInt64, Int64, Float64}
const Scalar128 = Union{UInt128, Int128}
const ScalarBits = Union{Scalar8, Scalar16, Scalar32, Scalar64, Scalar128}
export Scalar8, Scalar16, Scalar32, Scalar64, Scalar128,
       ScalarBits

"Takes two zipped pieces of data and unzips them into two tuples."
@inline unzip2(zipped) = (tuple((first(a) for a in zipped)...),
                          tuple((last(a) for a in zipped)...))
export unzip2

"A variant of 'reduce()' which skips elements that fail a certain predicate"
@inline function reduce_some(f::Func, pred::Pred, iter; init=0) where {Func, Pred}
    result = init
    for t in iter
        if pred(t)
            result = f(result, t)
        end
    end
    return result
end
export reduce_some