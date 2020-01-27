//This is an AUTO-GENERATED temporary GLSL file for editing
//    as part of the Bplus Material editor.
//DO NOT modify outside of the marked areas far below;
//    it won't accomplish anything.

//=========================================
//           Vertex Shader:
//=========================================
#version 440 core

//Graphics settings configuration:
//General graphics quality level, from 1 to 5:
uniform uint s_Quality;


//Vertex inputs:
layout (location = 0) in vec3 vIn_Pos;
layout (location = 1) in vec3 vIn_Color;

//...user configuration goes here...

//Vertex outputs:
out vec3 vOut_Color;


//Built-in structures:
struct S_Light_Params {
    vec3 Color;
    float AmbientIntensity, DiffuseIntensity, SpecularIntensity;
};
struct S_Light_Directional {
    vec3 Dir;
    S_Light_Params Data;
};
struct S_Light_Point {
    vec3 Pos;
    S_Light_Params Data;
};


//Built-in uniforms:
uniform float u_Aspect; //Width over height of the render target/screen.
uniform uvec2 u_ScreenSize; //Size in pixels of the render target/screen.

vec2 GetScreenUV(vec2 pixelPos) { return pixelPos / vec2(u_ScreenSize); }

//Point transformations:
vec4 H_TransformPoint_Full(mat4 transf, vec3 p) { return transf          * vec4(p, 1.0f); }
vec3 H_TransformPoint_Fast(mat4 transf, vec3 p) { return (mat4x3(transf) * vec4(p, 1.0f)).xyz; }
vec3 H_TransformPoint     (mat3 transf, vec3 p) { return transf          * p; }

//Vector transformations:
vec4 H_TransformVector_Full(mat4 transf, vec3 v) { return mat3x4(transf) * v; }
vec3 H_TransformVector_Fast(mat4 transf, vec3 v) { return mat3x3(transf) * v; }
vec3 H_TransformVector     (mat3 transf, vec3 v  { return transf         * v; }

//Normal-map helpers:
vec3 H_UnpackNormalMap(vec4 map)                                        { return -1.0f + (2.0f * map.rgb); }
vec3 H_UnpackNormalMapN(vec4 map)                                       { return normalize(H_UnpackNormalMap(map)); }
mat3 H_GetNormalMapTransform(vec3 normal, vec3 tangent, vec3 bitangent) { return mat3(tangent, bitangent, normal); }
mat3 H_GetNormalMapTransform()                                          { return H_GetNormalMapTransform(vIn_Normal, vIn_Tangent, vIn_Bitangent); }
mat3 H_GetInverseNormalMapTransform(mat3 normalMapTransform)            { return transpose(normalMapTransform); }
void H_FixTangents(vec3 normal, inout vec3 tangent, out vec3 bitangent) {
    tangent = normalize(tangent - (dot(tangent, normal) * normal));
    bitangent = cross(normal, tangent);
}

//Lighting:
vec3 H_GetLightColor(S_Light_Params light,
                     vec3 surfaceNormal, vec3 lightDir, vec3 fragToCamDir,
                     float shininess) {
    float diffuseT = max(0.0f, dot(surfaceNormal, -light.Dir));

    vec3 specularDirection = reflect(light.Dir, surfaceNormal);
    float specularT = max(0.0f, dot(specularDirection, fragToCamDir));
    specularT = pow(specularT, shininess);

    return light.Data.Color *
           clamp(light.Data.AmbientIntensity +,
                     (diffuseT * light.DiffuseIntensity) +
                     (specularT * light.SpecularIntensity),
                 0.0f, 1.0f);
}
vec3 H_GetLightColor(S_Light_Directional light,
                     vec3 surfacePos, vec3 surfaceNormal, vec3 fragToCamDir) {
    return H_GetLightColor(light.Data, surfaceNormal, light.Dir, fragToCamDir);
}
vec3 H_GetLightColor(S_Light_Point light,
                     vec3 surfacePos, vec3 surfaceNormal, vec3 fragToCamDir) {
    return H_GetLightColor(light.Data, surfaceNormal,
                           normalize(surfacePos - light.Pos),
                           fragToCamDir);
}

//....custom user definitions go here....

void main()
{
    //Module: gl_Position
    gl_Position = vIn_Pos;
    //--------

    //Module: OtherOutputs
    vOut_Color = vIn_Color * u_ColorTint;
    //--------
}


//TODO: Geometry shader?

//=========================================
//           Fragment Shader:
//=========================================

//...same defines as above...

//...user configuration goes here...

in vec3 vOut_Color;

out vec4 fOut;

//...same defines/uniforms as in the vertex shader...
vec2 GetScreenUV() { return GetScreenUV(gl_fragCoord.xy); }


void main()
{
    //Define local versions of the vertex inputs in case they'll be modified.
    vec2 fIn_Color = vOut_Color;

    fOut = vec4(0, 0, 0, 1);
    //Module: Output
    fOut = fIn_Color;
    //--------
}