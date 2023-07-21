###################
#  Swizzle field  #
###################

"Swaps the components of a field's output"
struct SwizzleField{ NIn, NOut, F,
                     Swizzle, # Either: Symbol of the swizzle property, OR tuple of indices
                     TField<:AbstractField{NIn}
                   } <: AbstractField{NIn, NOut, F}
    field::TField
end
export SwizzleField

function SwizzleField( field::AbstractField{NIn, NOutSrc, F},
                       swizzle::Symbol
                     ) where {NIn, NOutSrc, F}
    NOutDest = length(string(swizzle))
    return SwizzleField{NIn, NOutDest, F, swizzle, typeof(field)}(field)
end
function SwizzleField( field::AbstractField{NIn, NOutSrc, F},
                       indices::Int...
                     ) where {NIn, NOutSrc, F}
    NOutDest = length(indices)
    return SwizzleField{NIn, NOutDest, F, Tuple{indices...}, typeof(field)}(field)
end

# Converts a swizzle field's type parameter into a tuple of integers representing the indices to use.
@inline swizzle_index_tuple(::SwizzleField{NIn, NOut, F, Swizzle, TField}) where {NIn, NOut, F, Swizzle<:Tuple, TField} = swizzle_index_tuple(Swizzle)
@inline swizzle_index_tuple(::Type{Swizzle}) where {Swizzle<:Tuple} = tuple(Swizzle.parameters...)

prepare_field(f::SwizzleField{NIn}) where {NIn} = prepare_field(f.field)

@generated function get_field( f::SwizzleField{NIn, NOut, F, Swizzle, TField},
                               pos::Vec{NIn, F},
                               prepared_data
                             )::Vec{NOut, F} where {NIn, NOut, F, Swizzle, TField}
    if (Swizzle isa Type) && (Swizzle <: Tuple)
        indices = swizzle_index_tuple(Swizzle)
        output_components = map(i -> :( input[$i] ), indices)
        return quote
            input = get_field(f.field, pos, prepared_data)
            return Vec{NOut, F}($(output_components...))
        end
    elseif Swizzle isa Symbol
        return quote
            input::VecT{F} = get_field(f.field, pos, prepared_data)
            return getproperty(input, Swizzle)
        end
    else
        return :( error("Unkonwn swizzle type: ", Swizzle) )
    end
end
@generated function get_field_gradient( f::SwizzleField{NIn, NOut, F, Swizzle, TField},
                                        pos::Vec{NIn, F},
                                        prepared_data
                                      )::Vec{NIn, Vec{NOut, F}} where {NIn, NOut, F, Swizzle, TField}
    return quote
        input = get_field_gradient(f.field, pos, prepared_data)
        return Vec{NIn, Vec{NOut, F}}(
            $((
                # Swizzle the derivative along each individual input axis.
                map(1:NIn) do axis::Int
                    return :(
                        let derivative = input[$axis]
                            Vec{NOut, F}(
                                $((
                                    # For each index of the swizzle, extract that component.
                                    if (Swizzle isa Type) && (Swizzle <: Tuple)
                                        map(swizzle_index_tuple(Swizzle)) do src_component::Int
                                            return :( derivative[$src_component] )
                                        end
                                    elseif Swizzle isa Symbol
                                        map(collect(string(Swizzle))) do property::Char
                                            return :( derivative.$(Symbol(property)) )
                                        end
                                    else
                                        :( error("Unexpected Swizzle type: ", Swizzle) )
                                    end
                                )...)
                            )
                        end
                    )
                end
            )...)
        )
    end
end

# Swizzling can be done with the property syntax (e.x. "pos.y"),
#    or with array accesses (e.x. "pos[1, 3, 2]" is like "pos.xzy").
function field_from_dsl_expr(::Val{:.}, ast::Expr, context::DslContext, state::DslState)
    source_expr = ast.args[1]
    swizzle_expr = ast.args[2]

    source = field_from_dsl(source_expr, context, state)
    @bp_check(swizzle_expr isa QuoteNode,
              "Component lookups into field isn't a QuoteNode as expected: ",
                 typeof(swizzle_expr), " ", swizzle_expr)

    return SwizzleField(source, swizzle_expr.value)
end
function field_from_dsl_expr(::Val{:ref}, ast::Expr, context::DslContext, state::DslState)
    source_expr = ast.args[1]
    swizzle_expr = ast.args[2:end]

    source = field_from_dsl(source_expr, context, state)
    @bp_check(all(e -> e isa Integer, swizzle_expr),
              "Index lookups into field must be integer literals: \"", ast, "\"")

    return SwizzleField(source, swizzle_expr...)
