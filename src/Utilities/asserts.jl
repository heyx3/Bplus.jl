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
        Redefine this function to enable/disable asserts.
  * `@Xassert`, to do an assert.
  * `@Xdebug a [b]`, to evaluate 'a' if asserts are enabled, and optionally 'b' otherwise.

These generated items are not exported, as the whole goal is to keep the asserts internal.
The asserts are disabled by default, so that release builds don't pay extra JIT cost.
You should redefine them when running tests.
"""
macro make_toggleable_asserts(prefix::Symbol)
    name_toggler = Symbol(prefix, "asserts_enabled")
    name_assert = Symbol(prefix, "assert")
    name_code = Symbol(prefix, "debug")
    return esc(quote
        Core.@__doc__ macro $name_assert(condition, msg...)
            condition = esc(condition)
            msg_expr = :( string($(map(esc, msg)...)) )
            name_toggler = Symbol($(string(name_toggler)))
            return :(
                if $name_toggler()
                    @assert $condition $msg_expr
                end
            )
        end
        macro $name_code(to_do, to_do_else...)
            name_toggler = Symbol($(string(name_toggler)))
            return isempty(to_do_else) ? :(
                if $name_toggler()
                    $(esc(to_do))
                end
            ) : :(
                if $name_toggler()
                    $(esc(to_do))
                else
                    $(map(esc, to_do_else)...)
                end
            )
        end
        @inline $name_toggler() = false
    end)
end
export @make_toggleable_asserts