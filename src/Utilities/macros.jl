"Gets whether a given expression is a function call"
is_function_call(expr) = (
    if isexpr(expr, :call)
        true
    elseif isexpr(expr, :(::))
        is_function_call(expr.args[1])
    elseif isexpr(expr, :where)
        is_function_call(expr.args[1])
    else
        false
    end
)

"Gets whether a given expression is a function definition"
is_function_def(expr) = (
    if isexpr(expr, :function)
        true
    elseif isexpr(expr, :(=))
        is_function_call(expr.args[1])
    else
        false
    end
)

"Gets whether a given expression is a struct field declaration"
is_field_def(expr) = (
    if expr isa Symbol
        true
    elseif isexpr(expr, :(::))
        expr.args[1] isa Symbol
    elseif isexpr(expr, :(=))
        is_field_def(expr.args[1])
    else
        false
    end
)

"Converts a modifying assignment operator (like `*=`) to its underlying operator (like `*`)"
const ASSIGNMENT_INNER_OP = Dict(
    :+= => :+,
    :-= => :-,
    :*= => :*,
    :/= => :/,
    :^= => :^,
    :÷= => :÷,
    :%= => :%,

    :|= => :|,
    :&= => :&,
    :!= => :!,

    :⊻= => :⊻,
    :<<= => :<<,
    :>>= => :>>,
)
"Converts an operator (like `*`) to its assignment operation (like `*=`)"
const ASSIGNMENT_WITH_OP = Dict(v => k for (k,v) in ASSIGNMENT_INNER_OP)

export is_function_call, is_function_def, is_field_def,
       ASSIGNMENT_INNER_OP, ASSIGNMENT_WITH_OP