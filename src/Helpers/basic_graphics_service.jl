"
A Context service which defines a bunch of useful GL resources:

 * `screen_triangle` : A 1-triangle mesh with 2D positions in NDC-space.
                       When drawn, it will perfectly cover the entire screen,
                          making it easy to spin up post-processing effects.
                       The UV coordinates can be calculated from the XY positions
                          (or from gl_FragCoord).
 * `quad` : A 2-triangle mesh describing a square, with 2D coordinates
                in the range (-1, -1) to (+1, +1).
            This _can_ be used for post-processing effects, but it's less efficient
                than `screen_triangle`, for technical reasons.
 * `blit` : A simple shader to render a 2D texture (e.x. copy a Target to the screen).
            Refer to `simple_blit()`.
 * `empty_mesh` : A mesh with no vertex data, for dispatching entirely procedural geometry.
                  Has the 'points' PrimitiveType.
"
mutable struct BasicGraphicsService
    screen_triangle::Mesh
    quad::Mesh
    blit::Program
    empty_mesh::Mesh

    # The master list of things this service owns and needs to clean up.
    references::Set{AbstractResource}
end
export BasicGraphicsService

function BasicGraphicsService()
    screen_tri_poses = Buffer(false, map(Vec{2, Int8}, [
        (-1, -1),
        (3, -1),
        (-1, 3)
    ]))
    screen_tri = Mesh(PrimitiveTypes.triangle_strip,
                      [ VertexDataSource(screen_tri_poses, sizeof(Vec{2, Int8})) ],
                      [ VertexAttribute(1, 0x0, VSInput_FVector(Vec2{Int8}, false)) ])

    quad_poses = Buffer(false, map(Vec{2, Int8}, [
        (-1, -1),
        (1, -1),
        (-1, 1),
        (1, 1)
    ]))
    quad = Mesh(PrimitiveTypes.triangle_strip,
                [ VertexDataSource(quad_poses, sizeof(Vec{2, Int8})) ],
                [ VertexAttribute(1, 0x0, VSInput_FVector(Vec2{Int8}, false)) ])

    #TODO: Bool uniforms to avoid the matrix math if they're identity matrices?
    blit = bp_glsl"""
uniform mat3 u_mesh_transform = mat3(1, 0, 0,
                                     0, 1, 0,
                                     0, 0, 1);
uniform sampler2D u_tex;
uniform mat4 u_color_map = mat4(1, 0, 0, 0,
                                0, 1, 0, 0,
                                0, 0, 1, 0,
                                0, 0, 0, 1);
#START_VERTEX
in vec2 vIn_corner;
out vec2 vOut_uv;
void main() {
    vec3 pos3 = u_mesh_transform * vec3(vIn_corner, 1.0);
    gl_Position = vec4(pos3.xy / pos3.z, 0.5, 1.0);

    vOut_uv = 0.5 + (0.5 * vIn_corner);
}
#START_FRAGMENT
in vec2 vOut_uv;
out vec4 vOut_color;
void main() {
    vOut_color = u_color_map * texture(u_tex, vOut_uv);
}
"""

    empty_mesh = GL.Mesh(GL.PrimitiveTypes.point,
                         GL.VertexDataSource[ ],
                         GL.VertexAttribute[ ])

    return BasicGraphicsService(
        screen_tri, quad, blit, empty_mesh,
        Set(AbstractResource[
            screen_tri_poses, screen_tri,
            quad_poses, quad,
            blit,
            empty_mesh
        ])
    )
end


const CONTEXT_SERVICE_KEY_RESOURCES = :HELPERS_Resources


function get_basic_graphics()::BasicGraphicsService
    c = get_context()
    GL.@bp_gl_assert(exists(c), "Trying to get the 'BasicGraphicsService' service outside of a Context")
    return get_basic_graphics(c)
end
function get_basic_graphics(c::Context)::BasicGraphicsService
    return GL.try_register_service(c, CONTEXT_SERVICE_KEY_RESOURCES) do
        return GL.Service(
            BasicGraphicsService(),
            on_destroyed = (c_basic_graphics -> close.(c_basic_graphics.references))
        )
    end
end
export get_basic_graphics

"
Renders the given texure, using the given screen-quad transform
    and the given color transform on the texture's pixels.
"
function simple_blit(basic_graphics::BasicGraphicsService,
                     tex::Union{Texture, View},
                     context = get_context()
                     ;
                     quad_transform::fmat3x3 = m_identityf(3, 3),
                     color_transform::fmat4x4 = m_identityf(4, 4),
                     disable_depth_test::Bool = true,
                     manage_tex_view::Bool = true
                    )
    basic_graphics = get_basic_graphics(context)

    tex_view = (tex isa View) ? tex : get_view(tex)
    if manage_tex_view
        was_active::Bool = tex_view.is_active
        view_activate(tex_view)
    end

    set_uniform(basic_graphics.blit, "u_tex", tex_view)
    set_uniform(basic_graphics.blit, "u_mesh_transform", quad_transform)
    set_uniform(basic_graphics.blit, "u_color_map", color_transform)

    old_depth_test = context.state.depth_test
    if disable_depth_test
        set_depth_test(context, ValueTests.pass)
    end

    # If drawing full-screen, use the more efficient triangle.
    # If transforming it, use the more precise, intuitive quad.
    render_mesh(context,
                (quad_transform == m_identityf(3, 3)) ?
                    basic_graphics.screen_triangle :
                    basic_graphics.quad,
                basic_graphics.blit)

    if manage_tex_view && !was_active
        view_deactivate(tex_view)
    end

    set_depth_test(context, old_depth_test)
end
@inline function simple_blit(tex::Union{Texture, View},
                               context = get_context()
                               ;
                               kw_args...
                              )
    simple_blit(get_basic_graphics(context), tex, context; kw_args...)
end
export simple_blit