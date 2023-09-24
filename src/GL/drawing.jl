
#########################
#        Clearing       #
#########################

"Clears the screen's color to the given value"
function clear_screen(color::vRGBA{<:Union{GLint, GLuint, GLfloat}})
    @bp_gl_assert(exists(get_context()), "Clearing render target from a non-rendering thread")
    if color isa vRGBA{GLint}
        glClearNamedFramebufferiv(Ptr_Target(), GL_COLOR, 0, Ref(color.data))
    elseif color isa vRGBA{GLuint}
        glClearNamedFramebufferuv(Ptr_Target(), GL_COLOR, 0, Ref(color.data))
    elseif color isa vRGBA{GLfloat}
        glClearNamedFramebufferfv(Ptr_Target(), GL_COLOR, 0, Ref(color.data))
    else
        error("Unexpected color component type: ", T)
    end
end

"Clears the screen's depth buffer to the given value"
function clear_screen(depth::GLfloat)
    @bp_gl_assert(exists(get_context()), "Clearing the screen from a non-rendering thread")
    glClearNamedFramebufferfv(Ptr_Target(), GL_DEPTH, 0, Ref(depth))
end

"Clears the screen's stencil buffer to the given value"
function clear_screen(stencil::GLuint)
    @bp_gl_assert(exists(get_context()), "Clearing the screen from a non-rendering thread")
    glClearNamedFramebufferiv(Ptr_Target(), GL_STENCIL, 0,
                              Ref(reinterpret(GLint, stencil)))
end

"Clears the screen's hybrid-depth-stencil buffer to the given value"
function clear_screen(depth::GLfloat, stencil::GLuint)
    @bp_gl_assert(exists(get_context()), "Clearing the screen from a non-rendering thread")
    glClearNamedFramebufferfi(Ptr_Target(), GL_DEPTH_STENCIL, 0,
                              depth, reinterpret(GLint, stencil))
end

export clear_screen


########################
#        Drawing       #
########################

"Extra parameters used for drawing indexed meshes"
Base.@kwdef struct DrawIndexed
    # An index of this value signals OpenGL to restart the drawing primitive
    #   (e.x. if occurred within a triangle_strip, a new strip is started).
    # Has no effect for primitive types which aren't linked, like point, triangle, or line.
    reset_value::Optional{GLuint} = nothing

    # All index values are offset by this amount.
    # Does not affect the 'reset_value' above; that test happens before this offset is applied.
    value_offset::UInt = zero(UInt)
