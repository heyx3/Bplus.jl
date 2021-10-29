# Check some basic facts.
@bp_check(GL.gl_type(GL.Ptr_Uniform) === GLint,
          "GL.Ptr_Uniform's original type is not GLint, but ",
             GL.gl_type(GL.Ptr_Uniform))
@bp_check(GL.gl_type(GL.Ptr_Buffer) === GLuint)

# Create a GL Context and window.
bp_gl_context(v2i(800, 500), "Press Enter to close me") do context::Context
    @bp_check(context === GL.get_context(),
              "Just started this Context, but another one is the singleton")

    timer::Int = 200_000
    while !GLFW.WindowShouldClose(context.window)
        clear_col = vRGBAf(rand(Float32), rand(Float32), rand(Float32), @f32 1)
        GL.render_clear(context, GL.Ptr_Target(), clear_col)

        GLFW.PollEvents()
        timer -= 1
        if (timer <= 0) || GLFW.GetKey(context.window, GLFW.KEY_ENTER)
            break
        end
    end

    
end

@bp_check(isnothing(GL.get_context()),
          "Just closed the context, but it still exists")