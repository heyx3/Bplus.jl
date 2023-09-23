# Shows off some B+ GUI widgets, built off of Dear ImGUI.
# References to the B+ GUI module are explicit
#    to make the GUI code more understandable to new readers.

cd(joinpath(@__DIR__, ".."))
insert!(LOAD_PATH, 1, ".")
insert!(LOAD_PATH, 1, "test")

using Dates, Setfield
using ModernGL, GLFW, CImGui, CSyntax

using Bplus
using Bplus.GL, Bplus.Helpers, Bplus.Math, Bplus.Utilities


"The data that the GUI is editing."
Base.@kwdef mutable struct DemoGuiState
    float::Float32 = 0.5

    file_dialog_settings::FileDialogSettings = FileDialogSettings(
        directory=pwd(),
        file_name="test.txt",
        show_unselectable_options = true
    )
    file_dialog_params::FileDialogParams = FileDialogParams(
        is_writing = true,
        is_folder_mode = false #TODO: Also test folder mode.
    )
    file_dialog_state::FileDialogState = FileDialogState()
end

"The GUI code for the demo."
function demo_gui(service::Bplus.GUI.GuiService, state::DemoGuiState)
    # The details of Julia <=> C interop are a bit weird,
    #    but the CSyntax package lets us pretend it's easy with the @c macro.
    @c CImGui.SliderFloat("Slide me", &state.float, 0.5, 20.5)
    CImGui.TextColored(CImGui.LibCImGui.ImVec4(1, 0, 1, 1), "Ouch this magenta is ugly")

    Bplus.GUI.gui_within_fold("Click my arrow") do
        result = gui_file_dialog(state.file_dialog_settings,
                                 state.file_dialog_params,
                                 state.file_dialog_state)
        #TODO: Display the result in some way.
    end
end


bp_gl_context( v2i(800, 500), "GUI demo";
               vsync=VsyncModes.on,
               debug_mode=true
             ) do context::Context
    gui_service::Bplus.GUI.GuiService = Bplus.GUI.service_GUI_init(context)
    gui_state::DemoGuiState = DemoGuiState()

    while !GLFW.WindowShouldClose(context.window)
        window_size::v2i = get_window_size(context)

        Bplus.GUI.service_GUI_start_frame()

        # Clear the screen.
        set_viewport(context, zero(v2i), window_size)
        GL.clear_screen(vRGBAf(0.2, 0.2, 0.5, 0.0))
        GL.clear_sceen(@f32 1.0)

        # Scene rendering could go here.

        demo_gui(gui_service, gui_state)

        Bplus.GUI.service_GUI_end_frame()

        GLFW.SwapBuffers(context.window)
        GLFW.PollEvents()
        if GLFW.GetKey(context.window, GLFW.KEY_ESCAPE)
            break
        end
    end

    # Services can clean themselves up automatically.
    # The GUI service does so.
end