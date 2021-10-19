# Runs a GLFW/OpenGL window and messes with it.
using GLFW

GLFW.Init()
try
    wnd = GLFW.CreateWindow(700, 500, "Minnie is cute")
    GLFW.MakeContextCurrent(wnd)

    while !GLFW.WindowShouldClose(wnd)
        GLFW.PollEvents()
    end
catch e
    rethrow()
finally
    GLFW.Terminate()
end