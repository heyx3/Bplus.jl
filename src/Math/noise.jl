#TODO: Fancy Julia implementations of N-dimensional perlin and worley noise

"
1D perlin noise.
For fastest performance, pass 64 bits of data in for the seeds.
"
function perlin_fast( f::Vec{1, T},
                      seeds...;
                      t_curve::F = smoothstep
                    )::T where {F, T<:AbstractFloat}
    ONE = one(Vec{1, T})
    TWO = Vec{1, T}(2)

    # Calculate the interval this value falls inside.
    f_min = floor(f)
    f_max = t_min + ONE
    t = f - f_min

    # Assign a random 1D vector to each end of the interval,
    #    as well as a vector pointing from the input to that end.
    (v_min, _) = rand(ConstPRNG(f_min, seeds), T)
    v_min = -ONE + (TWO * v_min)
    dir_min = -t
    (v_max, _) = rand(ConstPRNG(f_max, seeds), T)
    v_max = -ONE + (TWO * v_max)
    dir_max = ONE - t

    # Interpolate between the gradients.
    t = t_curve(t)
    out::T = lerp(vdot(v_min, dir_min),
                  vdot(v_max, dir_max),
                  t)
    return out
end
@inline perlin_fast(f::T; kw...) where {T<:Number} = perlin_fast(Vec(f); kw...).x

#TODO: perlin_fast 2D and 3D version, compare performance to the @generated perlin()


function perlin_sample( v::Vec{2, T}
                 ;
                 seeds = fill_in_seeds(Vec{2, T}),
                 t_curve::FCurve = smoothstep,
                 pos_filter::FPosFilter = identity
               )::T where {FCurve, FPosFilter, T<:Union{AbstractFloat, Vec{2, <:AbstractFloat}}}
    ONE = one(Vec{2, T})
    TWO = Vec{2, T}(Val(2))

    # Calculate the square this value falls inside.
    offsets = ( (Vec(1, 1), Vec(2, 1)),
                (Vec(1, 2), Vec(2, 2))
              )
    v_min = floor(v)
    v_minmax = (v_min, v_min + one(Vec{2, T}))
    v_minmax_filtered = map(pos_filter, v_minmax)

    # Map each corner of the square to its noise value,
    #    calculated based on a random gradient vector and the direction towards that corner.
    corner_values = map(offsets) do offsets_row
        return map(offsets_row) do off::Vec{2, Int}
            pos = Vec{2, T}(i -> v_minmax[off[i]])
            pos_filtered = (typeof(FPosFilter) == typeof(identity)) ?
                               pos :
                               Vec{2, T}(i -> v_minmax_filtered[off[i]])

            delta = pos - v

            rng = ConstPRNG(pos_filtered..., seeds...)
            (gradient_x, rng) = rand(rng, T)
            (gradient_y, rng) = rand(rng, T)
            gradient = ONE + (TWO * Vec(gradient_x, gradient_y))

            return vdot(gradient, delta)
        end
    end

    # Interpolate between the gradients based on the input position.
    t = t_curve(v - v_min)
    results_along_x = ntuple(2) do y
        return lerp(dot(corner_values[y][1]),
                    dot(corner_values[y][2]),
                    t.x)
    end
    return lerp(results_along_x..., t.y)
end
function perlin_sample( v::Vec{3, T}
                 ;
                 seeds = fill_in_seeds(Vec{3, T}),
                 t_curve::FCurve = smoothstep,
                 pos_filter::FPosFilter = identity
               )::T where {FCurve, FPosFilter, T<:Union{AbstractFloat, Vec{3, <:AbstractFloat}}}
    ONE = one(Vec{3, T})
    TWO = Vec{3, T}(Val(2))

    # Calculate the cube corners this value falls inside.
    offsets = (
        (
            (Vec(1, 1, 1), Vec(2, 1, 1)),
            (Vec(1, 2, 1), Vec(2, 2, 1))
        ),
        (
            (Vec(1, 1, 2), Vec(2, 1, 2)),
            (Vec(1, 2, 2), Vec(2, 2, 2))
        )
    )
    v_min = floor(v)
    v_minmax = (v_min, v_min + one(Vec{3, T}))
    v_minmax_filtered = map(pos_filter, v_minmax)

    # Map each corner of the cube to its noise value,
    #    calculated based on a random gradient vector and the direction towards that corner.
    corner_values = map(offsets) do offsets_z_slice
        map(offsets_z_slice) do offsets_y_line
            map(offsets_y_line) do off::Vec{3, Int}
                pos = Vec{3, T}(i -> v_minmax[off[i]])
                pos_filtered = (typeof(FPosFilter) == typeof(identity)) ?
                                pos :
                                Vec{3, T}(i -> v_minmax_filtered[off[i]])

                delta = pos - v

                rng = ConstPRNG(pos_filtered..., seeds...)
                (gradient_x, rng) = rand(rng, T)
                (gradient_y, rng) = rand(rng, T)
                (gradient_z, rng) = rand(rng, T)
                gradient = ONE + (TWO * Vec(gradient_x, gradient_y, gradient_z))

                return vdot(gradient, delta)
            end
        end
    end

    # Interpolate between the gradients based on the input position.
    t = t_curve(v - v_min)
    results_along_y = ntuple(2) do z
        results_along_x = ntuple(2) do y
            lerp(corner_values[z][y]..., t[1])
        end
        return lerp(results_along_x..., t[2])
    end
    return lerp(results_along_y..., t.z)