end
function dsl_from_field(s::SwizzleField{NIn, NOut, F, Swizzle, TField}) where {NIn, NOut, F, Swizzle, TField}
    source = dsl_from_field(s.field)

    if Swizzle isa Symbol
        return :( $source.$Swizzle )
    elseif (Swizzle isa Type) && (Swizzle <: Tuple)
        return :( $source[$(Swizzle.parameters...)] )
    else
        error("Unexpected Swizzle type: ", Swizzle)
    end
end


####################
#  Gradient field  #
####################

"""
Samples the gradient of a field, in a given direction.
The direction can be an `Integer` representing the position axis,
    a vector (`AbstractField{NIn, NIn, F}`) that is dotted with the gradient,
    or `Nothing` if the output should be a Vec with all components flattened together
    (this is most useful for 1D fields, where the gradient is just like a position input).
"""
struct GradientField{ NIn, NOut, F,
                      TField<:AbstractField{NIn, NOut, F},
                      TDir<:Union{Nothing, Integer, AbstractField{NIn, NIn, F}}
                    } <: AbstractField{NIn, NOut, F}
    field::TField
    dir::TDir
end
export GradientField

"Constructor for a GradientField that appends all the output gradients together into one vector"
GradientField(field::AbstractField{NIn, NOut, F}) where {NIn, NOut, F} = GradientField{NIn, NIn * NOut, F, typeof(field), Nothing}(field)

prepare_field(g::GradientField{NIn, NOut, F, TField, TDir}) where {NIn, NOut, F, TField, TDir} =
    if TDir isa AbstractField
        prepare_field.((g.field, g.dir))
    else
        tuple(prepare_field(g.field), nothing)
    end

@generated function get_field( g::GradientField{NIn, NOut, F, TField, TDir},
                               pos::Vec{NIn, F},
                               prep_data::Tuple = (nothing, nothing)
                             ) where {NIn, NOut, F, TField, TDir}
    gradient_expr = :(
        gradient::Vec{NIn, Vec{NOut, F}} = get_field_gradient(g.field, pos)
    )
    if TDir <: Integer
        return :( $gradient_expr[g.dir] )
    elseif TDir <: AbstractField{NIn, NIn, F}
        return quote
            gradient = $gradient_expr
            dir::Vec{NIn, F} = get_field(g.dir, pos, prep_data[2])
            output_per_input_axis = gradient * dir
            return reduce(+, output_per_input_axis)
        end
    elseif TDir <: Nothing
        return quote
            gradient = $gradient_expr
            return Vec{NOut + NIn}((
                :( gradient[$i]... )
                  for i in 1:NIn
            )...)
        end
    else
        error("Unexpected type for GradientField's direction input: ", TDir)
    end
end


# Gradients are specified as a function call, with special internal syntax:
#    "gradient([field], [dir])"
# The "[field]" is a normal field expression.
# "The "[dir]" is either an integer representing the axis of the change direction,
#    a field providing the vector of the change direction,
#    or nonexistent (to append all the per-axis derivatives together).
function field_from_dsl_func(::Val{:gradient}, context::DslContext, state::DslState, args::Tuple)
    @bp_check(length(args) in 1:2,
              "gradient() needs 1 or 2 arguments, the input field and optionally the direction.",
                " Got ", length(args), ": ", args)

    input_field = field_from_dsl(args[1], context, state)

    local input_dir
    if length(args) < 2
        input_dir = nothing
    elseif args[2] isa Integer
        input_dir = args[2]
    else
        input_dir = field_from_dsl(args[2], context, state)
        @bp_check(field_output_size(input_dir) == field_input_size(input_field),
                  "Direction input for gradient needs to be a ", field_input_size(input_field),
                    "-dimensional output, but it's ", field_output_size(input_dir), "-dimensional")
    end

    return GradientField(input_field, input_dir)
end
function dsl_from_field(g::GradientField{NIn, NOut, F, TField, TDir}) where {NIn, NOut, F, TField, TDir}
    field = dsl_from_field(g.field)
    local dir_tuple::Tuple # May be 1 argument, or 0
    if g.dir isa Integer
        dir_tuple = tuple(g.dir)
    elseif g.dir isa Nothing
        dir_tuple = tuple()
    elseif g.dir isa AbstractField
        dir_tuple = dsl_from_field(g.dir)
    else
        error("Unhandled case: ", typeof(g.dir), "\n\t", g.dir)
    end
    return :( gradient(field, $(dir_tuple...)) )
end


##################
#  Append field  #
##################

