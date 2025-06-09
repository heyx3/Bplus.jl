module Bplus

using BplusCore, BplusApp, BplusTools
@using_bplus_core

#TODO: Each sub-module should list its own important modules for B+ to automatically consume.
const SUB_MODULES = [
    BplusCore => [:Utilities, :Math ],
    BplusApp => [ :GL, :GUI, :Input, :ModernGLbp ],
    BplusTools => [ :ECS, :Fields, :SceneTree ]
]

for (sub_module, features) in SUB_MODULES
    for feature_name in features
        @eval const $feature_name = $sub_module.$feature_name
        @eval export $feature_name
    end
end

macro using_bplus()
    # Import the modules as if they came directly from B+.
    using_statements = [ ]
    for (sub_module, features) in SUB_MODULES
        push!(using_statements, :( using Bplus.$(Symbol(sub_module)) ))
        for feature_name in features
            push!(using_statements, :( using Bplus.$feature_name ))
        end
    end
    return quote
        $(using_statements...)
    end
end
export @using_bplus

end # module
