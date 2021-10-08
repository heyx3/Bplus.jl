"""
Basically @assert, except it's never compiled out
  and can take any number of message arguments.
"""
macro bp_check(expr, msg...)
    msg = map(esc, msg)
    return quote
        if !($(esc(expr)))
            error($(msg...), ". @bp_check(", $(string(expr)), ") failed.")
        end
    end
end
export @bp_check


"""
The ToggleableAsserts package allows you to call asserts
   which can be toggled on or off by the JIT compiler.
However, this means that all projects which use that package
   are forced to keep all asserts on or off, as a group.
This macro generates the ToggleableAsserts code on command,
   so that you can toggle asserts for a specifc module without affecting other ones.
In particular it generates an `Xasserts_enabled()` and an `@Xassert`, where `X` is some prefix.
These generated items are NOT exported, as the whole idea is to keep the asserts internal.
By default, the asserts are disabled. You should enable them when running tests.
"""
macro make_toggleable_asserts(prefix::Symbol)
    name_toggler = Symbol(prefix, "asserts_enabled")
    name_assert = Symbol(prefix, "assert")
    return esc(quote
        macro $name_assert(condition, msg...)
            condition = esc(condition)
            msg_expr = esc(:( string($msg...) ))
            name_toggler = Symbol($(string(name_toggler)))
            return quote
                if $name_toggler()
                    @assert $condition $msg_expr
                end
            end
        end
        @inline $name_toggler() = false
    end)
end
export @make_toggleable_asserts