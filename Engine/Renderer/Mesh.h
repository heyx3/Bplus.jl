#pragma once

#include "../../RenderLibs.h"

#include <vector>


namespace GL::File
{
    class Mesh
    {
    public:

        Mesh(const std::string& file, std::string& outErrorMsg);

        uint_fast32_t GetNSubmeshes() const { return subMeshes.size(); }
        const std::string& GetSubmeshName(int i) const { return subMeshes[i].Name; }
        uint_fast32_t FindSubmeshByName(const std::string& name)
        {
            for (uint_fast32_t i = 0; i < subMeshes.size(); ++i)
            {
                if (subMeshes[i].Name == name)
                    return i;
            }
            return 0;
        }


    private:
        struct OglMesh
        {
            const std::string Name;
            const int OglIndexType;

            OglMesh(const std::string& name, size_t nVerts, size_t nIndices,
                    glm::vec3* poses, glm::vec2* uvs,
                    glm::vec3* normals, glm::vec3* tangents, glm::vec3* bitangents,
                    uint_fast32_t* indices);
            ~OglMesh();
            
            OglMesh(OglMesh&& from)
                : Name(std::move(from.Name)), OglIndexType(from.OglIndexType)
            { *this = std::move(from); }
            OglMesh& operator=(OglMesh&& from);

            GLuint hVAO,
                   hVBO_Pos, hVBO_UV, hVBO_Normal, hVBO_Tangent, hVBO_Bitangent,
                   hVBO_Indices;
        };

        std::vector<OglMesh> subMeshes;
    };
}