end


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
function render_mesh( mesh::Mesh, program::Program
                      ;
                      #TODO: Can't do type inference with named parameters! This creates garbage and slowdown.
                      shape::E_PrimitiveTypes = mesh.type,
                      indexed_params::Optional{DrawIndexed} =
                          exists(mesh.index_data) ? DrawIndexed() : nothing,
                      elements::Union{IntervalU, TMultiDraw} = IntervalU((
                          min=1,
                          size=exists(indexed_params) ?
                                   count_mesh_elements(mesh) :
                                   count_mesh_vertices(mesh)
                      )),
                      instances::Optional{IntervalU} = nothing,
                      known_vertex_range::Optional{IntervalU} = nothing
                    ) where {TMultiDraw<:Union{AbstractVector{IntervalU}, ConstVector{IntervalU}}}
    context::Context = get_context()
    @bp_check(isnothing(indexed_params) || exists(mesh.index_data),
              "Trying to render with indexed_params but the mesh has no indices")

    # Several of the optional drawing parameters are mutually exclusive.
    n_used_features::Int = (exists(indexed_params) ? 1 : 0) +
                           ((elements isa IntervalU) ? 0 : 1) +
                           (exists(known_vertex_range) ? 1 : 0)
    @bp_check(n_used_features <= 1,
              "Trying to use more than one of the mutually-exclusive features: ",
                "instancing, multi-draw, and known_vertex_range!")

    # Let the View-Debugger know that we are rendering with this program.
    service_ViewDebugging_check(get_ogl_handle(program))

    # Activate the mesh and program.
    glUseProgram(program.handle)
    setfield!(context, :active_program, program.handle)
    glBindVertexArray(mesh.handle)
    setfield!(context, :active_mesh, mesh.handle)

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
            glDisable(GL_PRIMITIVE_RESTART)
        end
        # Pre-compute data.
        index_type = get_index_ogl_enum(mesh.index_data.type)
        index_byte_size = sizeof(mesh.index_data.type)
        # Make the draw calls.
        if elements isa AbstractArray{IntervalU}
            offsets = ntuple(i -> index_byte_size * (min_inclusive(elements[i]) - 1), length(elements))
            counts = ntuple(i -> size(elements[i]), length(elements))
            value_offsets = ntuple(i -> indexed_params.value_offset, length(elements))
            glMultiDrawElementsBaseVertex(shape,
                                          Ref(counts),
                                          index_type,
                                          Ref(offsets),
                                          length(elements),
                                          Ref(value_offsets))
        else
            # OpenGL has a weird requirement that the index offset be a void*,
            #    not a simple integer.
            index_byte_offset = Ptr{Cvoid}(index_byte_size * (min_inclusive(elements) - 1))

            if indexed_params.value_offset == 0
                if exists(instances)
                    if min_inclusive(instances) == 1
                        glDrawElementsInstanced(shape, size(elements),
                                                index_type,
                                                index_byte_offset,
                                                size(instances))
                    else
                        glDrawElementsInstancedBaseInstance(shape, size(elements),
                                                            index_type,
                                                            index_byte_offset,
                                                            size(instances),
                                                            min_inclusive(instances) - 1)
                    end
                elseif exists(known_vertex_range)
                    glDrawRangeElements(shape,
                                        min_inclusive(known_vertex_range) - 1,
                                        max_inclusive(known_vertex_range) - 1,
                                        size(elements),
                                        index_type,
                                        index_byte_offset)
                else
                    glDrawElements(shape, size(elements),
                                   index_type,
                                   index_byte_offset)
                end
            else
                if exists(instances)
                    if min_inclusive(instances) == 1
                        glDrawElementsInstancedBaseVertex(shape, size(elements),
                                                          index_type,
                                                          index_byte_offset,
                                                          size(instances),
                                                          indexed_params.value_offset)
                    else
                        glDrawElementsInstancedBaseVertexBaseInstance(shape, size(elements),
                                                                      index_type,
                                                                      index_byte_offset,
                                                                      size(instances),
                                                                      indexed_params.value_offset,
                                                                      min_inclusive(instances) - 1)
                    end
                elseif exists(known_vertex_range)
                    glDrawRangeElementsBaseVertex(shape,
                                                  min_inclusive(known_vertex_range) - 1,
                                                  max_inclusive(known_vertex_range) - 1,
                                                  size(elements),
                                                  index_type,
                                                  index_byte_offset,
                                                  indexed_params.value_offset)
                else
                    glDrawElementsBaseVertex(shape, size(elements),
                                             index_type,
                                             index_byte_offset,
                                             indexed_params.value_offset)
                end
            end
        end
    else
        if elements isa AbstractArray{IntervalU}
            offsets = ntuple(i -> min_inclusive(elements[i]) - 1, length(elements))
            counts = ntuple(i -> size(elements[i]), length(elements))
            glMultiDrawArrays(shape, Ref(offsets), Ref(counts), length(elements))
        elseif exists(instances)
            if min_inclusive(instances) == 1
                glDrawArraysInstanced(shape,
                                      min_inclusive(elements) - 1, size(elements),
                                      size(instances))
            else
                glDrawArraysInstancedBaseInstance(shape,
                                                  min_inclusive(elements) - 1, size(elements),
                                                  size(instances), min_inclusive(instances) - 1)
            end
        else
            glDrawArrays(shape, min_inclusive(elements) - 1, size(elements))
        end
    end
end

export DrawIndexed, render_mesh


############################
#     Compute Dispatch     #
############################

"Dispatches the given compute shader with enough work-groups for the given number of threads"
function dispatch_compute_threads(program::Program, count::Vec3{<:Integer}
                                  ; context::Context = get_context())
    @bp_check(exists(program.compute_work_group_size),
              "Program isn't a compute shaer: " + get_ogl_handle(program))
    group_size::v3u = program.compute_work_group_size
    dispatch_compute_groups(program,
                            round_up_to_multiple(count, group_size) ÷ group_size
                            ; context=context)
end

"Dispatches the given commpute shader with the given number of work-groups"
function dispatch_compute_groups(program::Program, count::Vec3{<:Integer}
                                 ; context::Context = get_context())
    # Activate the program.
    glUseProgram(get_ogl_handle(program))
    setfield!(context, :active_program, get_ogl_handle(program))

    glDispatchCompute(convert(Vec{3, GLuint}, count)...)
end

export dispatch_compute_threads, dispatch_compute_groups