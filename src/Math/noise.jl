#TODO: Fancy Julia implementations of N-dimensional simplex and worley noise

using StaticArrays


function perlin_fast( v::Vec{2, T}
                        ;
                        seeds = fill_in_seeds(Vec{2, T}),
                        t_curve::FCurve = smootherstep,
                        pos_filter::FPosFilter = identity
                      )::T where {FCurve, FPosFilter, T<:Union{AbstractFloat, Vec{2, <:AbstractFloat}}}
    ONE = one(Vec{2, T})
    TWO = Vec{2, T}(Val(2))

    # Calculate the square this value falls inside.
    v_min = map(floor, v)
    v_minmax = (v_min, v_min + one(Vec{2, T}))
    v_minmax_filtered = map(pos_filter, v_minmax)

    noise_min = vdot(
        let rng = ConstPRNG(v_minmax_filtered[1]..., seeds...)
            (g_x, rng) = rand(rng, T)
            (g_y, rng) = rand(rng, T)
            ONE + (TWO * Vec(g_x, g_y))
        end,
        v_minmax[1] - v
    )
    noise_minXmaxY = vdot(
        let rng = ConstPRNG(v_minmax_filtered[1].x, v_minmax_filtered[2].y, seeds...)
            (g_x, rng) = rand(rng, T)
            (g_y, rng) = rand(rng, T)
            ONE + (TWO * Vec(g_x, g_y))
        end,
        Vec(v_minmax[1].x, v_minmax[2].y) - v
    )
    noise_maxXminY = vdot(
        let rng = ConstPRNG(v_minmax_filtered[2].x, v_minmax_filtered[1].y, seeds...)
            (g_x, rng) = rand(rng, T)
            (g_y, rng) = rand(rng, T)
            ONE + (TWO * Vec(g_x, g_y))
        end,
        Vec(v_minmax[2].x, v_minmax[1].y) - v
    )
    noise_max = vdot(
        let rng = ConstPRNG(v_minmax_filtered[2]..., seeds...)
            (g_x, rng) = rand(rng, T)
            (g_y, rng) = rand(rng, T)
            ONE + (TWO * Vec(g_x, g_y))
        end,
        v_minmax[2] - v
    )

    t = t_curve(v - v_min)
    output = lerp(lerp(noise_min, noise_maxXminY, t.x),
                  lerp(noise_minXmaxY, noise_max, t.x),
                  t.y)
    output = inv_lerp(-0.70710678118, 0.70710678118, output)
    output = clamp(output, 0, 1)
    return output
end
export perlin_fast

@inline perlin(f::Real; kw...) = perlin(@f32(f), kw...)
@inline perlin(f::AbstractFloat; kw...) = perlin(Vec(f); kw...)
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
        TVec = Vec{N, T}
        ONE = one(TVec)
        TWO = TVec(Val(2))
    end

    # Calculate the cube corners the value falls inside.
    # Each corner is initially stored as a Vec of 1 ("min") or 2 ("max") for each axis.
    minmax_indices = 1:2
    all_corners_ordered = Iterators.product(ntuple(i -> minmax_indices, Val(N))...)
    all_corners_ordered = tuple(Iterators.map(Vec, all_corners_ordered)...)
    n_corners::Int = length(all_corners_ordered)
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
    # Generate code to compute the gradient, like:
    #     (gradient_x::T, rng) = rand(rng, T)
    #     (gradient_y::T, rng) = rand(rng, T)
    #     gradient::v2f = vnorm(-1 + (2 * v2f(gradient_x, gradient_y)))
    gradient_component_names = map(i -> Symbol(:gradient_, i), 1:N)
    for i in 1:N
        push!(expr_make_gradient.args, :(
            ($(gradient_component_names[i])::T, rng) = rand(rng, T)
        ))
    end
    push!(expr_make_gradient.args, :(
        #TODO: This isn't a uniform distribution, it biases towards the corners. However, in practice it looks quite good...
        gradient::TVec = vnorm(lerp(-1, 1, TVec($(gradient_component_names...))))
    ))
    # Generate code that combines the gradient with the input position to get a noise value.
    # First, generate the expression that gets each corner position's components:
    #    TVec(j -> v_minmax[corner_idx[j]])
    # You can't actually have closures in @generated functions,
    #    so build the expression manually for each component.
    corner_pos_components::Vector{Expr} = map(1:N) do i::Int
        return :( v_minmax[corner_idx[$i]][$i] )
    end
    filtered_corner_pos_components::Vector{Expr} = map(1:N) do i::Int
        return :( v_minmax_filtered[corner_idx[$i]][$i] )
    end
    push!(output.args, quote
        # It's easier to use a mutable vector to insert the values,
        #    then copy into an immutable vector,
        #    and trust the compiler to optimze.
        #TODO: Based on testing, we can expect a 2x speedup by removing the StaticArrays stuff and just using normal variables.
        #TODO: Instead of messing with StaticArrays, try directly writing N copies of the gradient/noise code. Profile the change in performance.
        corner_noise::SVector{$n_corners, T} =
            let output = MVector{$n_corners, T}(undef)
                for i::Int in 1:$n_corners
                    corner_idx = CORNER_IDCS[i]
                    pos_raw = TVec($(corner_pos_components...))
                    pos_filtered = $(if TFuncFilter == typeof(identity)
                                         :pos_raw
                                     else
                                         :( TVec($(filtered_corner_pos_components...)) )
                                     end)

                    # Get the vectors influencing the noise at this corner.
                    delta::TVec = pos_raw - v
                    $expr_make_gradient

                    output[i] = vdot(delta, gradient)
                end
                output
            end
    end)

    push!(output.args, :( t = t_curve(v - v_min) ))

    # Each pair of corners are adjacent along the X axis.
    # Interpolate the noise along each X edge, to get N/2 noise values.
    # Next, interpolate the noise along each Y face, to get N/4 noise values.
    # Repeat for each dimension, leaving us with the final noise value.
    for dimension::Int in 1:N
        # Take the previous dimension's results and interpolate pairs of them
        #    into a new results vector of half the size.
        prev_name = (dimension == 1) ?
                        :corner_noise :
                        Symbol(:corner_noise_, dimension - 1)
        next_name = Symbol(:corner_noise_, dimension)

        n_output_values::Int = n_corners ÷ (2^dimension)
        push!(output.args, :(
            # As mentioned above, it's much nicer to work with mutable data,
            #    and keeping the mutable data inside a "let" block should help the compiler
            #    put it on the stack instead of the heap.
            $next_name::SVector{$n_output_values, T} =
                let output = MVector{$n_output_values, T}(undef)
                    for i in 1:$n_output_values
                        prev_i::Int = ((i - 1) * 2) + 1
                        output[i] = lerp($prev_name[prev_i],
                                         $prev_name[prev_i + 1],
                                         t[$dimension])
                    end
                    output
                end
        ))
    end
    final_name::Symbol = Symbol(:corner_noise_, N)

    # The range of perlin noise is -x to +x, where x is a value less than 1
    #    and proportional to the number of dimensions.
    # We want the output to be in the 0-1 range.
    # References:
    #    https://digitalfreepen.com/2017/06/20/range-perlin-noise.html
    #    https://stackoverflow.com/a/18263038
    #    https://www.gamedev.net/forums/topic/285533-2d-perlin-noise-gradient-noise-range--/
    max_output::T = convert(T, sqrt(N) / 2)
    push!(output.args, quote
        result::T = $final_name[1]
        result = inv_lerp(-$max_output, $max_output, result)
        result = clamp(result, zero(T), one(T))
        return result
    end)

    #println(string(output))
    return output
end

export perlin