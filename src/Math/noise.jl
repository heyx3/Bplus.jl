#TODO: Fancy Julia implementations of N-dimensional simplex and worley noise

@inline perlin(f::Real, args...) = perlin(@f32(f), args...)
@inline perlin(f::AbstractFloat, args...) = perlin(Vec(f), args...)
@generated function perlin( v::Vec{N, T},
                            # Extra seed data to randomize the output.
                            seeds::TSeeds = tuple(0xabcd9166),
                            # With enough seed values, you can get away with a weaker PRNG
                            #    without creating artifacts.
                            prng_strength::Val{TPrngStrength} = Val(PrngStrength.medium),
                            # A function that filters the corner postions surrounding the input point.
                            # Use this to create wrapped or clamped noise.
                            filter_pos::TFuncFilter = identity,
                            # A mapping function applied to the noise interpolant.
                            # Smoother interpolants look better.
                            t_curve::TFuncCurve = smootherstep
                          )::T where {N, T, TFuncFilter, TFuncCurve, TPrngStrength, TSeeds<:Tuple}
    @bp_check((N isa Integer) && (N > 0),
              "Perlin noise must be done in a positive-dimensional space, but N == ", N)
    @bp_check(TPrngStrength isa E_PrngStrength,
              "PRNG strength parameter must be <: E_PrngStrength, but is ", typeof(TPrngStrength))

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
    expr_rng_seeds = [ ]
    for i in 1:N
        push!(expr_rng_seeds, :( pos_filtered[$i] ))
    end
    for i in 1:length(seeds.parameters)
        push!(expr_rng_seeds, :( seeds[$i] ))
    end
    expr_make_gradient = quote end
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
        #TODO: This isn't a uniform distribution, it biases towards the corners. However, in practice it looks quite good in 2D...
        #TODO: See if we can get away with not normalizing the gradients. This would increase the range of possible output values.
        vnorm(lerp(-1, 1, TVec($(gradient_component_names...))))
    ))
    # Generate code that combines the gradient with the input position to get a noise value.
    # Define local variables for each corner's noise, named 'corner_noise_[X]'.
    for i in 1:n_corners
        corner_idx = all_corners_ordered[i]

        # First, generate the expression that gets each corner position's components:
        #    TVec(j -> v_minmax[corner_idx[j]])
        # You can't actually have closures in @generated functions,
        #    so build the expression manually for each component.
        corner_pos_components::Vector{Expr} = map(1:N) do i::Int
            return :( v_minmax[$(corner_idx[i])][$i] )
        end
        filtered_corner_pos_components::Vector{Expr} = map(1:N) do i::Int
            return :( v_minmax_filtered[$(corner_idx[i])][$i] )
        end

        push!(output.args, :(
            $(Symbol(:corner_noise_, i))::T = let pos_raw = TVec($(corner_pos_components...))
                pos_filtered = $(if TFuncFilter == typeof(identity)
                                    :pos_raw
                                else
                                    :( TVec($(filtered_corner_pos_components...)) )
                                end)

                # Get the vectors influencing the noise at this corner.
                delta::TVec = pos_raw - v
                gradient::TVec = let rng = ConstPRNG(prng_strength, $(expr_rng_seeds...))
                    $expr_make_gradient
                end

                # Compute and output the noise.
                vdot(delta, gradient)
            end
        ))
    end

    # Start interpolating between the corners to get the final noise value.
    push!(output.args, :( t::TVec = t_curve(v - v_min) ))
    # Each pair of corners are adjacent along the X axis.
    # Interpolate the noise along each X edge, to get N/2 noise values.
    # Next, interpolate the noise along each Y face, to get N/4 noise values.
    # Repeat for each dimension, leaving us with the final noise value.
    for dimension::Int in 1:N
        # Take the previous dimension's results and interpolate pairs of them
        #    into some new local variables.
        prev_name = (dimension == 1) ?
                        :corner_noise_ :
                        Symbol(:corner_noise_, dimension - 1, :_)
        next_name = Symbol(:corner_noise_, dimension, :_)

        n_output_values::Int = n_corners ÷ (2^dimension)
        for i in 1:n_output_values
            prev_i::Int = ((i - 1) * 2) + 1
            push!(output.args, :(
                $(Symbol(next_name, i))::T = lerp($(Symbol(prev_name, prev_i)),
                                                  $(Symbol(prev_name, prev_i+1)),
                                                  t[$dimension])
            ))
        end
    end
    final_name::Symbol = Symbol(:corner_noise_, N, :_1)

    # The range of perlin noise is -x to +x, where x is a value less than 1
    #    and proportional to the number of dimensions.
    # We want the output to be in the 0-1 range.
    # References:
    #    https://digitalfreepen.com/2017/06/20/range-perlin-noise.html
    #    https://stackoverflow.com/a/18263038
    #    https://www.gamedev.net/forums/topic/285533-2d-perlin-noise-gradient-noise-range--/
    max_output::T = convert(T, sqrt(N) / 2)
    push!(output.args, quote
        result::T = $final_name
        result = inv_lerp(-$max_output, $max_output, result)
        result = clamp(result, zero(T), one(T))
        return result
    end)

    #@async println(output)
    return output
end

export perlin