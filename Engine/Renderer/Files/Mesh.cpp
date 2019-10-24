#include "Mesh.h"
using namespace GL::File;


Mesh::Mesh(const std::string& file, std::string& outErrorMsg)
{
    //TODO: Implement.
}


Mesh::OglMesh::OglMesh(const std::string& name, size_t nVerts, size_t nIndices,
                       glm::vec3* poses, glm::vec2* uvs,
                       glm::vec3* normals, glm::vec3* tangents, glm::vec3* bitangents,
                       uint_fast32_t* indices)
    : OglIndexType((nVerts <= std::numeric_limits<uint16_t>::max()) ?
                       GL_UNSIGNED_SHORT :
                       GL_UNSIGNED_INT),
      Name(name)
{
    //TODO: Accept "null" values for any of the vertex data arrays.

    glCreateVertexArrays(1, &hVAO);
    glBindVertexArray(hVAO);

    //Set up the vertex buffers.
    int nextVboI = -1;
#define SET_UP_VBO(suffix, param, nFloats) \
    glCreateBuffers(1, &hVBO_##suffix); \
    glBindBuffer(GL_ARRAY_BUFFER, hVBO_##suffix); \
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * nFloats * nVerts, \
                 param, GL_STATIC_DRAW); \
    glEnableVertexAttribArray(++nextVboI); \
    glVertexAttribPointer(nextVboI, nFloats, GL_FLOAT, GL_FALSE, 0, 0);

    SET_UP_VBO(Pos, poses, 3);
    SET_UP_VBO(UV, uvs, 2);
    SET_UP_VBO(Normal, normals, 3);
    SET_UP_VBO(Tangent, tangents, 3);
    SET_UP_VBO(Bitangent, bitangents, 3);
#undef SET_UP_VBO

    //Set up the index buffer.
    glCreateBuffers(1, &hVBO_Indices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, hVBO_Indices);
    //If the indices will fit into a 16-bit int, use that.
    if (OglIndexType == GL_UNSIGNED_SHORT)
    {
        std::vector<uint16_t> indexData(nVerts);
        for (uint_fast32_t i = 0; i < nVerts; ++i)
            indexData[i] = (uint16_t)indices[i];
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * nVerts,
                     indexData.data(), GL_STATIC_DRAW);
    }
    else
    {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint_fast32_t) * nIndices,
                     indices, GL_STATIC_DRAW);
    }

    //Make sure the VAO is not modified from outside code.
    glBindVertexArray(0);
}
Mesh::OglMesh::~OglMesh()
{
    //If this instance was taken by a move operator, don't do anything.
    if (hVAO == NULL)
        return;

    GLuint VBOs[6] = {
        hVBO_Pos, hVBO_UV,
        hVBO_Normal, hVBO_Tangent, hVBO_Bitangent,
        hVBO_Indices
    };
    glDeleteBuffers(6, VBOs);

    glDeleteVertexArrays(1, &hVAO);
}

Mesh::OglMesh& Mesh::OglMesh::operator=(OglMesh&& from)
{
    hVAO = from.hVAO;
    from.hVAO = NULL;

    hVBO_Pos = from.hVBO_Pos;
    hVBO_UV = from.hVBO_UV;
    hVBO_Normal = from.hVBO_Normal;
    hVBO_Tangent = from.hVBO_Tangent;
    hVBO_Bitangent = from.hVBO_Bitangent;
    hVBO_Indices = from.hVBO_Indices;
}