# To disable the slower tests, set `global GL_TEST_FULL=false`.
if !@isdefined GL_TEST_FULL
    GL_TEST_FULL = true
end

# Check some basic facts.
@bp_check(GL.gl_type(GL.Ptr_Uniform) === GLint,
          "GL.Ptr_Uniform's original type is not GLint, but ",
             GL.gl_type(GL.Ptr_Uniform))
@bp_check(GL.gl_type(GL.Ptr_Buffer) === GLuint)

using ModernGL, GLFW, Images, FileIO
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

# Runs a test on textures.
function test_textures()
    # Manipulate the texture using different sizes and different TexTypes.
    for type::E_TexTypes in (TexTypes.oneD, TexTypes.twoD, TexTypes.threeD)
        for size in (1, 5, 13) # Larger sizes make the test crazy slow for 3D textures
            local D::Int
            if type == TexTypes.oneD
                D = 1
            elseif type == TexTypes.twoD
                D = 2
            elseif type == TexTypes.threeD
                D = 3
            else
                error("Unhandled case: ", D)
            end

            sizeD::VecI{D} = Vec(i->size, D)

            # Try some different texture formats.
            formats::Vector{Tuple{Type, TexFormat}} = vcat(
                # Generate a bunch of normal formats.
                filter(((I, f), ) -> exists(get_ogl_enum(f)),
                  map(Iterators.product((SimpleFormatComponents.R,
                                         SimpleFormatComponents.RG,
                                         SimpleFormatComponents.RGB,
                                         SimpleFormatComponents.RGBA),
                                        ((UInt8, SimpleFormatBitDepths.B8),
                                         (UInt16, SimpleFormatBitDepths.B16),
                                         (UInt32, SimpleFormatBitDepths.B32)))) do (components, (I, depth))
                      return (I, SimpleFormat(FormatTypes.normalized_uint, components, depth))
                  end
                )
                #TODO: Incorporate other formats, e.x. depth/stencil
            )
            for (I, format) in formats
                components = GL.get_n_channels(format)
                for n_mips::Int in 1:get_n_mips(sizeD)
                    #TODO: Another loop for which set of components to set/get
                    tex = Texture(format, sizeD; n_mips=n_mips)
                    #TODO: Do the below tests for each mip level. Also check that mips are different after recomputation

                    # Clear the texture, then check the pixels.
                    val = Vec(ntuple(i -> rand(I), Int(components)))
                    clear_tex_color(tex, val)
                    out_buf = Array{Vec{Int(components), I}, D}(undef, reverse(sizeD).data)
                    get_tex_color(tex, out_buf)
                    @bp_check(all(p -> p == val, out_buf),
                              "Cleared pixels to ", val, " but got: ", out_buf)
                    # Clear a subset, then check the pixels again.
                    if all(sizeD >= 3)
                        val2 = Vec(ntuple(i -> rand(I), Int(components)))
                        clear_range = Box{VecI{D}}(2, max(sizeD - 3, 1))
                        clear_tex_color(tex, val2; subset=TexSubset(clear_range))
                        uncleared_range = union(collect(1:(clear_range.min - 1)),
                                                collect(max_exclusive(clear_range):sizeD))
                        get_tex_color(tex, out_buf)
                        @bp_check(all(i -> out_buf[i]==val, uncleared_range),
                                  "Pixels that weren't affected by clear_tex_color()",
                                    " are still different: ", uncleared_range, " of\n\t", out_buf)
                        @bp_check(all(i -> out_buf[i]==val2,
                                      clear_range.min : max_inclusive(clear_range)),
                                  "Pixels that were in the cleared subset aren't",
                                    " the expected values: ", cleared_range, " of\n\t", out_buf)
                    end

                    # Try getting and setting individual pixels.
                    for pixel::VecI in 1:sizeD
                        val = Vec(ntuple(i -> rand(I), Int(components)))
                        set_tex_color(tex, [ val ];
                                      subset=TexSubset(Box(pixel, 1)))
                        out_buf = Array{Vec{Int(components), I}, D}(undef, ntuple(i->1, D))
                        get_tex_color(tex, out_buf;
                                      subset=TexSubset(Box(pixel, 1)))
                        @bp_check(out_buf[1] == val,
                                  "Set pixel of texture ", type, "_", format, "_", sizeD, "_", n_mips,
                                      " at ", pixel, " with ", val, ".")
                    end
                    #TODO: Set as UInt[N], get as float, and test

                    # Clean up.
                    close(tex)
                    @bp_check(get_ogl_handle(tex) == GL.Ptr_Texture(),
                              "Texture was close()-d yet it still has a handle: ",
                                type, " ", format, " ", size)
                end
            end
        end
    end 
