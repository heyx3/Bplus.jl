######################
#  Swizzling inputs  #
######################

"Swaps the components of an input position before passing it to another field"
struct SwizzleInputField{NIn, NOut, F, Swizzle, TField} <: AbstractField{NIn, NOut, F}
    field::TField
end
export SwizzleInputField

# Swizzling constructor:
@inline function SwizzleInputField{NInSrc}( field::AbstractField{NInDest, NOut, F},
                                            swizzle::Symbol
                                          ) where {NInSrc, NInDest, NOut, F}
    swizzle_str = string(swizzle)
    @bp_check(NInDest == length(swizzle_str),
              "Input position swizzle '", swizzle, "' would map the input to a ",
                 Vec{length(swizzle_str), F},
                 ", but this doesn't match the expected input to field ",
                 typeof(field), ": ", Vec{NInDest, F})

    return SwizzleInputField{NInSrc, NOut, F, Val(swizzle)}(field)
end

@inline function apply_swizzle( f::SwizzleInputField{NIn, NOut, F, Val{Swizzle}},
                                v::Vec{NIn}
                              ) where {NIn, NOut, F, Swizzle}
    return Bplus.Math.swizzle(v, Swizzle)
end

prepare_field(f::SwizzleInputField{NIn}, grid_size::Vec{NIn, Int}) where {NIn} = prepare_field(f.field, apply_swizzle(tf, grid_size))

@inline function get_field( f::SwizzleInputField{NIn, NOut, F, Swizzle},
                            pos::Vec{NIn, F},
                            prepared_data = nothing
                          )::Vec{NOut, F} where {NIn, NOut, F, Swizzle}
    return get_field(f.field, apply_swizzle(f, pos), prepared_data)
end
@inline function get_field_gradient( f::SwizzleInputField{NIn, NOut, F, TField, Swizzle},
                                     pos::Vec{NIn, F},
                                     prepared_data = nothing
                                   ) where {NIn, NOut, F, TField, Swizzle}
    return get_field_gradient(f.field, apply_swizzle(tf, pos), prepared_data)
end


#######################
#  Swizzling outputs  #
#######################

"Swaps the components of a field's output"
struct SwizzleOutputField{NIn, NOut, F, Swizzle, TField} <: AbstractField{NIn, NOut, F}
    field::TField
end
export SwizzleOutputField

function SwizzleOutputField( field::AbstractField{NIn, NOutSrc, F},
                             swizzle::Symbol
                           ) where {NIn, NOutSrc, F}
    swizzle_str = string(swizzle)
    NOutDest = length(swizzle_str)

    return SwizzleOutputField{NIn, NOutDest, F, Val(swizzle)}(field)
end

prepare_field(f::SwizzleOutputField{NIn}, grid_size::Vec{NIn, Int}) where {NIn} = prepare_field(f.field, grid_size)

@inline function get_field( f::SwizzleOutputField{NIn, NOut, F, Swizzle, TField},
                            pos::Vec{NIn, F},
                            prepared_data = nothing
                          )::Vec{NOut, F} where {NIn, NOut, F, TField, Swizzle}
    input = get_field(f.field, pos, prepared_data)
    return Bplus.Math.swizzle(input, Swizzle)
end
@inline function get_field_gradient( f::SwizzleOutputField{NIn, NOut, F, Swizzle, TField},
                                     pos::Vec{NIn, F},
                                     prepared_data = nothing
                                   ) where {NIn, NOut, F, TField, Swizzle}
    input = get_field_gradient(f.field, pos, prepared_data)
    # The return value has one field output per input axis.
    # Each of these outputs must be swizzled.
    return map(v -> Bplus.Math.swizzle(v, Swizzle), input)
end