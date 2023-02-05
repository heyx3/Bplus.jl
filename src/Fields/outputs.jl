"Fills an array by sampling the given field."
function sample_field!( array::Array{Vec{NOut, F}, NIn},
                        field::TField
                        ;
                        use_threading::Bool = true,
                        array_bounds::Box{Vec{NIn, UInt}} = Box_minsize(
                            one(Vec{NIn, UInt}),
                            convert(Vec{NIn, UInt}, vsize(array))
                        ),
                        sample_space::Box{Vec{NIn, F}} = Box_minmax(
                            zero(Vec{NIn, F}),
                            one(Vec{NIn, F})
                        )
                      ) where {NIn, NOut, F, TField<:AbstractField{NIn, NOut, F}}
    prep_data = prepare_field(field)

    # Calculate field positions.
    HALF = one(F) / F(2)
    texel = one(F) / vsize(array)
    @inline grid_pos_to_sample_pos(pos_component::Integer, axis::Int) = lerp(
        sample_space.min[axis],
        max_inclusive(sample_space)[axis],
        (pos_component + HALF) * texel[axis]
    )

    # If threading is enabled, each Z slice will be run as its own Task.
    function process_slice(z::UInt, zF::F)
        for y in array_bounds.min.y:max_inclusive(array_bounds).y
            yF::F = grid_pos_to_sample_pos(y, 2)
            for x in array_bounds.min.x:max_inclusive(array_bounds).x
                xF::F = grid_pos_to_sample_pos(x, 1)
                array[Vec(x, y, z)] = get_field(field, Vec(xF, yF, zF), prep_data)
            end
        end
    end
    if use_threading
        Threads.@threads for z::UInt in array_bounds.min.z:max_inclusive(array_bounds).z
            zF::F = grid_pos_to_sample_pos(z, 3)
            process_slice(z, zF)
        end
    else
        for z::UInt in array_bounds.min.z:max_inclusive(array_bounds).z
            zF::F = grid_pos_to_sample_pos(z, 3)
            process_slice(z, zF)
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

#TODO: Function to fill a GL.Texture with a field shader

export sample_field!, sample_field