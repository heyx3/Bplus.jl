# At the top of this file are definitions to convert Julia syntax structures into fields.
# Below that is the high-level language/macro, to string multiple pre-computed fields together.


######################
##   Field to DSL   ##
######################

"Meta-information that's needed to parse the field DSL."
struct DslContext
    n_in::Int
    # "n_out" is inferred during parsing, not specified here
    float_type::DataType
end

"A lookup for variables, arrays, etc. Gets modified during DSL parsing."
mutable struct DslState
    # New variables can hide previous ones.
    vars::Dict{Symbol, Stack{AbstractField}}
    # TextureFields use a lookup of names to arrays.
    arrays::Dict{Symbol, Array}
end
DslState() = DslState(Dict{Symbol, Stack{AbstractField}}(), Dict{Symbol, Array}())

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
const RESERVED_VAR_NAMES = Set{Symbol}()
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
            @bp_check(!in(new_var_name, RESERVED_VAR_NAMES),
                      "Can't use the name '", new_var_name, "' for a variable")
            @bp_check(!haskey(state.arrays, new_var_name),
                      "Can't use the name '", new_var_name, "', as it already refers to an array/texture")

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

export field_from_dsl, DslContext, DslState


######################
##   DSL to Field   ##
######################

"Converts an `AbstractField` into the custom DSL (as a Julia AST)."
dsl_from_field(field::AbstractField) = error("DSL generation not implemented for ", typeof(field).name.name)

export dsl_from_field


###########################
##   High-level syntax   ##
###########################

"""
Defines a single Julia field.

Takes 3 or 4 arguments:
* The input dimensions, as an integer.
* The number type being used (e.x. `Float32`).
* **[Optional]** An instance of a `DslState`
* The field's value (e.x. `perlin(pos.yzx * 7)`)

Sample uses:

````
# Define a 3D voxel grid, i.e. 3D input to 1D output.
# The value of the grid comes from Perlin noise.
perlin_field = @field 3 Float32 perlin(pos)

# Define an "image" field, a.k.a. 2D pixel coordinates to 3D color (RGB).
# Use half-precision, a.k.a. 16-bit float.
# The previous field is available for reference through a special lookup.
field_lookups = Dict(:my_perlin => Stack{AbstractField}([ perlin_field ]))
array_lookups = Dict{Symbols, Array}()
dsl_state = DslState(field_lookups, array_lookups)
image_field = @field(2, Float16, dsl_state,
    # Convert the perlin noise down to 16-bit float,
    #    then spread its scalar value across all 3 color channels.
    (my_perlin => Float16).xxx
)
````
"""
macro field(args...)
    return field_macro_impl(args...)
end
field_macro_impl(input_dims_expr, component_type_name::Symbol, field_expr) = field_macro_impl(
    input_dims_expr, component_type_name,
    :( DslState() ),
    field_expr
)
function field_macro_impl(input_dims_expr, component_type_name::Symbol,
                          dsl_state_expr, field_expr)
    return :(
        let context = DslContext(Int($input_dims_expr),
                                 $component_type_name)
            field_from_dsl(
                $(Expr(:quote, field_expr)),
                context,
                $dsl_state_expr
            )
        end
    )
end
export @field


"
A sequence of fields.
Each one will be sampled into an array and exposed to subsequent fields in the form of a `TextureField`.
"
struct MultiField
    sequence::Vector{Pair{Symbol, Tuple{AbstractField, Array{<:Vec}}}}
    finale::AbstractField
end
export MultiField

"Samples from a `MultiField`, by running its full sequence and then sampling from its finale"
function sample_field!(array::Array, mf::MultiField,
                       state::DslState = DslState()
                       ;
                       use_threading::Bool = true
                      )
    for (name::Symbol, (field::AbstractField, output::Array{<:Vec})) in mf.sequence
        sample_field!(output, field;
                      use_threading = use_threading)
        @bp_check(!haskey(state.arrays, name),
                  "Array name '", name, "' would overwite existing array")
        state.arrays[name] = output 
    end
    sample_field!(array, mf.finale;
                  use_threading = use_threading)

    return nothing
end

