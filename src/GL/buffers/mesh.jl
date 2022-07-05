###############################
#       VertexDataSource      #
###############################

"
A reference to an array of data stored in a Buffer.
The elements of the array could be numbers, vectors, matrices,
   or entire structs.
"
struct VertexDataSource
    buf::Buffer
    element_byte_size::UInt
    buf_byte_offset::UInt

    #TODO: ConstVector of internal buffers of data, which can be set up in the constructor
end

VertexDataSource(buf, element_byte_size::Integer, buf_byte_offset::Integer = 0) =
    VertexDataSource(buf, UInt(element_byte_size), UInt(buf_byte_offset))

"Gets the maximum number of elements which can be pulled from the given data source"
function get_max_elements(data::VertexDataSource)
    @bp_gl_assert(data.buf.byte_size >= data.buf_byte_offset,
                  "VertexDataSource's buffer is ", data.buf.byte_size,
                    ", but the byte offset is ", data.buf_byte_offset)
    max_bytes = data.buf.byte_size - data.buf_byte_offset
    return max_bytes ÷ data.element_byte_size
end

export VertexDataSource, get_max_elements


##############################
#       VertexAttribute      #
##############################

"Pulls data out of the elements of a VertexDataSource"
struct VertexAttribute
    # A reference to the VertexDataSource, as its index in some list.
    data_source_idx::Int
    # The byte offset from each element of the VertexDataSource
    #    to the specific field inside it.
    field_byte_offset::UInt
    # The type of this data.
    field_type::Type{<:VertexData}

    # If 0, this data is regular old per-vertex data.
    # If 1, this data is per-innstance, for instanced rendering.
    # If 2, this data is per-instance, and each element is reused for 2 consecutive instances.
    # etc.
    per_instance::UInt
end

VertexAttribute(data_source_idx::Integer,
                field_byte_offset::Integer,
                field_type::Type{<:VertexData},
                per_instance::Integer = 0) =
    VertexAttribute(Int(data_source_idx), UInt(field_byte_offset),
                    field_type, UInt(per_instance))

export VertexAttribute


#########################
#       Other Data      #
#########################

# The different kinds of shapes that mesh vertices can represent.
@bp_gl_enum(PrimitiveTypes::GLenum,
    # Each vertex is a screen-space square.
    point = GL_POINTS,
    # Each pair of vertices is a line.
    # If the number of vertices is odd, the last one is ignored.
    line = GL_LINES,
    # Each triplet of vertices is a triangle.
    # If the number of vertices is not a multiple of three, the last one/two are ignored.
    triangle = GL_TRIANGLES,

    # Each vertex creates a line reaching backwards to the previous vertex.
    # If there's only one vertex, no lines are created.
    line_strip_open = GL_LINE_STRIP,
    # Each vertex creates a line reaching backwards to the previous vertex.
    # The last vertex also reaches out to the first, creating a closed loop.
    # If there's only one vertex, no lines are created.
    line_strip_closed = GL_LINE_LOOP,

    # Each new vertex creates a triangle with the two previous vertices.
    # If there's only one or two vertices, no triangles are created.
    triangle_strip = GL_TRIANGLE_STRIP,
    # Each new vertex creates a triangle with its previous vertex plus the first vertex.
    # If there's only one or two vertices, no triangles are created.
    triangle_fan = GL_TRIANGLE_FAN
);


"The set of valid types for mesh indices"
const MeshIndexTypes = Union{UInt8, UInt16, UInt32}

"Gets the OpenGL enum corresponding to mesh index data of the given type"
get_index_ogl_enum(::Type{UInt8}) = GL_UNSIGNED_BYTE
get_index_ogl_enum(::Type{UInt16}) = GL_UNSIGNED_SHORT
get_index_ogl_enum(::Type{UInt32}) = GL_UNSIGNED_INT
"Gets the OpenGL enum corresponding to mesh index data of the given size"
get_index_ogl_enum(byte_size::Integer) = get_index_ogl_enum(get_index_type(byte_size))

"Gets the mesh index type, given its byte size"
get_index_type(byte_size::Integer) =
    if byte_size == 1
        UInt8
    elseif byte_size == 2
        UInt16
    elseif byte_size == 4
        UInt32
    else
        error("Not a valid size for mesh index data: ", byte_size)
    end


export MeshIndexTypes, PrimitiveTypes, E_PrimitiveTypes
export get_index_type, get_index_ogl_enum


###################
#       Mesh      #
###################

"
A collection of vertices (and optionally indices),
   representing geometry that the GPU can render.
The vertex data and indices are taken from a set of Buffers.
This is what OpenGL calls a 'VAO' or 'VertexArrayObject'.
Most data will be per-'vertex', but some data can be
   per-instance, for instanced rendering.
"
mutable struct Mesh <: Resource
    handle::Ptr_Mesh
    type::E_PrimitiveTypes

    vertex_data_sources::ConstVector{VertexDataSource}
    vertex_data::ConstVector{VertexAttribute}

    # The source of the index data, if this mesh uses indices.
    index_data::Optional{Tuple{Buffer, Type{<:MeshIndexTypes}}}
end

