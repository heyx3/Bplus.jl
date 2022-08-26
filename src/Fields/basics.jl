###################
#  ConstantField  #
###################

"Outputs a constant value"
struct ConstantField{NIn, NOut, F} <: AbstractField{NIn, NOut, F}
    value::Vec{NOut, F}
end
export ConstantField
ConstantField{NIn}(v::Vec{NOut, F}) where {NIn, NOut, F} = ConstantField{NIn, NOut, F}(v)

"
Creates a constant field matching the output type of another field,
    with all components set to the given value(s).
"
ConstantField(template::AbstractField{NIn, NOut, F}, value::Real) where {NIn, NOut, F} = ConstantField{NIn, NOut, F}(Vec(i -> convert(F, value), Val(NOut)))
ConstantField(template::AbstractField{NIn, NOut, F}, value::Vec{NOut}) where {NIn, NOut, F} = ConstantField{NIn, NOut, F}(convert(Vec{NOut, F}, value))


@inline get_field(f::ConstantField{NIn, NOut, F}, pos::Vec{NIn, F}, ::Nothing) where {NIn, NOut, F} = f.value
@inline get_field_gradient(f::ConstantField{NIn, NOut, F}, pos::Vec{NIn, F}, ::Nothing) where {NIn, NOut, F} = zero(GradientType(f))

# In the DSL, constant 1D fields can be created with a number literal.
# AppendField then allows you to create constants of higher dimensions.
function field_from_dsl(value::Real, context::DslContext, state::DslState)
    return ConstantField{context.n_in}(Vec{1, context.float_type}(value))
end
function dsl_from_field(c::ConstantField{NIn, NOut, F}) where {NIn, NOut, F}
    if NOut == 1
        return c.value.x
    else
        component_constants = map(x -> ConstantField{NIn}(Vec(x)), c.value.data)
        return dsl_from_field(AppendField(component_constants...))
    end
end


##############
#  PosField  #
##############

"Outputs the input position"
struct PosField{N, F} <: AbstractField{N, N, F} end
export PosField
@inline get_field(::PosField{N, F}, pos::Vec{N, F}, ::Nothing) where {N, F} = pos
function get_field_gradient(::PosField{N, F}, ::Vec{N, F}, ::Nothing) where {N, F}
    # Along each axis, the rate of change is 1 for that axis and 0 for the others.
    return Vec{N, Vec{N, F}}() do axis::Int
        derivative = zero(Vec{N, F})
        @set! derivative[axis] = one(F)
        return derivative
    end
end

# The pos field is represented in the DSL by the special constant "pos".
const POS_FIELD_NAME = :pos
push!(RESERVED_VAR_NAMES, POS_FIELD_NAME)
field_from_dsl_var(::Val{POS_FIELD_NAME}, context::DslContext, ::DslState) = PosField{context.n_in, context.float_type}()
dsl_from_field(::PosField) = POS_FIELD_NAME


##################
#  TextureField  #
##################

@bp_enum(SampleModes, nearest, linear, cubic)

"Samples from a 'texture', in UV space (0-1)"
struct TextureField{NIn, NOut, F,
                    TArray<:AbstractArray{Vec{NOut, F}, NIn},
                    WrapMode, SampleMode,
                    TPos<:AbstractField{NIn, NIn, F}
                   } <: AbstractField{NIn, NOut, F}
    pixels::TArray
    pos::TPos
end
function TextureField( pixels::AbstractArray{Vec{NOut, F}, NIn},
                       pos::AbstractField{NIn, NIn, F} = PosField{NIn}()
                       ;
                       wrapping::GL.E_WrapModes = WrapModes.repeat,
                       sampling::E_SampleModes = SampleModes.linear
                     ) where {NIn, NOut, F}
    return TextureField{NIn, NOut, F,
                        typeof(pixels), Val(wrapping), Val(sampling),
                        typeof(pos)
                       }(pixels, pos)
end

"Applies a TextureField's wrapping to a given component of a position"
function wrap_component( tf::TField,
                         x::F
                       )::F where {NIn, NOut, F, TArray, WrapMode, SampleMode,
                                   TField<:TextureField{NIn, NOut, F, TArray, Val{WrapMode}, Val{SampleMode}}}
    ZERO = zero(F)
    ONE = one(F)
    if WrapMode == GL.WrapModes.repeat
        x -= trunc(x)
        if x < ZERO
            x += ONE
        end
        return x
    elseif WrapMode == GL.WrapModes.mirrored_repeat
        x = abs(x)
        (fpart, ipart) = modf(x)

        is_odd::Bool = (Int(ipart) % 2) == 1
        if is_odd
            return 1 - fpart
        else
            return fpart
        end
    elseif WrapMode == GL.WrapModes.clamp
        return clamp(x, zero(F), prevfloat(one(F)))
    elseif WrapMode == GL.WrapModes.custom_border
        error("The 'custom border' wrap mode is not supported for fields")
    else
        error("Unhandled wrap mode: ", WrapMode)
    end
