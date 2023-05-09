"Implementation of a Dear IMGUI renderer/controller within B+, plus various helpers."
module GUI

    using Dates, InteractiveUtils
    using CImGui, GLFW, CSyntax, StructTypes
    using ..Utilities, ..Math, ..GL

    # Define @bp_gui_assert and related stuff.
    @make_toggleable_asserts bp_gui_

    # De-centralize the use of __init__() across this module's files.
    const RUN_ON_INIT = [ ]
    function __init__()
        for f::Function in RUN_ON_INIT
            f()
        end
    end

    include("aliases.jl")
    include("gui_service.jl")
    include("simple_helpers.jl")
    include("file_dialog.jl")

end # module