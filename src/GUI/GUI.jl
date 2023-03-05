"Implementation of a Dear IMGUI renderer/controller within B+, plus various helpers."
module GUI

    using Dates
    using CImGui, GLFW, CSyntax, StructTypes
    using ..Utilities, ..Math, ..GL

    # Define @bp_gui_assert and related stuff.
    @make_toggleable_asserts bp_gui_

    include("aliases.jl")
    include("gui_service.jl")
    include("simple_helpers.jl")
    include("file_dialog.jl")

end # module