################
##   Perlin   ##
################

"Perlin noise, in any number of dimensions."
struct PerlinField{ NIn, F,
                    TPos<:AbstractField{NIn, NIn, F},
                    TSeedLength,
                    TSeeds<:Tuple
                  } <: AbstractField{NIn, 1, F}
    pos::TPos
    seeds::TSeeds
end
function PerlinField( pos::AbstractField{NIn, NIn, F},
                      seeds = tuple()
                    ) where {NIn, F}
    return PerlinField{NIn, F, typeof(pos),
                       isnothing(seeds) ? 0 : 1,
                       typeof(seeds)}(
        pos, seeds
    )
end

prepare_field(p::PerlinField) = prepare_field(p.pos)

function get_field( p::PerlinField{NIn, F},
                    pos::Vec{NIn, F},
                    prep_data
                  ) where {NIn, F}
    noise_pos = get_field(p.pos, pos, prep_data)
    return Vec{1, F}(perlin(noise_pos, p.seeds))
end

# The DSL is a mostly-normal function, "perlin([pos expr])".
# However, you can pass any number of Real numbers as extra arguments,
#    which are used as seeds for the PRNG.
function field_from_dsl_func(::Val{:perlin}, context::DslContext, state::DslState, args::Tuple)
    pos_field = field_from_dsl(args[1], context, state)
    for (i, arg) in enumerate(args[2:end])
        @bp_check((arg isa Real) && isbits(arg),
                  "Seed #", i, " for perlin() call isn't a number literal: \"", arg, "\"")
    end
    return PerlinField(pos_field, tuple(args[2:end])...)
end
dsl_from_field(p::PerlinField) = :( perlin($(dsl_from_field(p.pos)), $(p.seeds...)) )