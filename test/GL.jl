# Check some basic facts.
@bp_check(GL.gl_type(GL.Ptr_Uniform) === GLint,
          "GL.Ptr_Uniform's original type is not GLint, but ",
             GL.gl_type(GL.Ptr_Uniform))
@bp_check(GL.gl_type(GL.Ptr_Buffer) === GLuint)

using ModernGL, GLFW
using Bplus.GL

# Create a GL Context and window.
bp_gl_context(v2i(800, 500), "Press Enter to close me"; vsync=VsyncModes.On) do context::Context
    @bp_check(context === GL.get_context(),
              "Just started this Context, but another one is the singleton")
    
    # Try compiling a shader.
    draw_triangles::Program = bp_glsl"""
        uniform ivec3 u_myVec;
    #START_VERTEX
        out vec3 out_points;
        void main() {
            if (gl_VertexID == 0) {
                out_points = vec3(1.0, 0.0, 0.0);
                gl_Position = vec4(-0.5, -0.5, 0.5, 1.0);
            } else if (gl_VertexID == 1) {
                out_points = vec3(0.0, 1.0, 0.0);
                gl_Position = vec4(-0.5, 0.5, 0.5, 1.0);
            } else {
                out_points = vec3(0.0, 0.0, 1.0);
                gl_Position = vec4(0.5, 0.5, 0.5, 1.0);
            }
        }
    #START_FRAGMENT
        in vec3 out_points;
        out vec4 out_color;
        void main() {
            out_color = vec4(out_points, 1.0);
        }
    """
    glUseProgram(draw_triangles.handle)

    # Create one dummy mesh with no data, to render our test shader.
    empty_mesh::Mesh = Mesh(PrimitiveTypes.triangle,
                            VertexDataSource[ ],
                            VertexAttribute[ ])

    # Configure the render state.
    GL.set_culling(context, GL.FaceCullModes.Off)

    timer::Int = 6_000
    while !GLFW.WindowShouldClose(context.window)
        clear_col = lerp(vRGBAf(0.4, 0.8, 0.2, 1.0),
                         vRGBAf(0.6, 0.8, 0.8, 1.0),
                         rand(Float32))
        GL.render_clear(context, GL.Ptr_Target(), clear_col)
        GL.render_clear(context, GL.Ptr_Target(), @f32 1.0)

        GL.render_mesh(empty_mesh, elements = IntervalU(1, 3))

        GLFW.SwapBuffers(context.window)

        GLFW.PollEvents()
        timer -= 1
        if (timer <= 0) || GLFW.GetKey(context.window, GLFW.KEY_ENTER)
            break
        end
    end

    # Clean up the shader.
    close(draw_triangles)
    @bp_check(draw_triangles.handle == GL.Ptr_Program(),
              "GL.Program's handle isn't nulled out")

    # Test Buffers, initialize one with some data.
    buf::Buffer = Buffer(true, [ 0x1, 0x2, 0x3, 0x4 ])
    data::Vector{UInt8} = get_buffer_data(buf)
    @bp_check(data isa Vector{UInt8}, "Actual type: ", data)
    @bp_check(data == [ 0x1, 0x2, 0x3, 0x4 ],
              "Expected data: ", map(bitstring, [ 0x1, 0x2, 0x3, 0x4 ]),
              "\n\tActual data: ", map(bitstring, data))
    # Try setting its data after creation.
    set_buffer_data(buf, [ 0x5, 0x7, 0x9, 0x19])
    @bp_check(get_buffer_data(buf) == [ 0x5, 0x7, 0x9, 0x19 ])
    # Try reading the data into an existing array.
    buf_actual = UInt8[ 0x0, 0x0, 0x0, 0x0 ]
    get_buffer_data(buf, buf_actual)
    @bp_check(buf_actual == [ 0x5, 0x7, 0x9, 0x19 ])
    # Try setting the buffer to contain one big int, make sure there's no endianness issues.
    set_buffer_data(buf,                      [ 0b11110000000000111000001000100101 ])
    buf_actual = get_buffer_data(buf, UInt32)
    @bp_check(buf_actual == [ 0b11110000000000111000001000100101 ],
              "Actual data: ", buf_actual)
    # Try setting and getting with offsets.
    set_buffer_data(buf, [ 0x5, 0x7, 0x9, 0x19])
    buf_actual = get_buffer_data(buf, UInt8; src_elements = IntervalU(2, 2))
    @bp_check(buf_actual == [ 0x7, 0x9 ], map(bitstring, buf_actual))
    buf_actual = get_buffer_data(buf, UInt8;
                                 src_elements = IntervalU(2, 2),
                                 src_byte_offset = 1)
    @bp_check(buf_actual == [ 0x9, 0x19 ], map(bitstring, buf_actual))
    buf_actual = UInt8[ 0x2, 0x0, 0x0, 0x0, 0x0, 0x4 ]
    get_buffer_data( buf, buf_actual;
                     dest_offset = UInt(1)
                   )
    @bp_check(buf_actual == [ 0x2, 0x5, 0x7, 0x9, 0x19, 0x4 ],
              buf_actual)
    # Try copying.
    buf2 = Buffer(30, true, true)
    copy_buffer(buf, buf2;
                src_byte_offset = 2,
                dest_byte_offset = 11,
                byte_size = 2)
    buf_actual = get_buffer_data(buf2; src_byte_offset = 11, src_elements = IntervalU(1, 2))
    @bp_check(buf_actual == [ 0x9, 0x19 ],
              "Copying buffers with offsets: expected [ 0x9, 0x19 ], got ", buf_actual)
    # Clean up.
    close(buf2)
    close(buf)
    @bp_check(buf.handle == GL.Ptr_Buffer(),
              "Buffer 1's handle isn't zeroed out after closing")
    @bp_check(buf2.handle == GL.Ptr_Buffer(),
              "Buffer 2's handle isn't zeroed out after closing")
end

@bp_check(isnothing(GL.get_context()),
          "Just closed the context, but it still exists")