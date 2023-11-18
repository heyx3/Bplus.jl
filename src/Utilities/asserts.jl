"""
Basically @assert, except it's never compiled out
  and can take any number of message arguments.
"""
macro bp_check(expr, msg...)
    msg = map(esc, msg)
    return quote
        if !($(esc(expr)))
            error($(msg...),
                  $((isempty(msg) ? "" : ".\n  ")),
                    "@bp_check(", $(string(expr)), ") failed.")
        end
    end
end
export @bp_check


"""
This is an alternative to *ToggleableAsserts.jl*.
That package allows you to call asserts which can be toggled on or off by the JIT compiler;
    however all projects using the package must respect a single global flag.

This macro generates ToggleableAsserts-style code,
    so that you can have multiple separate instances of ToggleableAsserts.
B+ uses this to give each sub-module its own debug flag,
    and you can use it to give your project its own debug flag as well.

Invoking `@make_toggleable_asserts X_` generates the following:
  * `@X_assert(condition, msg...)` performs a check if `X_asserts_enabled()`,
       and throws an error if the check fails.
  * `@X_debug()` evaluates to a boolean for whether or not asserts are enabled.
  * `@X_debug a [b]`, to evaluate 'a' if asserts are enabled, and optionally 'b' otherwise.
  * `X_asserts_enabled()` represents the debug flag; it's a function that returns the constant `false`.
    * For debug mode, your scripts can redefine it to return `true` instead.

These generated items are not exported by default.
Additionally, it's configured for "release" mode by default
    so that release builds don't pay the extra JIT cost.
When running tests, redefine `X_asserts_enabled() = true`.

Note that constants are not part of JIT recompilation,
    so a `const` global should never be defined in terms of the above macros.
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
        macro $name_code(to_do...)
            name_toggler = Symbol($(string(name_toggler)))
            if isempty(to_do)
                return :( $name_toggler() )
            elseif length(to_do) == 1
                return :( if $name_toggler()
                              $(esc(to_do))
                          end )
            elseif length(to_do) == 2
                return :( if $name_toggler()
                              $(esc(to_do[1]))
                          else
                              $(esc(to_do[2]))
                          end )
            else
                error("Expected 0, 1, or 2 arguments to , $(string(name_code))")
            end
        end
        @inline $name_toggler() = false
    end)
end
export @make_toggleable_asserts