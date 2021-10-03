module Utilities

Optional{T} = Union{T, Nothing}
exists(x) = !isnothing(x)
export Optional, exists

"""
Basically @assert, except it's never compiled out
  and can take any number of message arguments.
"""
macro check(expr, msg...)
    msg = map(esc, msg)
    return quote
        if !($(esc(expr)))
            error("@check(", $(string(expr)), ") failed. ",
                  $(msg...))
        end
    end
end
export @check

"""
Game math is mostly done with 32-bit floats,
   especially when interacting with computer graphics.
This is a quick short-hand for making a 32-bit float.
"""
macro f32(f64)
    return :(Float32($(esc(f64))))
end
export @f32

end