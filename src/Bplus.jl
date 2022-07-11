module Bplus

include("check_dependencies.jl")

"Loads the given submodule, at the path 'name/name.jl'"
macro submodule(name::Symbol)
    path = joinpath(string(name),
                    string(name, ".jl"))
    return quote
        include($path)
        # Enforce the naming conventions.
        @assert(isdefined(@__MODULE__, Symbol($(string(name)))))
    end
end

@submodule Utilities
@submodule Math

@submodule GL

@submodule Input
@submodule Helpers
@submodule SceneTree

end # module