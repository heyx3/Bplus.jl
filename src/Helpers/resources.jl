"
A Context service which defines a bunch of common, simple, useful resources:

 * `screen_triangle` : A 1-triangle mesh with 2D positions in NDC-space.
                       When drawn, it will perfectly cover the entire viewport,
                          making it easy to spin up post-processing effects.
                       The UV coordinates can be calculated from the XY positions
                          (or from gl_FragCoord).
 * `quad` : A 2-triangle mesh describing a square, with 2D coordinates
                in the range (-1, -1) to (+1, +1).
            This _can_ be used for post-processing effects, but it's less efficient
                than `screen_triangle`, for technical reasons.
 * `blit` : A simple shader to render a 2D texture (e.x. copy a Target to the screen).
            Refer to `resource_blit()`.
 * `empty_mesh` : A mesh with no vertex data, for dispatching entirely procedural geometry.
                  Has the 'points' PrimitiveType.
"
mutable struct CResources
    screen_triangle::Mesh
    quad::Mesh
    blit::Program
    empty_mesh::Mesh
    references::Set{Resource}
end
export CResources

function CResources()
    screen_tri_poses = Buffer(false, map(Vec{2, Int8}, [
        (-1, -1),
        (3, -1),
        (-1, 3)
    ]))
    screen_tri = Mesh(PrimitiveTypes.triangle_strip,
                      [ VertexDataSource(screen_tri_poses, sizeof(Vec{2, Int8})) ],
                      [ VertexAttribute(1, 0x0, VertexData_FVector(2, Int8, false)) ])

    quad_poses = Buffer(false, map(Vec{2, Int8}, [
        (-1, -1),
        (1, -1),
        (-1, 1),
        (1, 1)
    ]))
    quad = Mesh(PrimitiveTypes.triangle_strip,
                [ VertexDataSource(quad_poses, sizeof(Vec{2, Int8})) ],
                [ VertexAttribute(1, 0x0, VertexData_FVector(2, Int8, false)) ])

    #TODO: Bool uniforms to avoid the matrix math if they're identity matrices.
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

    return CResources(
        screen_tri, quad, blit, empty_mesh,
        Set(Resource[
            screen_tri_poses, screen_tri,
            quad_poses, quad,
            blit,
            empty_mesh
        ])
    )
end


const CONTEXT_SERVICE_KEY_RESOURCES = :HELPERS_Resources


function get_resources()::CResources
    c = get_context()
    GL.@bp_gl_assert(exists(c), "Trying to get the 'CResources' service outside of a Context")
    return get_resources(c)
end
function get_resources(c::Context)::CResources
    return GL.try_register_service(c, CONTEXT_SERVICE_KEY_RESOURCES) do
        return GL.Service(
            CResources(),
            on_destroyed = (c_resources -> close.(c_resources.references))
        )
    end
end
export get_resources

"
Renders the given texure, using the given screen-quad transform
    and the given color transform on the texture's pixels.
"
function resource_blit(resources::CResources,
                       tex::Union{Texture, View},
                       context = get_context()
                       ;
                       quad_transform::fmat3x3 = m_identityf(3, 3),
                       color_transform::fmat4x4 = m_identityf(4, 4),
                       disable_depth_test::Bool = true,
                       manage_tex_view::Bool = true
                      )
                      view_activate
    resources = get_resources(context)

    tex_view = get_view(tex)
    if manage_tex_view
        was_active::Bool = tex_view.is_active
        view_activate(tex_view)
    end

    set_uniform(resources.blit, "u_tex", tex_view)
    set_uniform(resources.blit, "u_mesh_transform", quad_transform)
    set_uniform(resources.blit, "u_color_map", color_transform)

    old_depth_test = context.state.depth_test
    if disable_depth_test
        set_depth_test(context, ValueTests.Pass)
    end

    # If drawing full-screen, use the more efficient triangle.
    # If transforming it, use the more precise, intuitive quad.
    render_mesh(context,
                (quad_transform == m_identityf(3, 3)) ?
                    resources.screen_triangle :
                    resources.quad,
                resources.blit)

    if manage_tex_view && !was_active
        view_deactivate(tex_view)
    end

    set_depth_test(context, old_depth_test)
end
@inline function resource_blit(tex::Union{Texture, View},
                               context = get_context()
                               ;
                               kw_args...
                              )
    resource_blit(get_resources(context), tex, context; kw_args...)
end
export resource_blit