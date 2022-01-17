# This test involves a user-controlled camera.
# By default, it is disabled so that unit tests are still automated.
if !@isdefined ENABLE_CAM3D
    # If this test was specifically requested, enable it.
    ENABLE_CAM3D = @isdefined(TEST_NAME) && (lowercase(TEST_NAME) == "cam3d")
end

using Dates
using ModernGL, GLFW
using Bplus.GL, Bplus.Helpers


# Create a GL Context and window, add some 3D stuff, and a controllable 3D camera.
ENABLE_CAM3D && bp_gl_context(v2i(800, 500), "Cam3D test"; vsync=VsyncModes.On) do context::Context
    # Set up a mesh with some 3D triangles.
    buf_tris_poses = Buffer(false, [ v4f(-0.75, -0.75, -0.75, 1.0),
                                     v4f(-0.75, 0.75, 0.25, 1.0),
                                     v4f(0.75, 0.75, 0.75, 1.0),
                                     v4f(2.75, -0.75, -0.75, 1.0) ])
    buf_tris_colors = Buffer(false, [ v3f(1, 0, 0),
                                      v3f(0, 1, 0),
                                      v3f(0, 0, 1),
                                      v3f(1, 0, 0) ])
    mesh_triangles = Mesh(PrimitiveTypes.triangle_strip,
                          [ VertexDataSource(buf_tris_poses, sizeof(v4f)),
                            VertexDataSource(buf_tris_colors, sizeof(v3f))
                          ],
                          [ VertexAttribute(1, 0x0, VertexData_FVector(4, Float32)),  # The positions, pulled directly from a v4f
                            VertexAttribute(2, 0x0, VertexData_FVector(3, Float32)) # The colors, pulled directly fromm a v3f
                          ])

    # Set up a shader to render the triangles.
    # Use a variety of uniforms and vertex attributes to mix up the colors.
    draw_triangles::Program = bp_glsl"""
        uniform vec2 u_clipPlanes = vec2(0.01, 10.0);
        float linearDepth(float z) {
            return -(z - u_clipPlanes.x) /
                   (u_clipPlanes.y - u_clipPlanes.x);
        }
        uniform vec2 u_pixelSize = vec2(800, 500);
    #START_VERTEX
        in vec4 vIn_pos;
        in vec3 vIn_color;
        uniform mat4 u_mat_worldview, u_mat_projection;
        out float vOut_depth;
        void main() {
            vec4 viewPos = u_mat_worldview * vIn_pos;
            vOut_depth = linearDepth(viewPos.z);
            gl_Position = u_mat_projection * viewPos;
        }
    #START_FRAGMENT
        uniform sampler2D u_tex;
        in float vOut_depth;
        out vec4 fOut_color;
        void main() {
            float depth_col = pow(vOut_depth, 1.0);
            fOut_color = vec4(depth_col, depth_col, depth_col,
                              1.0);

            vec2 uv = gl_FragCoord.xy / u_pixelSize;
            vec4 texCol = texture(u_tex, uv);

            fOut_color.rgb *= texCol.rgb;
        }
    """

    # Configure the render state.
    println("#TODO: Test culling mode")
    GL.set_culling(context, GL.FaceCullModes.Off)

    println("#TODO: Add a cubemap texture for the sky, with a second shader to draw it")

    # Set up the texture used to draw the triangles.
    # It's a one-channel texture containing perlin noise.
    T_SIZE::Int = 512
    tex_data = Array{Float32, 2}(undef, (T_SIZE, T_SIZE))
    for x in 1:T_SIZE
        for y in 1:T_SIZE
            perlin_pos::v2f = v2f(x, y) / @f32(T_SIZE)
            perlin_pos *= 10
            tex_data[y, x] = perlin(perlin_pos)
        end
    end
    # Configure the texture swizzler to use the single channel for RGB,
    #    and a constant value of 1.0 for the alpha.
    tex = Texture(SimpleFormat(FormatTypes.normalized_uint,
                               SimpleFormatComponents.R,
                               SimpleFormatBitDepths.B8),
                  tex_data;
                  sampler = Sampler{2}(wrapping = WrapModes.repeat),
                  swizzling = Vec4{E_SwizzleSources}(
                      SwizzleSources.red,
                      SwizzleSources.red,
                      SwizzleSources.red,
                      SwizzleSources.one
                  ))
    # Give the texture to the shader.
    glActiveTexture(GL_TEXTURE0)
    glBindTexture(GL_TEXTURE_2D, tex.handle)
    glProgramUniform1i(draw_triangles.handle, draw_triangles.uniforms["u_tex"].handle, 0)

    # Configure the 3D camera.
    cam = Cam3D{Float32}(
        pos = v3f(-3, -3, 3),

        forward = vnorm(v3f(1, 1, -1)),
        up = v3f(0, 0, 1),

        fov_degrees = 90,
        aspect_width_over_height = 800 / 500,

        clip_range = Box_minmax(0.01, 5.0)
    )
    cam_settings = Cam3D_Settings{Float32}(
        move_speed = 5
    )

    start_time = now()
    current_time = start_time
    while !GLFW.WindowShouldClose(context.window)
        # Update the clock.
        new_time = now()
        delta_time::Dates.Millisecond = new_time - current_time
        delta_seconds::Float32 = @f32(delta_time.value / 1000)
        current_time = new_time

        window_size_data::NamedTuple = GLFW.GetWindowSize(context.window)
        window_size::v2i = v2i(window_size_data.width, window_size_data.height)

        # Update the camera.
        @set! cam.fov_degrees = @f32 lerp(70, 110,
                                          0.5 + (0.5 * sin((new_time - start_time).value / 2000)))
        @set! cam.aspect_width_over_height = @f32 window_size.x / window_size.y
        cam_input = Cam3D_Input{Float32}(
            # Enable rotation:
            GLFW.GetKey(context.window, GLFW.KEY_ENTER),
            # Yaw:
            (GLFW.GetKey(context.window, GLFW.KEY_RIGHT) ? 1 : 0) -
             (GLFW.GetKey(context.window, GLFW.KEY_LEFT) ? 1 : 0),
            # Pitch:
            (GLFW.GetKey(context.window, GLFW.KEY_UP) ? 1 : 0) -
             (GLFW.GetKey(context.window, GLFW.KEY_DOWN) ? 1 : 0),
            # Boost:
            GLFW.GetKey(context.window, GLFW.KEY_LEFT_SHIFT) |
             GLFW.GetKey(context.window, GLFW.KEY_RIGHT_SHIFT),
            # Forward:
            (GLFW.GetKey(context.window, GLFW.KEY_W) ? 1 : 0) -
              (GLFW.GetKey(context.window, GLFW.KEY_S) ? 1 : 0),
            # Rightward:
            (GLFW.GetKey(context.window, GLFW.KEY_D) ? 1 : 0) -
              (GLFW.GetKey(context.window, GLFW.KEY_A) ? 1 : 0),
            # Upward:
            (GLFW.GetKey(context.window, GLFW.KEY_E) ? 1 : 0) -
              (GLFW.GetKey(context.window, GLFW.KEY_Q) ? 1 : 0),
            # Speed change:
            0
        )
        old_cam = cam
        (cam, cam_settings) = cam_update(cam, cam_settings, cam_input, delta_seconds)

        # Clear the screen.
        set_viewport(context, zero(v2i), window_size)
        clear_col = vRGBAf(0.2, 0.2, 0.5, 0.0)
        GL.render_clear(context, GL.Ptr_Target(), clear_col)
        GL.render_clear(context, GL.Ptr_Target(), @f32 1.0)

        # Draw the triangles.
        set_uniform(draw_triangles, "u_pixelSize", v2f(window_size))
        set_uniform(draw_triangles, "u_clipPlanes",
                    v2f(cam.clip_range.min, max_exclusive(cam.clip_range)))
        set_uniform(draw_triangles, "u_mat_worldview",
                    cam_view_mat(cam))
        set_uniform(draw_triangles, "u_mat_projection",
                    cam_projection_mat(cam))
        GL.render_mesh(context, mesh_triangles, draw_triangles,
                       elements = IntervalU(1, 4))

        GLFW.SwapBuffers(context.window)
        GLFW.PollEvents()
        if GLFW.GetKey(context.window, GLFW.KEY_ESCAPE)
            break
        end
    end

    # Clean up the textures.
    close(tex)
    @bp_check(tex.handle == GL.Ptr_Texture(),
              "GL.Texture's handle isn't nulled out after closing")

    # Clean up the shader.
    close(draw_triangles)
    @bp_check(draw_triangles.handle == GL.Ptr_Program(),
              "GL.Program's handle isn't nulled out after closing")

    # Clean up the buffer.
    close(mesh_triangles)
    @bp_check(mesh_triangles.handle == GL.Ptr_Mesh(),
              "GL.Mesh's handle isn't nulled out after closing")
end

# Make sure the Context got cleaned up.
@bp_check(isnothing(GL.get_context()),
          "Just closed the context, but it still exists")