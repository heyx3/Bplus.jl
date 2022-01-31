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


#TODO: Keep one local copy of ToggleableAsserts for all of Bplus
"""
The ToggleableAsserts package allows you to call asserts
   which can be toggled on or off by the JIT compiler.
However, this means that all projects which use that package
   are forced to keep all asserts on or off, as a group.
This macro generates the ToggleableAsserts code on command,
   so that you can toggle asserts for a specifc module without affecting other ones.
In particular it generates the following (assuming the prefix `X`):
  * `Xasserts_enabled()`, to test if asserts are enabled.
  * `@Xassert`, to do an assert.
  * `@Xdebug`, to execute some code if asserts are enabled.
These generated items are not exported, as the whole goal is to keep the asserts internal.
By default, the asserts are disabled. You should enable them when running tests.
"""
macro make_toggleable_asserts(prefix::Symbol)
    name_toggler = Symbol(prefix, "asserts_enabled")
    name_assert = Symbol(prefix, "assert")
    name_code = Symbol(prefix, "debug")
    return esc(quote
        macro $name_assert(condition, msg...)
            condition = esc(condition)
            msg_expr = :( string($(map(esc, msg)...)) )
            name_toggler = Symbol($(string(name_toggler)))
            return :(
                if $name_toggler()
                    @assert $condition $msg_expr
                end
            )
        end
        macro $name_code(to_do)
            name_toggler = Symbol($(string(name_toggler)))
            return :(
                if $name_toggler()
                    $(esc(to_do))
                end
            )
        end
        @inline $name_toggler() = false
    end)
end
export @make_toggleable_asserts