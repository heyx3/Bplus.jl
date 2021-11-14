# Check some basic facts.
@bp_check(GL.gl_type(GL.Ptr_Uniform) === GLint,
          "GL.Ptr_Uniform's original type is not GLint, but ",
             GL.gl_type(GL.Ptr_Uniform))
@bp_check(GL.gl_type(GL.Ptr_Buffer) === GLuint)

using ModernGL, GLFW
using Bplus.GL


# Runs a test on buffers.
function test_buffers()
    # Initialize one with some data.
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
    set_buffer_data(buf,    [ 0b11110000000000111000001000100101 ])
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
    get_buffer_data( buf, buf_actual; dest_offset = UInt(1))
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

# Create a GL Context and window.
bp_gl_context(v2i(800, 500), "Press Enter to close me"; vsync=VsyncModes.On) do context::Context
    @bp_check(context === GL.get_context(),
              "Just started this Context, but another one is the singleton")

    test_buffers()
    
    # Set up a mesh with some triangles.
    # Each triangle has position, color, and "IDs".
    # The position data is in its own buffer.
    buf_tris_poses = Buffer(false, [ v4f(-0.5, -0.5, 0.5, 1.0),
                                     v4f(-0.5, 0.5, 0.5, 1.0),
                                     v4f(0.5, 0.5, 0.5, 1.0) ])
    # The color and IDs are together in a second buffer.
    buf_tris_color_and_IDs = Buffer(false, Tuple{vRGBu8, Vec{2, UInt8}}[
        (vRGBu8(128, 128, 128), Vec{2, UInt8}(1, 0)),
        (vRGBu8(0, 255, 0),     Vec{2, UInt8}(0, 1)),
        (vRGBu8(255, 0, 255),   Vec{2, UInt8}(0, 0))
    ])
    mesh_triangles = Mesh(PrimitiveTypes.triangle,
                          [ VertexDataSource(buf_tris_poses, sizeof(v4f)),
                            VertexDataSource(buf_tris_color_and_IDs,
                                             sizeof(Tuple{vRGBu8, Vec{2, UInt8}}))
                          ],
                          [ VertexAttribute(1, 0x0, VertexData_FVector(4, Float32)),  # The positions, pulled directly from a v4f
                            VertexAttribute(2, 0x0, VertexData_FVector(3, UInt8, true)), # The colors, normalized from [0,255] to [0,1]
                            VertexAttribute(2, sizeof(vRGBu8), VertexData_IVector(2, UInt8)) # The IDs, left as integers
                          ])
    @bp_check(count_mesh_vertices(mesh_triangles) == 3,
              "The mesh has 3 vertices, but thinks it has ", count_mesh_vertices(mesh_triangles))
    @bp_check(count_mesh_elements(mesh_triangles) == count_mesh_vertices(mesh_triangles),
              "The mesh isn't indexed, yet it gives a different result for 'count vertices' (",
                  count_mesh_vertices(mesh_triangles), ") vs 'count elements' (",
                  count_mesh_elements(mesh_triangles), ")")

    # Set up a shader to render the mesh.
    draw_triangles::Program = bp_glsl"""
        uniform ivec3 u_myVec = ivec3(1, 1, 1);
    #START_VERTEX
        in vec4 vIn_pos;
        in vec3 vIn_color;
        in ivec2 vIn_IDs;
        out vec3 vOut_color;
        void main() {
            gl_Position = vIn_pos;

            ivec3 ids = ivec3(vIn_IDs, all(vIn_IDs == 0) ? 1 : 0);
            vOut_color = vIn_color * vec3(ids);
        }
    #START_FRAGMENT
        in vec3 vOut_color;
        out vec4 fOut_color;
        void main() {
            fOut_color = vec4(vOut_color * u_myVec, 1.0);
        }
    """
    #@bp_check(isempty(draw_triangles.uniforms), "Program has >0 uniforms: ", draw_triangles.uniform)
    # Tests for when we ue the uniform (currently it gets compiled out):
    @bp_check(length(draw_triangles.uniforms) == 1,
              "Program has !=1 uniforms: ", draw_triangles.uniforms)
    @bp_check(haskey(draw_triangles.uniforms, "u_myVec"),
              "Program has unexpected uniform: ", draw_triangles.uniforms)
    @bp_check(draw_triangles.uniforms["u_myVec"] ==
                UniformData(GL.Ptr_Uniform(0), Vec3{Int32}, 1),
              "Wrong uniform data: ", draw_triangles.uniforms["u_myVec"])

    # Configure the render state.
    GL.set_culling(context, GL.FaceCullModes.Off)

    timer::Int = 5 * 60  #Vsync is on, assume 60fps
    while !GLFW.WindowShouldClose(context.window)
        clear_col = lerp(vRGBAf(0.4, 0.8, 0.2, 1.0),
                         vRGBAf(0.6, 0.8, 0.8, 1.0),
                         rand(Float32))
        GL.render_clear(context, GL.Ptr_Target(), clear_col)
        GL.render_clear(context, GL.Ptr_Target(), @f32 1.0)

        GL.render_mesh(context, mesh_triangles, draw_triangles,
                       elements = IntervalU(1, 3))

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
end

@bp_check(isnothing(GL.get_context()),
          "Just closed the context, but it still exists")