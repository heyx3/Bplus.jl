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

end