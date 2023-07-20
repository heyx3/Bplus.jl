"Fills an array by sampling the given field."
function sample_field!( array::Array{Vec{NOut, F}, NIn},
                        field::TField
                        ;
                        use_threading::Bool = true,
                        array_bounds::Box{NIn, UInt} = Box(
                            min = one(Vec{NIn, UInt}),
                            size = convert(Vec{NIn, UInt}, vsize(array))
                        ),
                        sample_space::Box{NIn, F} = Box(
                            min = zero(Vec{NIn, F}),
                            max = one(Vec{NIn, F})
                        )
                      ) where {NIn, NOut, F, TField<:AbstractField{NIn, NOut, F}}
    prep_data = prepare_field(field)

    # Calculate field positions.
    HALF = F(1) / F(2)
    texel = F(1) / vsize(array)
    @inline grid_pos_to_sample_pos(pos_component::Integer, axis::Int) = lerp(
        min_inclusive(sample_space)[axis],
        max_inclusive(sample_space)[axis],
        inv_lerp(min_inclusive(array_bounds)[axis],
                 max_inclusive(array_bounds)[axis],
                 pos_component + HALF)
    )

    # If threading is enabled, each slice along the last axis will be run as its own Task.
    b_min = min_inclusive(array_bounds)
    b_max = max_inclusive(array_bounds)
    b_min_slice = Vec(i -> b_min[i], Val(NIn - 1))
    b_max_slice = Vec(i -> b_max[i], Val(NIn - 1))
    function process_slice(i::UInt)
        for slice_pos in b_min_slice:b_max_slice
            posI = vappend(map(UInt, slice_pos), i)
            posF = Vec(c -> grid_pos_to_sample_pos(posI[c], c), Val(NIn))
            array[posI] = get_field(field, posF, prep_data)
        end
        return nothing
    end
    if use_threading
        Threads.@threads for i::UInt in b_min[NIn] : b_max[NIn]
            process_slice(i)
        end
    else
        for i::UInt in b_min[NIn] : b_max[NIn]
            process_slice(i)
        end
    end

    return nothing
end
"
Creates and fills a grid using the given field.
Optional arguments are the same as `sample_field!()`.
"
function sample_field( grid_size::Vec{NIn, <:Integer},
                       field::AbstractField{NIn, NOut, F}
                       ;
                       kw...
                     )::Array{Vec{NOut, F}, NIn} where {NIn, NOut, F}
    output = Array{Vec{NOut, F}, NIn}(undef, grid_size.data)
    sample_field!(output, field; kw...)
    return output
end

export sample_field!, sample_field