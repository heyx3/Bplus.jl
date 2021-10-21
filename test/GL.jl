# Runs a GLFW/OpenGL window and messes with it.

GLFW.Init()
try
    wnd = GLFW.CreateWindow(700, 500, "Press Enter to close it")
    GLFW.MakeContextCurrent(wnd)

    timer::Int = 200_000
    while !GLFW.WindowShouldClose(wnd)
        GLFW.PollEvents()
        timer -= 1
        if (timer <= 0) || GLFW.GetKey(wnd, GLFW.KEY_ENTER)
            break
        end
    end
catch e
    rethrow()
finally
    GLFW.Terminate()
end