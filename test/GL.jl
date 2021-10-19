# Runs a GLFW/OpenGL window and messes with it.
using GLFW

GLFW.Init()
try
    wnd = GLFW.CreateWindow(700, 500, "Press Enter to close it")
    GLFW.MakeContextCurrent(wnd)

    while !GLFW.WindowShouldClose(wnd)
        GLFW.PollEvents()
        if GLFW.GetKey(wnd, GLFW.KEY_ENTER)
            break
        end
    end
catch e
    rethrow()
finally
    GLFW.Terminate()
end