function Mesh( type::E_PrimitiveTypes,
               vertex_sources::AbstractVector{VertexDataSource},
               vertex_fields::AbstractVector{VertexAttribute},
               index_data::Optional{Tuple{Buffer, DataType}} = nothing
             )
    @bp_check(exists(get_context()), "Trying to create a Mesh outside a GL Context")
    m::Mesh = Mesh(Ptr_Mesh(get_from_ogl(gl_type(Ptr_Mesh), glCreateVertexArrays, 1)),
                   type,
                   ntuple(i -> vertex_sources[i], length(vertex_sources)),
                   ntuple(i -> vertex_fields[i], length(vertex_fields)),
                   index_data)

    # Configure the index buffer.
    if exists(index_data)
        @bp_check(index_data[2] <: MeshIndexTypes,
                  "Index type ", index_data[2], " is not <: ", MeshIndexTypes)
        glVertexArrayElementBuffer(m.handle, index_data[1].handle)
    end

    # Configure the vertex buffers.
    for i::Int in 1:length(vertex_sources)
        glVertexArrayVertexBuffer(m.handle, GLuint(i - 1),
                                  vertex_sources[i].buf.handle,
                                  vertex_sources[i].buf_byte_offset,
                                  vertex_sources[i].element_byte_size)
    end

    # Configure the vertex data.
    #TODO: Check that we can do this with one loop instead of three
    for i::Int in 1:length(vertex_fields)
        glEnableVertexArrayAttrib(m.handle, GLuint(i - 1))
    end
    vert_attrib_idx::GLuint = zero(GLuint)
    for (i::Int, vert::VertexAttribute) in enumerate(vertex_fields)
        # The OpenGL calls to set up vertex attributes have mostly the same arguments,
        #   regardless of the field type.
        # One exception is the 'isNormalized' parameter.
        normalized_args = ()
        get_gl_args() = (m.handle, vert_attrib_idx,
                         count_components(vert.field_type),
                         get_component_ogl_enum(vert.field_type),
                         normalized_args...,
                         vert.field_byte_offset)

        # Pick the correct OpenGL call for this type of vertex data.
        gl_func = nothing
        if vert.field_type <: VertexData_IVector
            gl_func = glVertexArrayAttribIFormat
        elseif vert.field_type <: VertexData_DVector
            gl_func = glVertexArrayAttribLFormat
        elseif vert.field_type <: VertexData_MatrixF
            normalized_args = tuple(GL_FALSE)
            gl_func = glVertexArrayAttribFormat
        elseif vert.field_type <: VertexData_MatrixD
            gl_func = glVertexArrayAttribLFormat
        elseif vert.field_type <: VertexData_FVector
            normalized_args = tuple(vert_data_is_normalized(vert.field_type) ? GL_TRUE : GL_FALSE)
            gl_func = glVertexArrayAttribFormat
        else
            error("Unhandled case: ", vert.field_type)
        end

        # Make the OpenGL calls, and increment the vertex attribute counter.
        for _ in 1:count_attribs(vert.field_type)
            gl_func(get_gl_args()...)
            glVertexArrayBindingDivisor(m.handle, vert_attrib_idx, vert.per_instance)
            vert_attrib_idx += 1
        end
    end
    for (i::Int, vert::VertexAttribute) in enumerate(vertex_fields)
        glVertexArrayAttribBinding(m.handle, GLuint(i - 1), GLuint(vert.data_source_idx - 1))
    end

    return m
end

#TODO: A singleton in the Context for an 'empty' mesh


function Base.close(m::Mesh)
    @bp_check(m.handle != Ptr_Mesh(), "Already closed this Mesh")
    glDeleteVertexArrays(1, Ref{GLuint}(m.handle))
    setfield!(m, :handle, Ptr_Mesh())
end

export Mesh


##############################
#       Mesh Operations      #
##############################

"
Gets the maximum number of vertices this mesh can offer for rendering,
   based only on vertex data (not index data).
"
function count_mesh_vertices(m::Mesh)::Int
    # Find the smallest set of per-vertex data within this mesh.
    n_elements::Optional{Int} = nothing
    for vertex_data::VertexAttribute in m.vertex_data
        if vertex_data.per_instance == 0
            buf_source::VertexDataSource = m.vertex_data_sources[vertex_data.data_source_idx]
            n_vert_elements = get_max_elements(buf_source)
            if isnothing(n_elements) || (n_vert_elements < n_vert_elements)
                n_elements = n_vert_elements
            end
        end
    end

    # Edge-case: if there's no per-vertex data (meaning it's all per-instance?), return 0
    return exists(n_elements) ? n_elements : 0
end

"
Gets the maximum number of vertices (or indices, if indexed)
  this mesh can offer for rendering.z
"
function count_mesh_elements(m::Mesh)::Int
    if exists(m.index_data)
        return m.index_data[1].byte_size ÷ sizeof(m.index_data[2])
    else
        return count_mesh_vertices(m)
    end
end


println("#TODO: Operations to change vertex data")

"Adds/changes the index data for this mesh"
function set_index_data(m::Mesh, source::Buffer, type::Type{<:MeshIndexTypes})
    setfield!(m, :index_data, (source, type))
    glVertexArrayElementBuffer(m.handle, source.buf.handle)
end

"
Removes any existing index data from this Mesh,
   so that its vertices are no longer indexed.
Does nothing if it already wasn't indexed.
"
function remove_index_data(m::Mesh)
    setfield!(m, :index_data, nothing)
    glVertexArrayElementBuffer(m.handle, Ptr_Buffer())
end

export count_mesh_vertices, count_mesh_elements, set_index_data, remove_index_data