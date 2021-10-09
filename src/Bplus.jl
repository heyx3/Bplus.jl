module Bplus

"Loads the submodule."
macro submodule(name::Symbol)
    path = joinpath(string(name),
                    string(name, ".jl"))
    return quote
        include($path)
    end
end

@submodule Utilities
@submodule Math

# @submodule GL

end # module
