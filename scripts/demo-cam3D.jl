# Defines a tiny 3D scene and a movable camera.
# Use WASD to move, and press Enter to enable rotation with the arrow keys.

cd(joinpath(@__DIR__, ".."))
insert!(LOAD_PATH, 1, ".")
insert!(LOAD_PATH, 1, "test")

using Dates, Setfield
using ModernGL, GLFW
using Bplus.GL, Bplus.Helpers, Bplus.Math, Bplus.Utilities


bp_gl_context( v2i(800, 500), "Cam3D demo";
               vsync=VsyncModes.on,
               debug_mode=true
             ) do context::Context
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
                          [ VertexAttribute(1, 0x0, VSInput(v4f)),
                            VertexAttribute(2, 0x0, VSInput(v3f))
                          ])
    # Set up a shader to render the triangles.
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

    # Set up a "sky box".
    buf_skybox_poses = Buffer(false, [
        Vec3{Int8}(-1, -1, -1),
        Vec3{Int8}(1, -1, -1),
        Vec3{Int8}(-1, 1, -1),
        Vec3{Int8}(1, 1, -1),
        Vec3{Int8}(-1, -1, 1),
        Vec3{Int8}(1, -1, 1),
        Vec3{Int8}(-1, 1, 1),
        Vec3{Int8}(1, 1, 1)
    ])
    buf_skybox_indices = Buffer(false, UInt8[
        # -X
        0, 2, 4,
        2, 4, 6,

        # +X
        1, 3, 5,
        3, 5, 7,

        # -Y
        0, 1, 4,
        1, 4, 5,

        # +Y
        2, 3, 6,
        3, 6, 7,

        # -Z
        0, 1, 2,
        1, 2, 3,

        # +Z
        4, 5, 6,
        5, 6, 7
    ])
    mesh_skybox = Mesh(PrimitiveTypes.triangle,
                       [ VertexDataSource(buf_skybox_poses, sizeof(Vec3{Int8})) ],
                       [ VertexAttribute(1, 0x0, VSInput_FloatVector(Vec3{Int8}, false)) ],
                       MeshIndexData(buf_skybox_indices, UInt8))
    # Set up a shader to render the skybox.
    draw_skybox::Program = bp_glsl"""
        uniform vec3 u_camPos = vec3(0.0, 0.0, 0.0);
    #START_VERTEX
        in vec3 vIn_pos;
        uniform mat4 u_mat_view, u_mat_projection;
        out vec3 vOut_cubeDir;
        void main() {
            vOut_cubeDir = vIn_pos;

            vec3 world_pos = (vIn_pos * 6.0) + u_camPos;
            gl_Position = u_mat_projection * (u_mat_view * vec4(world_pos, 1.0));
        }
    #START_FRAGMENT
        in vec3 vOut_cubeDir;
        uniform samplerCube u_tex;
        out vec4 fOut_color;
        void main() {
            vec3 skyColor = texture(u_tex, vOut_cubeDir).rgb;

            //Add some color at the center of each cube face.
            //Note that this causes some slight seams at the cube faces.
            vec3 cubeDirWeights = abs(normalize(vOut_cubeDir));
            float colorStrength = max(cubeDirWeights.x, max(cubeDirWeights.y, cubeDirWeights.z));
            colorStrength = pow(colorStrength, 3.0);
            colorStrength = smoothstep(0.0, 1.0, smoothstep(0.0, 1.0, colorStrength));
            skyColor *= mix(vec3(1.0), abs(vOut_cubeDir), colorStrength);

            fOut_color = vec4(skyColor, 1.0);
        }
    """
    SKYBOX_TEX_LENGTH = 64
    NOISE_SCALE = Float32(4)
    pixels_skybox = Array{Float32, 3}(undef, (SKYBOX_TEX_LENGTH, SKYBOX_TEX_LENGTH, 6))
    for face in 1:6
        for y in 1:SKYBOX_TEX_LENGTH
            for x in 1:SKYBOX_TEX_LENGTH
                uv::v2f = (v2f(x, y) - @f32(0.5)) / SKYBOX_TEX_LENGTH
                cube_dir::v3f = vnorm(get_cube_dir(CUBEMAP_MEMORY_LAYOUT[face], uv))
                pixels_skybox[x, y, face] = perlin(cube_dir * NOISE_SCALE)
            end
        end
    end
    tex_skybox = Texture_cube(SimpleFormat(FormatTypes.normalized_uint,
                                           SimpleFormatComponents.R,
                                           SimpleFormatBitDepths.B8),
                              pixels_skybox
                              ;
                              swizzling=SwizzleRGBA(
                                  SwizzleSources.red,
                                  SwizzleSources.red,
                                  SwizzleSources.red,
                                  SwizzleSources.one
                              ))
    tex_skybox_view = get_view(tex_skybox, GL.TexSampler{3}(
        pixel_filter = PixelFilters.smooth,
        wrapping = Vec(WrapModes.clamp, WrapModes.clamp, WrapModes.clamp)
    ))
    set_uniform(draw_skybox, "u_tex", tex_skybox_view)


    # Configure the render state.
    println("#TODO: Test culling mode")
    GL.set_culling(context, GL.FaceCullModes.off)

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
                  sampler = TexSampler{2}(wrapping = Vec(WrapModes.repeat,WrapModes.repeat)),
                  swizzling = Vec4{E_SwizzleSources}(
                      SwizzleSources.red,
                      SwizzleSources.red,
                      SwizzleSources.red,
                      SwizzleSources.one
                  ))
    # Give the texture to the shader.
    set_uniform(draw_triangles, "u_tex", tex)

    # Configure the 3D camera.
    cam = Cam3D{Float32}(
        pos = v3f(-3, -3, 3),

        forward = vnorm(v3f(1, 1, -1)),
        up = v3f(0, 0, 1),

        clip_range = Box(min=0.01, max-10.0)
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

        window_size::v2i = get_window_size(context)

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
        mat_view::fmat4x4 = cam_view_mat(cam)
        mat_projection::fmat4x4 = cam_projection_mat(cam)

        # Clear the screen.
        set_viewport(context, zero(v2i), window_size)
        clear_col = vRGBAf(0.2, 0.2, 0.5, 0.0)
        GL.render_clear(context, GL.Ptr_Target(), clear_col)
        GL.render_clear(context, GL.Ptr_Target(), @f32 1.0)

        # Draw the triangles.
        set_uniform(draw_triangles, "u_pixelSize", convert(v2f, window_size))
        set_uniform(draw_triangles, "u_clipPlanes",
                    v2f(min_inclusive(cam.clip_range),
                        max_exclusive(cam.clip_range)))
        set_uniform(draw_triangles, "u_mat_worldview", mat_view)
        set_uniform(draw_triangles, "u_mat_projection", mat_projection)
        view_activate(get_view(tex))
        GL.render_mesh(context, mesh_triangles, draw_triangles,
                       elements = IntervalU(1, 4))
        view_deactivate(get_view(tex))

        # Draw the skybox.
        #TODO: Move to be after the triangles to test depth-testing.
        set_uniform(draw_skybox, "u_camPos", cam.pos)
        set_uniform(draw_skybox, "u_mat_view", mat_view)
        set_uniform(draw_skybox, "u_mat_projection", mat_projection)
        view_activate(tex_skybox_view)
        GL.render_mesh(context, mesh_skybox, draw_skybox)
        view_deactivate(tex_skybox_view)

        GLFW.SwapBuffers(context.window)
        GLFW.PollEvents()
        if GLFW.GetKey(context.window, GLFW.KEY_ESCAPE)
            break
        end
    end

    # Clean up.
    close(tex)
    close(tex_skybox)
    close(draw_triangles)
    close(draw_skybox)
    close(mesh_triangles)
    close(mesh_skybox)
    close(buf_tris_poses)
    close(buf_tris_colors)
    close(buf_skybox_poses)
    close(buf_skybox_indices)
end