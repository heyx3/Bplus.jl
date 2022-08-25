######################
##   Field to DSL   ##
######################

"Meta-information that's needed to parse the field DSL."
struct DslContext
    n_in::Int
    n_out::Int
    float_type::DataType
end

"Stateful information that's built up when parsing a field DSL."
mutable struct DslState
    # New variables can hide previous ones.
    vars::Dict{Symbol, Stack{AbstractField}}
end
DslState() = DslState(Dict{Symbol, Stack{AbstractField}}())

"Parses an `AbstractField` from the custom DSL, given as a Julia AST"
field_from_dsl(ast,          context::DslContext) = field_from_dsl(ast, context, DslState())
field_from_dsl(ast,          context::DslContext, state::DslState)::AbstractField = error("Unknown type of AST:", typeof(ast))
field_from_dsl(ast::Expr,    context::DslContext, state::DslState) = field_from_dsl_expr(Val(ast.head), ast, context, state)
field_from_dsl(name::Symbol, context::DslContext, state::DslState) = field_from_dsl_var(Val(name), context, state)

# Most fields are represented as some kind of Julia syntax structure (e.x. a function call).
"Parses an `AbstractField` from the custom DSL, as a specific type of Julia `Expr`"
field_from_dsl_expr(type::Val, ast::Expr, context::DslContext, state::DslState) =
    error("Unsupported Expr in DSL: ", type, " \"", ast, "\"")
field_from_dsl_expr(::Val{:call}, ast::Expr, context::DslContext, state::DslState) =
    field_from_dsl_func(Val(ast.args[1]), context, state, tuple(ast.args[2:end]...))
# The "let" syntax defines new variables.
const RESERVED_VAR_NAMES = Symbol[ ]
function field_from_dsl_expr(::Val{:let}, ast::Expr, context::DslContext, state::DslState)
    @bp_fields_assert(length(ast.args) == 2, "Strange 'let' expr: ", ast)

    # Parse and define the variables from the "let" expression.
    new_vars_exprs = ast.args[1]
    if Meta.isexpr(new_vars_exprs, :(=)) # Even if there's a single definition,
                                         #    keep it nested in a block for consistency.
        new_vars_exprs = quote; $new_vars_exprs; end
    end
    @bp_fields_assert(Meta.isexpr(new_vars_exprs, :block),
                      "Strange 'let' variables expression: ", new_vars_exprs)
    var_names = Stack{Symbol}()
    for new_var_expr in new_vars_exprs.args
        if !isa(new_var_expr, LineNumberNode)
            @bp_check(Meta.isexpr(new_var_expr, :(=)),
                      "Variable assignments in a 'let' expression should be of the form ",
                         "'[name] = [field]'. Got: ", new_var_expr)
            @bp_fields_assert(length(new_var_expr.args) == 2,
                              "Strange 'let' assignment expression: ", new_var_expr)
            @bp_check(new_var_expr.args[1] isa Symbol,
                      "Invalid name for variable in a 'let' expression: ", new_var_expr)
            new_var_name::Symbol = new_var_expr.args[1]
            new_var_value = field_from_dsl(new_var_expr.args[2], context, state)

            push!(var_names, new_var_name)
            if !haskey(state.vars, new_var_name)
                state.vars[new_var_name] = Stack{AbstractField}()
            end
            push!(state.vars[new_var_name], new_var_value)
        end
    end

    # Create the field defined by the "let" expression.
    @bp_check(Meta.isexpr(ast.args[2], :block) && (length(ast.args[2].args) == 2) &&
                (ast.args[2].args[1] isa LineNumberNode),
              "The inside of a 'let' expression should be a single statement, but it's: ",
                 ast.args)
    output = field_from_dsl(ast.args[2].args[2], context, state)
    
    # The "let" expression is now closed, so remove those variables.
    while !isempty(var_names)
        var_name = pop!(var_names)
        @bp_fields_assert(haskey(state.vars, var_name) &&
                          !isempty(state.vars[var_name]))
        pop!(state.vars[var_name])
    end

    return output
end

# Most fields are specifically represented as a Julia function/operator call.
"Parses an `AbstractField` from the custom DSL, as a Julia function/operator call"
field_from_dsl_func(name::Val, ::DslContext, ::DslState, args::Tuple) =
    error("Unsupported function call in DSL: ", name, " (", join(args, ", "), ")")
#

# The DSL can define variables representing fields.
# A few special fields also have special constant names.
function field_from_dsl_var(name_param::Val, ::DslContext, state::DslState)
    name::Symbol = typeof(name_param).parameters[1]
    if haskey(state.vars, name)
        return first(state.vars[name])
    else
        error("Unknown field variable: '", name, "'")
    end
end

export field_from_dsl, DslContext


######################
##   DSL to Field   ##
######################

"Converts an `AbstractField` into the custom DSL (as a Julia AST)."
dsl_from_field(field::AbstractField) = error("DSL generation not implemented for ", typeof(field).name.name)

export dsl_from_field