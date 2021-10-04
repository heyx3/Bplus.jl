"""
Basically @assert, except it's never compiled out
  and can take any number of message arguments.
"""
macro bp_check(expr, msg...)
    msg = map(esc, msg)
    return quote
        if !($(esc(expr)))
            error("@bp_check(", $(string(expr)), ") failed. ",
                  $(msg...))
        end
    end
end
export @bp_check


using ToggleableAsserts
"""
Short-hand for @toggled_assert, with variable message arguments.
"""
macro bp_assert(expr, msg...)
    msg_expr = :( string($(msg...)) )
    return esc(quote
        @toggled_assert($expr, $msg_expr)
    end)
end
export @bp_assert, @toggled_assert