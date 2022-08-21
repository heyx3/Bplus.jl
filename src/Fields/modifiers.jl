###################
#  Swizzle field  #
###################

#TODO: Swizzle should allow more than xyzw components, reference as indices instead

"Swaps the components of a field's output"
struct SwizzleField{NIn, NOut, F, Swizzle, TField} <: AbstractField{NIn, NOut, F}
    field::TField
end
export SwizzleField

function SwizzleField( field::AbstractField{NIn, NOutSrc, F},
                             swizzle::Symbol
                           ) where {NIn, NOutSrc, F}
    swizzle_str = string(swizzle)
    NOutDest = length(swizzle_str)

    return SwizzleField{NIn, NOutDest, F, Val(swizzle), typeof(field)}(field)
end

prepare_field(f::SwizzleField{NIn}) where {NIn} = prepare_field(f.field)

@inline function get_field( f::SwizzleField{NIn, NOut, F, Swizzle, TField},
                            pos::Vec{NIn, F},
                            prepared_data
                          )::Vec{NOut, F} where {NIn, NOut, F, Swizzle, TField}
    input = get_field(f.field, pos, prepared_data)
    return Math.swizzle(input, Swizzle)
end
@inline function get_field_gradient( f::SwizzleField{NIn, NOut, F, Swizzle, TField},
                                     pos::Vec{NIn, F},
                                     prepared_data
                                   ) where {NIn, NOut, F, TField, Swizzle}
    input = get_field_gradient(f.field, pos, prepared_data)
    # The return value has one field output per input axis.
    # Each of these outputs must be swizzled.
    return map(v -> Math.swizzle(v, Swizzle), input)
end

####################
#  Gradient field  #
####################

"""
Samples the gradient of a field, in a given direction.
The direction can be an `Integer` representing the position axis,
    or a vector (`AbstractField{NIn, NIn, F}`) that is dotted with the gradient.
"""
struct GradientField{ NIn, NOut, F,
                      TField<:AbstractField{NIn, NOut, F},
                      TDir<:Union{Integer, AbstractField{NIn, NIn, F}}
                    } <: AbstractField{NIn, NOut, F}
    field::TField
    dir::TDir
end
export GradientField

prepare_field(g::GradientField{NIn, NOut, F}) where {NIn, NOut, F} = prepare_field.((g.field, g.dir))
function get_field( g::GradientField{NIn, NOut, F, TField, TDir},
                    pos::Vec{NIn, F},
                    prep_data::Tuple = (nothing, nothing)
                  ) where {NIn, NOut, F, TField, TDir}
    gradient::Vec{NIn, Vec{NOut, F}} = get_field_gradient(g.field, pos)
    if TDir <: Integer
        return gradient[g.dir]
    elseif TDir <: AbstractField{NIn, NIn, F}
        dir::Vec{NIn, F} = get_field(g.dir, pos, prep_data[2])
        output_per_input_axis = gradient * dir
        return reduce(+, output_per_input_axis)
    else
        error("Unexpected type for GradientField's direction input: ", TDir)
    end
end
#TODO: Gradient shouldn't be too too hard to work out analytically, as the value is made up of a few simple operations


#TODO: Transformations