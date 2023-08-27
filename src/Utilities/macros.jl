"Gets whether a given expression is a function call/definition"
#NOTE: this snippet comes from MacroTools source; unfortunately it isn't provided explicitly
is_function_expr(expr)::Bool = @capture(longdef(fdef), function (fcall_ | fcall_) body_ end)
export is_function_expr

"A data representation of the output of `splitdef()`"
mutable struct SplitDef
    name
    args::Vector
    kw_args::Vector
    body
    return_type
    where_params::Tuple

    SplitDef(name, args, kw_args, body, return_type, where_params) = new(name, args, kw_args, body, return_type, where_params)
    SplitDef(expr) = let dict = splitdef(expr)
        new(dict[:name], dict[:args], dict[:kwargs], dict[:body],
            get(dict, :rtype, nothing),
            dict[:whereparams])
    end
end
MacroTools.combinedef(struct_representation::SplitDef) = combinedef(Dict(
    :name => struct_representation.name,
    :args => struct_representation.args,
    :kwargs => struct_representation.kw_args,
    :body => struct_representation.body,
    :whereparams => struct_representation.where_params,
))
export SplitDef


"
Turns a function definition into a function call.
The function syntax you pass in may or may not have a body.

For example, passing `:( f(a::Int, b=7; c, d::Float32=10) = a+b/c )` yields
`:( f(a, b; c, d) )`.
"
function func_def_to_call(expr)
    # If the input is a mere function call, grab that data.
    # Otherwise, use MacroTools to parse out the full function defintion.
    data::SplitDef = if isexpr(expr, :call)
                         SplitDef(
                            expr.args[1],
                            isexpr(expr.args[2], :parameters) ?
                                expr.args[3:end] :
                                expr.args[2:end],
                            isexpr(expr.args[2], :parameters) ?
                                expr.args[2].args :
                                [ ],
                            quote end,
                            ()
                         )
                     else
                         SplitDef(expr)
                     end

    # Generate the function call.
    process_arg(a) = let (a_name, _, a_splats, _) = splitarg(a)
        a_splats ? :( $a_name... ) : a_name
    end
    return Expr(:call, data.name,
                @optional(!isempty(data.kw_args),
                          Expr(:parameters, process_arg.(data.kw_args)...)),
                process_arg.(data.args)...)
end
export func_def_to_call


"Maps a modifying assignment operator (like `*=`) to its underlying operator (like `*`)"
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

    :⊻= => :⊻,
    :<<= => :<<,
    :>>= => :>>,
)
"Converts an operator (like `*`) to its assignment operation (like `*=`)"
const ASSIGNMENT_WITH_OP = Dict(v => k for (k,v) in ASSIGNMENT_INNER_OP)
export ASSIGNMENT_INNER_OP, ASSIGNMENT_WITH_OP

"
Computes one of the modifying assignments (`*=`, `&=`, etc) given it and its inputs.
Also implements `=` for completeness.
"
@inline dynamic_modify(s::Symbol, a, b) = dynamic_modify(Val(s), a, b)
@inline dynamic_modify(::Val{:(=)}, a, b) = b
for (name, op) in ASSIGNMENT_INNER_OP
    @eval @inline dynamic_modify(::Val{Symbol($(string(name)))}, a, b) = $op(a, b)
end
export dynamic_modify