"""
Defines a sequence of fields, each one getting sampled into an array/texture
    and used by subsequent fields, culminating in a final field
    which can sample from all of the previous ones.

For the syntax of each field, refer to the `@field` macro.

Sample usage:

````
@multi_field begin
    # 2D perlin noise, sampled into a 128x128 texture.
    perlin1 = 128 => @field(2, Float32, perlin(pos * 20))

    # 3D perlin noise, sampled into a 32x32x128 texture.
    perlin2 = {32, 32, 128} => @field(3, Float32, perlin(pos * 20 * { 1, 1, 10 }))

    # The output field:
    @field(3, Float32,
        (perlin1{pos.xy}) *
        (perlin2{pos})
    )
end
````
"""
macro multi_field(args...)
    return multi_field_macro_impl(args...)
end
function multi_field_macro_impl(fields_block)
    @bp_check(Meta.isexpr(fields_block, :block),
              "Last argument to @multi_field should be a begin...end block")

    field_exprs = filter(arg -> !isa(arg, LineNumberNode), fields_block.args)
    @bp_check(!isempty(field_exprs), "Needs at least one @field (the final output)")

    # The last field expression is the "finale" field.
    @bp_check(Meta.isexpr(field_exprs[end], :macrocall) &&
                  (field_exprs[end].args[1] == Symbol("@field")),
              "Expected a @field() call for the last expression. Got: ", field_exprs[end])
    finale_expr = field_exprs[end]
    # Replace the @field with a call straight into the @field macro's implementation --
    #    nested macro calls cause trouble.
    # Note that the first argument to :macrocall is the name, second is a LineNumberNode.
    finale_expr = :( $field_macro_impl($(finale_expr.args[3:end]...)) )
    deleteat!(field_exprs, length(field_exprs))

    sequence_exprs = [ ]
    for (i, field_expr) in enumerate(field_exprs)
        @bp_check(Meta.isexpr(field_expr, :(=)),
                  "Fields should be specified as '[name] = [value]': \"$field_expr\"")

        @bp_fields_assert(length(field_expr.args) == 2, "Weird AST: ", field_expr)

        # Grab the field's name.
        @bp_check(field_expr.args[1] isa Symbol,
                  "Field name should be a plain token, not '", field_expr.args[1], "'")
        field_name::Symbol = field_expr.args[1]
        field_name_expr = QuoteNode(field_name)

        # Grab the field's texture size and value.
        @bp_check(Meta.isexpr(field_expr.args[2], :call) && (field_expr.args[2].args[1] == :(=>)),
                  "Field '", field_name, "' should have the value [texture size] => [value]: ",
                    field_expr.args[2])
        (field_resolution_expr, field_value_expr) = field_expr.args[2].args[2:3]
        @bp_check(Meta.isexpr(field_value_expr, :macrocall) &&
                     field_value_expr.args[1] == Symbol("@field"),
                  "Field's value should be a call into the @field macro: \"", field_value_expr, "\"")

        # Replace the @field with a call straight into the @field macro's implementation --
        #    nested macro calls cause trouble.
        # Note that the first argument to :macrocall is the name, second is a LineNumberNode.
        field_value_expr = :( $field_macro_impl($(field_value_expr.args[3:end]...)) )

        # Generate an expression for the field's texture size.
        # It's either a single size for all axes, or a per-axis size.
        tex_size_component_expr =
            if Meta.isexpr(field_resolution_expr, :braces)
                quote
                    tex_size = tuple($(field_resolution_expr.args...))
                    @bp_check(length(tex_size) == field_input_size(field),
                              "Field expects ", field_input_size(field), "D input,",
                                " but you specified a ", length(tex_size),
                                "D array to sample from it")
                    tex_size
                end
            else
                :( let size1 = $field_resolution_expr
                     ntuple(i -> size1, field_input_size(field))
                   end )
            end

        field_expr = :( field_from_dsl($(Expr(:quote, field_expr.args[1])),
                                       context, state) )

        # Add a block of code which defines this field and its array,
        #    and updates a DslState with a reference to the array.
        push!(sequence_exprs, :(
            let field = $field_expr,
                OutVec = Vec{field_output_size(field), field_component_type(field)},
                array = Array{OutVec, field_input_size(field)}(undef, $tex_size_component_expr)
              state.arrays[$field_name_expr] = array
              $field_name_expr => (field, array)
            end
        ))
    end

    return :( let state = DslState()
        MultiField(
            [ $(sequence_exprs...) ],
            $finale_expr
        )
    end )
end

export MultiField, @multi_field