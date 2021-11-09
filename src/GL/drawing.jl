
#########################
#        Clearing       #
#########################


"
Clears the given render target (or the screen, if you pass a default/null handle).
Optionally clears only a single attachment of the target.
"
function render_clear( context::Context,
                       target::Ptr_Target,
                       color::vRGBA{T},
                       attachment_idx::Optional{Int} = nothing
                     ) where {T<:Union{GLint, GLuint, GLfloat}}
    draw_buffer::GLint = exists(attachment_idx) ? attachment_idx : GL_NONE
    if color isa vRGBA{GLint}
        glClearNamedFramebufferiv(target, GL_COLOR, draw_buffer, Ref(color.data))
    elseif color isa vRGBA{GLuint}
        glClearNamedFramebufferuv(target, GL_COLOR, draw_buffer, Ref(color.data))
    elseif color isa vRGBA{GLfloat}
        glClearNamedFramebufferfv(target, GL_COLOR, draw_buffer, Ref(color.data))
    else
        error("Unexpected color component type: ", T)
    end
end
"
Clears the given depth render target (or the screen's depth buffer, if you pass a default/null handle).
Optionally clears only a single attachment of the target.
"
function render_clear( context::Context,
                       target::Ptr_Target,
                       depth::GLfloat,
                       attachment_idx::Optional{Int} = nothing
                     )
    draw_buffer::GLint = exists(attachment_idx) ? attachment_idx : GL_NONE
    glClearNamedFramebufferfv(target, GL_DEPTH, draw_buffer, Ref(depth))
end
"
Clears the given stencil render target (or the screen's stencil buffer, if you pass a default/null handle).
Optionally clears only a single attachment of the target.
"
function render_clear( context::Context,
                       target::Ptr_Target,
                       stencil::GLint,  #TODO: Should this take a GLuint instead, since that's more intuitive? Same question for the below overload.
                       attachment_idx::Optional{Int} = nothing
                     )
    draw_buffer::GLint = exists(attachment_idx) ? attachment_idx : GL_NONE
    glClearNamedFramebufferiv(target, GL_STENCIL, draw_buffer, Ref(stencil))
end
"
Clears the given depth/stencil render target (or the screen's depth/stencil buffer, if you pass a default/null handle).
Optionally clears only a single attachment of the target.
"
function render_clear( context::Context,
                       target::Ptr_Target,
                       depth::GLfloat,
                       stencil::GLint,
                       attachment_idx::Optional{Int} = nothing
                     )
    draw_buffer::GLint = exists(attachment_idx) ? attachment_idx : GL_NONE
    glClearNamedFramebufferfi(target, GL_DEPTH_STENCIL, draw_buffer, depth, stencil)
end
export render_clear


########################
#        Drawing       #
########################

#TODO: Take the shader program as a parameter

"Extra parameters used for drawing indexed meshes"
struct DrawIndexed
    # An index of this value signals OpenGL to restart the drawing primitive
    #   (e.x. if occurred within a triangle_strip, a new strip is started).
    # Has no effect for primitive types which aren't linked, like point, triangle, or line.
    reset_value::Optional{GLuint}

    # All index values are offset by this amount.
    # Does not affect the 'reset_value' above; that test happens before this offset is applied.
    value_offset::UInt
end

DrawIndexed(; reset_value = nothing, value_offset = zero(UInt)) = DrawIndexed(reset_value, value_offset)


"
Renders a mesh using the currently-active shader program.

IMPORTANT NOTE: All counting/indices follow Julia's 1-based convention.
Under the hood, they will get converted to 0-based for OpenGL.

You can manually disable the use of indexed rendering for an indexed mesh
    by passing 'indexed_params=nothing'.

You can configure one (and only one) of the following optional features:
1. Instanced rendering (by setting 'instances' to a range like `IntervalU(1, N)`)
2. Multi-draw (by setting 'elements' to a list of ranges, instead of a single range).
3. An optimization hint (by setting 'known_vertex_range') about what vertices
     are actually going to be drawn with indexed rendering,
     to help the GPU driver optimize memory usage.

NOTE: There is one small, relatively-esoteric OpenGL feature that is not included
   (adding it would probably require separating this function into 3 different ones):
   indexed multi-draw could use different index offsets for each subset of elements.
"
function render_mesh( mesh::Mesh
                      ;
                      shape::E_PrimitiveTypes = mesh.type,
                      indexed_params::Optional{DrawIndexed} =
                          exists(mesh.index_data) ? DrawIndexed() : nothing,

                      elements::Union{IntervalU, TMultiDraw} =
                          IntervalU(1,
                                    exists(indexed_params) ?
                                        count_mesh_elements(mesh) :
                                        count_mesh_vertices(mesh)),

                      instances::Optional{IntervalU} = nothing,
                      known_vertex_range::Optional{IntervalU} = nothing
                    ) where {TMultiDraw<:Union{AbstractVector{IntervalU}, ConstVector{IntervalU}}}
    @bp_check(isnothing(indexed_params) || exists(mesh.index_data),
              "Trying to render with indexed_params but the mesh has no indices")

    # Several of the optional drawing parameters are mutually exclusive.
    n_used_features::Int = (exists(indexed_params) ? 1 : 0) +
                           ((elements isa IntervalU) ? 0 : 1) +
                           (exists(known_vertex_range) ? 1 : 0)
    @bp_check(n_used_features <= 1,
              "Trying to use more than one of the mutually-exclusive features: ",
                "instancing, multi-draw, and known_vertex_range!")

    #=
     The notes I took when preparing the old C++ draw calls interface:

     All draw modes:
        * Normal              "glDrawArrays()" ("first" element index and "count" elements)
        * Normal + Multi-Draw "glMultiDrawArrays()" (multiple Normal draws from the same buffer data)
        * Normal + Instance   "glDrawArraysInstanced()" (draw multiple instances of the same mesh).
             should actually use "glDrawArraysInstancedBaseInstance()" to support an offset for the first instance to use

        * Indexed              "glDrawElements()" (draw indices instead of vertices)
        * Indexed + Multi-Draw "glMultiDrawElements()"
        * Indexed + Instance   "glDrawElementsInstanced()" (draw multiple instances of the same indexed mesh).
             should actually use "glDrawElementsInstancedBaseInstance()" to support an offset for the first instance to use
        * Indexed + Range      "glDrawRangeElements()" (provide the known range of indices that could be drawn, for driver optimization)

        * Indexed + Base Index              "glDrawElementsBaseVertex()" (an offset for all indices)
        * Indexed + Base Index + Multi-Draw "glMultiDrawElementsBaseVertex()" (each element of the multi-draw has a different "base index" offset)
        * Indexed + Base Index + Range      "glDrawRangeElementsBaseVertex()"
        * Indexed + Base Index + Instanced  "glDrawElementsInstancedBaseVertex()"
             should actually use "glDrawElementsInstancedBaseVertexBaseInstance()" to support an offset for the first instance to use

     All Indexed draw modes can have a "reset index", which is
         a special index value to reset for continuous fan/strip primitives
    =#

    if exists(indexed_params)
        # Configure the "primitive restart" index.
        if exists(indexed_params.reset_value)
            glEnable(GL_PRIMITIVE_RESTART)
            glPrimitiveRestartIndex(indexed_params.reset_value)
        else
            glDisable(GL_PRIMITIVE_RESTART)  #TODO: is this call expensive? We could cache it and only call when it really changes
        end
        # Pre-compute data.
        index_type = get_index_ogl_enum(mesh.index_data[2])
        index_byte_size = sizeof(get_index_type(mesh.index_data[2]))
        # Make the draw calls.
        if elements isa AbstractArray{IntervalU}
            offsets = ntuple(i -> index_byte_size * (elements[i].min - 1), length(elements))
            counts = ntuple(i -> elements[i].size, length(elements))
            value_offsets = ntuple(i -> indexed_params.value_offset, length(elements))
            glMultiDrawElementsBaseVertex(shape,
                                          Ref(counts),
                                          index_type,
                                          Ref(offsets),
                                          length(elements),
                                          Ref(value_offsets))
        elseif indexed_params.value_offset == 0
            if exists(instances)
                if instances.min == 1
                    glDrawElementsInstanced(shape, elements.size,
                                            index_type,
                                            index_byte_size * (elements.min - 1),
                                            instances.size)
                else
                    glDrawElementsInstancedBaseInstance(shape, elements.size,
                                                        index_type,
                                                        index_byte_size * (elements.min - 1),
                                                        instances.size,
                                                        instances.min - 1)
                end
            elseif exists(known_vertex_range)
                glDrawRangeElements(shape,
                                    known_vertex_range.min - 1,
                                    max_inclusive(known_vertex_range) - 1,
                                    elements.size,
                                    index_type,
                                    index_byte_size * (elements.min - 1))
            else
                glDrawElements(shape, elements.size,
                               index_type,
                               index_byte_size * (elements.min - 1))
            end
        else
            if exists(instances)
                if instances.min == 1
                    glDrawElementsInstancedBaseVertex(shape, elements.size,
                                                      index_type,
                                                      index_byte_size * (elements.min - 1),
                                                      instances.size,
                                                      indexed_params.value_offset)
                else
                    glDrawElementsInstancedBaseVertexBaseInstance(shape, elements.size,
                                                                  index_type,
                                                                  index_byte_size * (elements.min - 1),
                                                                  instances.size,
                                                                  indexed_params.value_offset,
                                                                  instances.min - 1)
                end
            elseif exists(known_vertex_range)
                glDrawRangeElementsBaseVertex(shape,
                                              known_vertex_range.min - 1,
                                              max_inclusive(known_vertex_range) - 1,
                                              elements.size,
                                              index_type,
                                              index_byte_size * (elements.min - 1),
                                              indexed_params.value_offset)
            else
                glDrawElementsBaseVertex(shape, elements.size,
                                         index_type,
                                         index_byte_size * (elements.min - 1),
                                         indexed_params.value_offset)
            end
        end
    else
        if elements isa AbstractArray{IntervalU}
            offsets = ntuple(i -> elements[i].min - 1, length(elements))
            counts = ntuple(i -> elements[i].size, length(elements))
            glMultiDrawArrays(shape, Ref(offsets), Ref(counts), length(elements))
        elseif exists(instances)
            if instances.min == 1
                glDrawArraysInstanced(shape, elements.min - 1, elements.size, instances.size)
            else
                glDrawArraysInstancedBaseInstance(shape, elements.min - 1, elements.size,
                                                  instances.size, instances.min - 1)
            end
        else
            glDrawArrays(shape, elements.min, elements.size)
        end
    end
end

#TODO: Indirect drawing: glDrawArraysIndirect(), glMultiDrawArraysIndirect(), glDrawElementsIndirect(), and glMultiDrawElementsIndirect().

export DrawIndexed, render_mesh