end

@generated function perlin( v::Vec{N, T};
                            # Extra seed data to randomize the output.
                            seeds::TSeeds = fill_in_seeds(Vec{N, T}),
                            # A function that filters the corner postions surrounding the input point.
                            # Use this to create wrapped or clamped noise.
                            filter_pos::TFuncFilter = identity,
                            # A mapping function applied to the noise interpolant.
                            # Smoother interpolants look better.
                            t_curve::TFuncCurve = smootherstep
                          )::T where {N, T, TFuncFilter, TFuncCurve, TSeeds<:Tuple}
    @bp_check((N isa Integer) && (N > 0),
              "Perlin noise must be done in a positive-dimensional space, but N == ", N)

    TVec = Vec{N, T}
    output::Expr = quote
        ONE = one(TVec)
        TWO = TVec(Val(2))
    end

    # Calculate the cube corners the value falls inside.
    # Each corner is initially stored as a Vec of 1 ("min") or 2 ("max") for each axis.
    minmax_indices = 1:2
    all_corners_ordered = Iterators.product(ntuple(i -> minmax_indices, N)...)
    all_corners_ordered = tuple(Iterators.map(Vec, all_corners_ordered)...)
    # Pair up corners across the X axis,
    #    then pair up those edges across the Y axis,
    #    then pair up those faces across the Z axis,
    #    etc.
    # For example, in 3-dimensions, 'all_corners_ordered' would become
    #    a 2-tuple of (2-tuple of (2-tuple of v3i)).
    # In 1 dimension, it would become a 2-tuple of Vec{1, Int}.
    if false
        for _ in 2:N
            grouped = Iterators.partition(all_corners_ordered, 2)
            # The partitions are Vectors; convert them to tuples.
            all_corners_ordered = tuple((tuple(data...) for data::Vector in grouped)...)
        end
    end
    push!(output.args, quote
        CORNER_IDCS = $all_corners_ordered
        v_min = map(floor, v)
        v_minmax = (v_min, v_min + ONE)
        v_minmax_filtered = $(
            if TFuncFilter == typeof(identity)
                :v_minmax
            else
                :( map(filter_pos, v_minmax) )
            end
        )
    end)

    # Map each corner of the cube to its noise value,
    #    calculated based on the direction towards that corner and a random gradient.
    expr_make_gradient = quote
        rng = ConstPRNG(pos_filtered..., seeds...)
    end
    for i in 1:N
        push!(expr_make_gradient.args, :(
            ($(Symbol(:gradient_, i)), rng) = rand(rng, T)
        ))
    end
    push!(expr_make_gradient.args,
          Meta.parse(string("gradient::\$TVec = \$TVec(",
                            join(map(i -> "gradient_$i", 1:N), ", "),
                            ")")
                    ))
    push!(output.args, quote
        corner_noise::NTuple{$(length(all_corners_ordered)), Float64} = ntuple(Val(N^2)) do i::Int
            corner_idx = CORNER_IDCS[i]
            pos_raw = $TVec(j -> v_minmax[corner_idx[j]])
            pos_filtered = $((TFuncFilter == typeof(identity)) ?
                                 :pos_raw :
                                 :( $TVec(i -> v_minmax_filtered[corner_idx[i]]) ))

            # Get the vectors influencing the noise at this corner.
            delta::$TVec = pos - v
            $expr_make_gradient

            return vdot(delta, gradient)
        end
    end)

    push!(output.args, :( t = t_curve(v - v_min) ))

    # Each pair of corners are adjacent along the X axis.
    # Compute the noise along each X edge, and put those N/2 values at the front of the tuple.
    # Repeat for each dimension, which should leave us with a 1-tuple containing the answer.
    for dimension::Int in 1:N
        for i in 1:(length(all_corners_ordered) ÷ (2^dimension))
            push!(output.args, quote
                @set! corner_noise[$i] = lerp(corner_noise[$i],
                                              corner_noise[$(i+1)],
                                              t[dimension])
            end)
        end
    end

    push!(output.args, :( return corner_noise[1] ))

    return output
end

export perlin