end
"Applies a TextureField's wrapping to a given component of a pixel index, assumed to be > 0"
function wrap_index_positive( tf::TextureField{NIn, NOut, F, TArray, Val{WrapMode}, Val{SampleMode}},
                              positive_x::Int,
                              range_max_inclusive::Int
                            )::Int where {NIn, NOut, F, TArray, WrapMode, SampleMode}
    @bp_fields_assert(positive_x > 0)

    if WrapMode == GL.WrapModes.repeat
        return mod1(positive_x, pixels_length)
    elseif WrapMode == GL.WrapModes.mirrored_repeat
        index::Int = mod1(positive_x, range_max_inclusive)

        counter::Int = (positive_x - 1) ÷ range_max_inclusive
        is_odd::Bool = (counter % 2) == 1
        if is_odd
            index = range_max_inclusive - index + 1
        end

        return index
    elseif WrapMode == GL.WrapModes.clamp
        return clamp(x, zero(F), prevfloat(one(F)))
    elseif WrapMode == GL.WrapModes.custom_border
        error("The 'custom border' wrap mode is not supported for fields")
    else
        error("Unhandled wrap mode: ", WrapMode)
    end
end

"Recursively performs linear sampling across a specific axis"
function linear_sample_axis( tf::TextureField{NIn, NOut, F, TArray, Val{WrapMode}, Val{SampleMode}},
                             t::Vec{NIn, F},
                             i::Vec{NIn, Int},
                             ::Val{Axis} = Val(NIn)
                           )::Vec{NOut, F} where {NIn, NOut, F, TArray, WrapMode, SampleMode, Axis}
    @bp_fields_assert(Axis > 0, "Bad Axis: $Axis")

    i2 = i
    @set! i2[Axis] = wrap_index_positive(tf, i2[Axis] + 1, size(tf.pixels, Axis))

    local a::Vec{NIn, F},
          b::Vec{NIn, F}
    if Axis == 1
        if @bp_fields_debug()
            a = tf.pixels[i...]
            b = tf.pixels[i2...]
        else
            a = @inbounds tf.pixels[i...]
            b = @inbounds tf.pixels[i2...]
        end
    else
        a = linear_sample_axis(tf, t, i, Val(Axis - 1))
        b = linear_sample_axis(tf, t, i2, Val(Axis - 1))
    end

    return lerp(a, b, t[Axis])
end

function get_field( tf::TextureField{NIn, NOut, F, TArray, Val{WrapMode}, Val{SampleMode}},
                    pos::Vec{NIn, F},
                    ::Nothing
                  )::Vec{NOut, F} where {NIn, NOut, F, TArray, WrapMode, SampleMode}
    pos = map(x -> wrap_component(tf, x), pos)

    pixelF = pos * texel
    pixelI = VecI{NIn}(pixelF)
    pixelT = pixelF - pixelI

    if SampleMode == SampleModes.nearest
        return @bp_fields_debug() ?
                   tf.pixels[pixel...] :
                   @inbounds(tf.pixels[pixel])
    elseif SampleMode == SampleModes.linear
        return linear_sample_axis(tf, pixelT, pixelI)
    #TODO: Implement cubic sampling, using Cubic Lagrange: https://www.shadertoy.com/view/MllSzX
    else
        error("Unhandled sample mode: ", SampleMode)
    end
end
#TODO: Implement an efficient derivative calculation

# TextureFields have special treatment.
# The DslState has a lookup providing a name for each allocated array.
# The syntax "a{b}" is used to look up array "a" with UV "b".
# You can also provide wrapping and sampling arguments, e.x.
#    "a{b, wrap=repeat, sampling=linear}".
function field_from_dsl_expr(::Val{:curly}, ast::Expr, context::DslContext, state::DslState)
    @bp_fields_assert(length(ast.args) == 2, "Weird AST: $ast")
    @bp_check((ast.args[1] isa Symbol) && haskey(state.arrays, ast.args[1]),
              "TextureField access should refer to one of the names: ",
                  join(("'$name'" for (name, _) in state.arrays), ", "),
                  ". Got: '", ast, "'")
    @bp_check(length(ast.args) in 2:4,
              "Expected one input to TextureField accessor, plus up to 2 optional args: '",
                 ast, "'")

    uv_field = field_from_dsl(ast.args[2], context, state)

    # Read extra arguments.
    wrapping::E_WrapModes = WrapModes.repeat
    sampling::E_SampleModes = SampleModes.linear
    for extra_arg in ast.args[3:end]
        @bp_check(Meta.isexpr(extra_arg, :(=)) && all(a -> isa(a, Symbol), extra_arg.args),
                  "Extra arguments should look like 'wrap=[value]', or 'sampling=[value]'. Got: $extra_arg")
        if extra_arg.args[1] == :wrap
            wrapping = parse(E_WrapModes, string(extra_arg.args[2]))
        elseif extra_arg.args[1] == :sampling
            sampling = parse(E_SampleModes, string(extra_arg.args[2]))
        else
            error("Unknown texture parameter '", extra_arg.args[1], "' in statement: ", extra_arg)
        end
    end

    return TextureField(state.arrays[ast.args[1]],
                        uv_field
                        ;
                        wrapping = wrapping,
                        sampling = sampling)
end

export SampleModes, E_SampleModes, TextureField