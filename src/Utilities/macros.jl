"
Information that can be part of a function definition but isn't normally handled,
    like `@inline` or doc-strings.

Turn it back into code with `MacrooTools.combinedef()`.
"
struct FunctionMetadata
    doc_string::Optional{AbstractString}
    inline::Bool
    generated::Bool
    core_expr # The actual function definition with everything stripped out
end

"
Returns `nothing` if the expression doesn't look like a valid function definition
    (i.e. it's wrapped by an unexpected macro, or not even a function in the first place).
"
function FunctionMetadata(expr)::Optional{FunctionMetadata}
    if @capture expr (@name_ args__)
        if name == GlobalRef(Core, Symbol("@doc"))
            doc_string = args[1]
            inner = FunctionMetadata(args[2])
            return FunctionMetadata(doc_string, inner.inline, inner.generated, inner.core_expr)
        elseif name == Symbol("@inline")
            inner = FunctionMetadata(args[1])
            return FunctionMetadata(inner.doc_string, true, inner.generated, inner.core_expr)
        elseif name == Symbol("@generated")
            inner = FunctionMetadata(args[1])
            return FunctionMetadata(inner.doc_string, inner.inline, true, inner.core_expr)
        else
            return nothing
        end
    #NOTE: this snippet comes from MacroTools source; unfortunately it isn't provided explicitly
    elseif @capture(longdef(expr), function (fcall_ | fcall_) body_ end)
        return FunctionMetadata(nothing, false, false, expr)
    else
        return nothing
    end
end

"Converts a `FunctionMetadata` to the original Expr it came from"
function MacroTools.combinedef(m::FunctionMetadata)
    output = m.core_expr
    if m.generated
        output = :( @generated $output )
    end
    if m.inline
        output = :( @inline $output )
    end
    if exists(m.doc_string)
        output = :( Core.@doc($(m.doc_string), $output) )
    end
    return output
end

export FunctionMetadata


"Gets whether a given expression is a function call/definition"
is_function_expr(expr)::Bool = exists(FunctionMetadata(expr))
export is_function_expr

"
A data representation of the output of `splitdef()`,
    plus the ability to recognize doc-strings and `@inline`
"
mutable struct SplitDef
    name
    args::Vector
    kw_args::Vector
    body
    return_type
    where_params::Tuple
    doc_string::Optional{AbstractString}
    inline::Bool
    generated::Bool

    function SplitDef(expr)
        metadata = FunctionMetadata(expr)
        expr = metadata.core_expr

        dict = splitdef(expr)
        return new(
            dict[:name], dict[:args], dict[:kwargs], dict[:body],
            get(dict, :rtype, nothing),
            dict[:whereparams],
            metadata.doc_string, metadata.inline, metadata.generated
        )
    end
    SplitDef(s::SplitDef) = deepcopy(s)
end
function MacroTools.combinedef(struct_representation::SplitDef)
    definition = combinedef(Dict(
        :name => struct_representation.name,
        :args => struct_representation.args,
        :kwargs => struct_representation.kw_args,
        :body => struct_representation.body,
        :whereparams => struct_representation.where_params,
        @optional(exists(struct_representation.return_type),
                :rtype => struct_representation.return_type)
    ))
    return combinedef(FunctionMetadata(
        struct_representation.doc_string,
        struct_representation.inline,
        struct_representation.generated,
        definition
    ))
end
export SplitDef


"
Turns a function definition into a function call.
The function syntax you pass in may or may not have a body.

For example, passing `:( f(a::Int, b=7; c, d::Float32=10) = a+b/c )` yields
`:( f(a, b; c, d) )`.
"
function func_def_to_call(expr)
    data::SplitDef = if isexpr(expr, :call)
                         SplitDef(:( $expr = nothing ))
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