"Combines multiple fields together into a single, higher-dimensional output"
struct AppendField{NIn, NOut, F, TInputs<:Tuple} <: AbstractField{NIn, NOut, F}
    inputs::TInputs
end
export AppendField

function AppendField(fields::AbstractField{NIn}...) where {NIn}
    @bp_check(length(fields) > 0,
              "Need to provide at least one input to the AppendField ('{ }')")
    F = field_component_type(fields[1])
    @bp_check(all(field -> field_component_type(field) == F, fields),
              "All fields should be using the same number type: ",
                map(field_component_type, fields))

    NOut = sum(field_output_size, fields)
    return AppendField{NIn, NOut, F, typeof(fields)}(fields)
end

@inline prepare_field(f::AppendField) = tuple((
    prepare_field(i) for i in f.inputs
)...)
@generated function get_field( field::TAppend,
                               pos::Vec{NIn, F},
                               prep_data::Tuple
                             ) where {NIn, NOut, F, TInputs, TAppend<:AppendField{NIn, NOut, F, TInputs}}
    input_components = map(1:length(TInputs.parameters)) do i::Int
        return :( get_field(field.inputs[$i], pos, prep_data[$i])... )
    end
    return :( Vec{NOut, F}($(input_components...)) )
end
@generated function get_field_gradient( field::TAppend,
                                        pos::Vec{NIn, F},
                                        prep_data::Tuple
                                      ) where {NIn, NOut, F, TInputs, TAppend<:AppendField{NIn, NOut, F, TInputs}}
    input_gradients = map(1:length(TInputs.parameters)) do i::Int
        return :( get_field_gradient(field.inputs[$i], pos, prep_data[$i]) )
    end

    derivatives = map(1:NIn) do axis::Int
        gradient_pieces = map(1:length(TInputs.parameters)) do input_idx::Int
            return :( input_gradients[$input_idx][$axis] )
        end
        return :( Vec{NOut, F}($(gradient_pieces...)) )
    end

    return quote
        input_gradients = tuple($(input_gradients...))
        return Vec{NIn, Vec{NOut, F}}($(derivatives...))
    end
end

# Append in the DSL uses the braces syntax, e.x. '{ pos, 1 }'.
function field_from_dsl_expr(::Val{:braces}, ast::Expr, context::DslContext, state::DslState)
    args = map(arg -> field_from_dsl(arg, context, state), ast.args)
    return AppendField(args...)
end
dsl_from_field(a::AppendField) = :( { $(dsl_from_field.(a.inputs)...) } )


######################
#  Conversion field  #
######################

"
Translates data from one float-type to another.
You can't really get different float types within a single field expression,
    but the full DSL allows you to compose multiple field expresions together.
"
struct ConversionField{ NIn, NOut, FIn, FOut,
                        TInput<:AbstractField{NIn, NOut, FIn}
                      } <: AbstractField{NIn, NOut, FOut}
    input::TInput
end
export ConversionField

ConversionField( input::AbstractField{NIn, NOut, FIn},
                 FOut::Type{<:Real}
               ) where {NIn, NOut, FIn} = ConversionField{NIn, NOut, FIn, FOut, typeof(input)}(input)

prepare_field(c::ConversionField) = prepare_field(c.input)

# Point of clarification: the position input is of type FOut, not FIn, because
#    the ConversionField itself is FOut.
get_field(c::ConversionField{NIn, NOut, FIn, FOut}, pos::Vec{NIn, FOut}, prep_data) where {NIn, NOut, FIn, FOut} =
    convert(Vec{NOut, FOut}, get_field(c.input, convert(Vec{NIn, FIn}, pos), prep_data))
get_field_gradient(c::ConversionField{NIn, NOut, FIn, FOut}, pos::Vec{NIn, FOut}, prep_data) where {NIn, NOut, FIn, FOut} =
    convert(Vec{NIn, Vec{NOut, FOut}}, get_field_gradient(c.input, convert(Vec{NIn, FIn}, pos), prep_data))

# The DSL is done with the "=>" operator. E.x. "my_field => Float64"
function field_from_dsl_func(::Val{:(=>)}, context::DslContext, state::DslState, args::Tuple)
    @bp_fields_assert(length(args) == 2, "Huh? $args")
    input = field_from_dsl(args[1], context, state)
    output_component_type = getproperty(Base, args[2])

    # Warn the user about redundant conversions.
    if field_component_type(input) == output_component_type
        @warn("Conversion from $output_component_type to itself: \"$(args[1]) => $(args[2])\"")
        return input
    end

    return ConversionField(input, output_component_type)
end
dsl_from_field(c::ConversionField) = :( $(dsl_from_field(c)) => $(field_component_type(c)) )