const Optional{T} = Union{T, Nothing}
@inline exists(x) = !isnothing(x)
export Optional, exists


@inline none(args...) = !any(args...)
export none

"A function that combines a UnionAll and its type parameter(s)"
@inline specify(TOuter, TInner...) = TOuter{TInner...}
export specify


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
macro f32(value)
    return :(Float32($(esc(value))))
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
#

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

"Creates a vector pre-allocated for some expected size."
function preallocated_vector(::Type{T}, capacity::Int) where {T}
    v = Vector{T}(undef, capacity)
    empty!(v)
    return v
end
export preallocated_vector

"
Finds the index/key of the first element matching a desired one.
Returns `nothing` if no key was found.
"
function find_matching(target, collection, comparison = Base.:(==))::Optional
    for (key, value) in pairs(collection)
        if comparison(value, target)
            return key
        end
    end
    return nothing
end
export find_matching


"
Provides a do-while loop for Julia.

Example:

```
i = 0
@do_while begin
    i += 1
end (i < 5)
@test (i == 5)
```
"
macro do_while(to_do, condition)
    return :(
        while true
            $(esc(to_do))
            $(esc(condition)) || break
        end
    )
end
export @do_while