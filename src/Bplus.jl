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
@submodule SceneTree
@submodule ECS
@submodule Fields

@submodule Input
@submodule GUI

@submodule Helpers


"
Loads all B+ modules with `using` statements.
You can import all of B+ with two lines:
````
using Bplus
@using_bplus
````
"
macro using_bplus()
    return :(
        using Bplus.Utilities,
              Bplus.Math, Bplus.GL, Bplus.SceneTree,
              Bplus.Input, Bplus.GUI, Bplus.Helpers,
              Bplus.Fields, Bplus.ECS
    )
end
export @using_bplus

end # module