end

function check_gl_logs(context::String)
    logs = pull_gl_logs()
    for log in logs
        if log.severity in (DebugEventSeverities.high, DebugEventSeverities.medium)
            @error "While $context. $(sprint(show, log))"
        elseif log.severity == DebugEventSeverities.low
            @warn "While $context. $(sprint(show, log))"
        elseif log.severity == DebugEventSeverities.none
            @info "While $context. $(sprint(show, log))"
        else
            error("Unhandled case: ", log.severity)
        end
    end
end

# Create a GL Context and window, and put the GL library through its paces.
bp_gl_context( v2i(800, 500), "Running tests...press Enter to finish once rendering starts";
               vsync=VsyncModes.On,
               debug_mode=true
             ) do context::Context
    @bp_check(context === GL.get_context(),
              "Just started this Context, but another one is the singleton")
    check_gl_logs("just starting up")

    println("Device: ", context.device.gpu_name)

    # Run some basic tests with GL resources.
    if GL_TEST_FULL
        test_buffers()
        test_textures()
        check_gl_logs("running test battery")
    end

    # Keep track of the resources to clean up, in the order they should be cleaned up.
    to_clean_up = Resource[ ]

    # Set up a mesh with some triangles.
    # Each triangle has position, color, and "IDs".
    # The position data is in its own buffer.
    buf_tris_poses = Buffer(false, [ v4f(-0.75, -0.75, -0.35, 1.0),
                                     v4f(-0.75, 0.75, 0.25, 1.0),
                                     v4f(0.75, 0.75, 0.75, 1.0) ])
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
                            VertexAttribute(2, sizeof(vRGBu8), VertexData_UVector(2, UInt8)) # The IDs, left as integers
                          ])
    check_gl_logs("creating the simple triangle mesh")
    @bp_check(count_mesh_vertices(mesh_triangles) == 3,
              "The mesh has 3 vertices, but thinks it has ", count_mesh_vertices(mesh_triangles))
    @bp_check(count_mesh_elements(mesh_triangles) == count_mesh_vertices(mesh_triangles),
              "The mesh isn't indexed, yet it gives a different result for 'count vertices' (",
                  count_mesh_vertices(mesh_triangles), ") vs 'count elements' (",
                  count_mesh_elements(mesh_triangles), ")")
    push!(to_clean_up, mesh_triangles, buf_tris_poses, buf_tris_color_and_IDs)

    #TODO: Add another indexed mesh to test indexed rendering.

    # Set up a shader to render the triangles.
    # Use a variety of uniforms and vertex attributes to mix up the colors.
    draw_triangles::Program = bp_glsl"""
        uniform ivec3 u_colorMask = ivec3(1, 1, 1);
    #START_VERTEX
        in vec4 vIn_pos;
        in vec3 vIn_color;
        in ivec2 vIn_IDs;  // Expected to be (1,0) for the first point,
                           //    (0,1) for the second,
                           //    (0,0) for the third.
        out vec3 vOut_color;
        void main() {
            gl_Position = vIn_pos;
            ivec3 ids = ivec3(vIn_IDs, all(vIn_IDs == 0) ? 1 : 0);
            vOut_color = vIn_color * vec3(ids);
        }
    #START_FRAGMENT
        uniform mat3 u_colorTransform = mat3(1, 0, 0, 0, 1, 0, 0, 0, 1);
        uniform dmat2x4 u_colorCurveAt512[10];
        uniform sampler2D u_tex;
        in vec3 vOut_color;
        out vec4 fOut_color;
        void main() {
            fOut_color = vec4(vOut_color * u_colorMask, 1.0);

            fOut_color.rgb = clamp(u_colorTransform * fOut_color.rgb, 0.0, 1.0);

            float scale = float(u_colorCurveAt512[3][1][2]);
            fOut_color.rgb = pow(fOut_color.rgb, vec3(scale));

            fOut_color.rgb *= texture(u_tex, gl_FragCoord.xy / vec2(800, 500)).rgb;
        }
    """
    function check_uniform(name::String, type, array_size)
        @bp_check(haskey(draw_triangles.uniforms, name),
                  "Program is missing uniform '", name, "': ", draw_triangles.uniforms)
        @bp_check(draw_triangles.uniforms[name].type == type,
                  "Wrong uniform data for '", name, "': ", draw_triangles.uniforms[name])
        @bp_check(draw_triangles.uniforms[name].array_size == array_size,
                  "Wrong uniform data for '", name, "': ", draw_triangles.uniforms[name])
    end
    @bp_check(Set(keys(draw_triangles.uniforms)) ==
                  Set([ "u_colorMask", "u_colorTransform", "u_colorCurveAt512", "u_tex" ]),
              "Unexpected set of uniforms: ", collect(keys(draw_triangles.uniforms)))
    check_uniform("u_colorMask", Vec3{Int32}, 1)
    check_uniform("u_colorTransform", fmat3x3, 1)
    check_uniform("u_colorCurveAt512", dmat2x4, 10)
    check_uniform("u_tex", GL.Ptr_View, 1)
    push!(to_clean_up, draw_triangles)
    check_gl_logs("creating the simple triangle shader")
    # Set up the texture used to draw the triangles.
    T_SIZE::Int = 512
    tex_data = Array{vRGBf, 2}(undef, (T_SIZE, T_SIZE))
    for x in 1:T_SIZE
        for y in 1:T_SIZE
            perlin_pos::v2f = v2f(x, y) / @f32(T_SIZE)
            perlin_pos *= 20
            tex_data[y, x] = vRGBf(
                perlin(perlin_pos, tuple(0x90843277)),
                perlin(perlin_pos, tuple(0xabfbcce2)),
                perlin(perlin_pos, tuple(0x5678cbef))
            )
        end
    end
    tex = Texture(SimpleFormat(FormatTypes.normalized_uint,
                               SimpleFormatComponents.RGB,
                               SimpleFormatBitDepths.B8),
                  tex_data;
                  sampler = Sampler{2}(wrapping = Vec(WrapModes.repeat, WrapModes.repeat)))
    push!(to_clean_up, tex)
    check_gl_logs("creating the simple triangles' texture")
    set_uniform(draw_triangles, "u_tex", tex)
    check_gl_logs("giving the texture to the simple triangles' shader")

    resources::CResources = get_resources(context)
    @assert(GL.count_mesh_vertices(resources.screen_triangle) == 3)
    @assert(GL.count_mesh_vertices(resources.quad) == 4)

    # Draw the skybox as a full-screen triangle with some in-shader projection math.
    # Set up a shader for the "skybox",
    #    which takes a yaw angle and aspect ratio to map the screen quad to a 3D view range.
    draw_skybox::Program = bp_glsl"""
    #START_VERTEX
        in vec2 vIn_corner;
        uniform float u_yaw_radians = 0.0,
                      u_width_over_height = 1.0,
                      u_fov_width = 1.0;
        out vec3 vOut_cubeUV;
        vec2 Rot(vec2 v, float radians) {
            vec2 cossin = vec2(cos(radians), sin(radians));
            vec4 cossin4 = vec4(cossin, -cossin);
            return vec2(dot(v, cossin4.xw),
                        dot(v, cossin4.yx));
        }
        void main() {
            gl_Position = vec4(vIn_corner, 0.9999, 1.0);

            vec2 uvHorzMin = Rot(vec2(-u_fov_width, 1.0), u_yaw_radians),
                 uvHorzMax = Rot(vec2(u_fov_width, 1.0), u_yaw_radians);

            vec3 uvMax = vec3(uvHorzMax, u_fov_width * u_width_over_height),
                 uvMin = vec3(uvHorzMin, -u_fov_width * u_width_over_height);

            vec3 t = vec3(0.5 + (0.5 * vIn_corner.xxy));
            vOut_cubeUV = mix(uvMin, uvMax, t);
        }
    #START_FRAGMENT
        in vec3 vOut_cubeUV;
        uniform samplerCube u_tex;
        out vec4 fOut_color;
        void main() {
            fOut_color = vec4(texture(u_tex, normalize(vOut_cubeUV)).rgb,
                              1.0);
        }
    """
    push!(to_clean_up, draw_skybox)
    check_gl_logs("compiling the 'skybox' shader")
    # Create the skybox texture.
    SKYBOX_TEX_LENGTH = 512
    NOISE_SCALE = Float32(17)
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
    push!(to_clean_up, tex_skybox)
    check_gl_logs("creating the 'skybox' texture")
    set_uniform(draw_skybox, "u_tex", tex_skybox)
    check_gl_logs("giving the 'skybox' texture to its shader")

    # Set up a Target for rendering into.
    target = Target(v2u(800, 600),
                    SimpleFormat(FormatTypes.normalized_uint,
                                 SimpleFormatComponents.RGB,
                                 SimpleFormatBitDepths.B8),
                    DepthStencilFormats.depth_24u)
    push!(to_clean_up, target)
    check_gl_logs("creating the Target")

    # Configure the render state.
    set_culling(context, FaceCullModes.Off)
    set_depth_writes(context, true)
    set_depth_test(context, ValueTests.LessThan)
    set_blending(context, make_blend_opaque(BlendStateRGBA))

    camera_yaw_radians::Float32 = 0
    timer::Int = (GL_TEST_FULL ? 5 : 500) * 60  #Vsync is on, assume 60fps
    while !GLFW.WindowShouldClose(context.window)
        window_size::v2i = get_window_size(context)
        check_gl_logs("starting a new tick")

        # Clear the screen.
        clear_col = lerp(vRGBAf(0.4, 0.4, 0.2, 1.0),
                         vRGBAf(0.6, 0.45, 0.6, 1.0),
                         rand(Float32))
        GL.render_clear(context, GL.Ptr_Target(), clear_col)
        GL.render_clear(context, GL.Ptr_Target(), @f32 1.0)
        check_gl_logs("clearing the screen")

        # Randomly shift some uniforms every N ticks.
        if timer % 30 == 0
            # Set 'u_colorMask' as an array of 1 element, just to test that code path.
            set_uniforms(draw_triangles, "u_colorMask",
                         [ Vec{Int32}(rand(0:1), rand(0:1), rand(0:1)) ])
            set_uniform(draw_triangles, "u_colorTransform",
                        fmat3x3(ntuple(i->rand(), 9)...))
        end
        # Continuously vary another uniform.
        pow_val = lerp(0.3, 1/0.3,
                       abs(mod(timer / 30, 2) - 1) ^ 5)
        set_uniform(draw_triangles, "u_colorCurveAt512",
                    dmat2x4(0, 0, 0, 0,
                            0, 0, pow_val, 0),
                    4)
        check_gl_logs("setting uniforms during tick")

        # Render the triangles into the render-target,
        #    then we'll re-render them while sampling from that target
        #    to create a trippy effect.
        function draw_scene(triangle_tex, msg_context...)
            set_depth_test(context, ValueTests.LessThan)

            # Update the triangle uniforms.
            set_uniform(draw_triangles, "u_tex", triangle_tex)
            # Draw the triangles.
            view_activate(get_view(triangle_tex))
            render_mesh(context, mesh_triangles, draw_triangles,
                        elements = IntervalU(1, 3))
            view_deactivate(get_view(triangle_tex))
            check_gl_logs(string("drawing the triangles ", msg_context...))

            # Update the skybox uniforms.
            camera_yaw_radians += deg2rad(.05)
            set_uniform(draw_skybox, "u_yaw_radians", camera_yaw_radians)
            set_uniform(draw_skybox, "u_fov_width", @f32(1))
            set_uniform(draw_skybox, "u_width_over_height", @f32(window_size.x / window_size.y))
            check_gl_logs(string("setting the skybox's uniforms ", msg_context...))
            # Draw the skybox.
            view_activate(get_view(tex_skybox))
            render_mesh(context, resources.screen_triangle, draw_skybox)
            view_deactivate(get_view(tex_skybox))
            check_gl_logs(string("drawing the skybox ", msg_context...))
        end
        target_activate(target)
        target_clear(target, vRGBAf(1, 0, 1, 0))
        target_clear(target, @f32 1.0)
        draw_scene(tex, "into a Target")

        # Draw the Target's data onto the screen.
        target_activate(nothing)
        render_clear(context, GL.Ptr_Target(), vRGBAf(0, 1, 0, 0))
        render_clear(context, GL.Ptr_Target(), @f32 1.0)
        set_depth_test(context, ValueTests.Pass)
        check_gl_logs("clearing the screen")
        target_tex = target.attachment_colors[1].tex
        resource_blit(resources, target_tex)

        GLFW.SwapBuffers(context.window)

        GLFW.PollEvents()
        timer -= 1
        if (timer <= 0) || GLFW.GetKey(context.window, GLFW.KEY_ENTER)
            break
        end
    end

    # Clean up the resources.
    for rs::Resource in to_clean_up
        close(rs)

        # Make sure the resource's handle is nulled out, to prevent potential errors.
        closed_handle = get_ogl_handle(rs)
        null_handle = typeof(closed_handle)()
        @bp_check(closed_handle == null_handle,
                  "After closing a ", typeof(rs), " its handle wasn't nulled out: ",
                    closed_handle, " vs ", null_handle)

        check_gl_logs("cleaning up $(typeof(rs))")
    end
end

# Make sure the Context got cleaned up.
@bp_check(isnothing(GL.get_context()),
          "Just closed the context, but it still exists")