const Optional{T} = Union{T, Nothing}
exists(x) = !isnothing(x)
export Optional, exists

@inline none(args...) = !any(args...)
export none

"""
Game math is mostly done with 32-bit floats,
   especially when interacting with computer graphics.
This is a quick short-hand for making a 32-bit float.
"""
macro f32(f64)
    return :(Float32($(esc(f64))))
end
export @f32


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