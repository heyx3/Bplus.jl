module Bplus

"Loads and exports the given submodule, using the path 'name/name.jl'"
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

@submodule Fields
@submodule Input
@submodule Helpers
@submodule GUI
@submodule SceneTree

end # module
