//This is an AUTO-GENERATED temporary GLSL file for editing
//    as part of the _____ editor.
//DO NOT modify outside of the marked areas far below;
//    it won't accomplish anything.

//=========================================
//           Vertex Shader:
//=========================================
#version 440 core

//Graphics settings configuration:
//General graphics quality level, from 1 to 5:
uniform uint s_Quality;
//Lights:
#define c_MaxDirectionalLights 3
#define c_MaxPointLights 6

//Features toggled by the material/app settings:
uniform bool s_UseNormalMap = true;
uniform bool s_HasScreenGrab = false;

//...user configuration goes here...


//Vertex inputs:
layout (location = 0) in vec3 vIn_Pos;
layout (location = 1) in vec2 vIn_UV;
layout (location = 2) in vec3 vIn_Normal;
layout (location = 3) in vec3 vIn_Tangent;
layout (location = 4) in vec3 vIn_Bitangent;

//Vertex outputs:
out vec2 vOut_UV;
out vec3 vOut_WorldPos;
out vec3 vOut_WorldNormal;
out vec3 vOut_WorldTangent;
out vec3 vOut_WorldBitangent;


//Built-in structures:
struct S_Camera {
    vec3 Pos;
    vec3 Forward, Up, Right;
    bool IsOrthographic;
    float FoV; // For perspective cameras, this is the vertical FoV.
               // For orthographic cameras, this is the vertical view size.
    float Aspect; //Width over height.
    uvec2 ScreenSize; //Size in pixels of the render target/screen.
    float NearClip, FarClip;
};
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
uniform mat4                u_M_WVP,   u_M_VP,
                            u_M_World, u_M_View, u_M_Projection;
uniform S_Camera            u_Camera;
uniform S_Light_Directional u_Lights_Directional[c_MaxDirectionalLights];
uniform S_Light_Point       u_Lights_Point[c_MaxPointLights];
uniform uint                u_Lights_NDirectional, u_Lights_NPoint;
uniform vec3                u_SurfaceTint = 1.0f;

uniform sampler2D u_DiffuseMap, u_NormalMap,
                  u_ScreenColor, u_ScreenDepth;

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
    //Module: Setup
    //--------

    //Module: UV.
    vOut_UV = vIn_UV;
    //--------

    //Module: WorldPos.
    vOut_WorldPos = H_TransformPoint_Fast(u_M_World, vIn_Pos);
    //--------

    //Module: WorldNormal
    vOut_WorldNormal = H_TransformVector_Fast(u_M_World, vIn_Normal);
    //--------

    //Module: WorldTangents
    vOut_WorldTangent = H_TransformVector_Fast((u_M_World, vIn_Tangent));
    vOut_WorldBitangent = H_TransformVector_Fast((u_M_World, vIn_Bitangent));
    //--------

    //Module: GLPos
    gl_Position = H_TransformPoint_Full(u_M_VP, vOut_WorldPos);
    //--------

    //Module: OtherOutputs
    //--------
}


//TODO: Geometry shader?

//=========================================
//           Fragment Shader:
//=========================================

//...same defines as above...

//...user configuration goes here...

in vec2 vOut_UV;
in vec3 vOut_WorldPos;
in vec3 vOut_WorldNormal;
in vec3 vOut_WorldTangent;
in vec3 vOut_WorldBitangent;

out vec4 fOut;

//...same defines/uniforms as in the vertex shader...
vec2 GetScreenUV(vec2 pixelPos) { return pixelPos / vec2(u_Camera.ScreenSize); }
vec2 GetScreenUV() { return GetScreenUV(gl_fragCoord.xy); }


void main()
{
    //Define local versions of the vertex inputs in case they'll be modified.
    vec2 fIn_UV = vOut_UV;
    vec3 fIn_WorldPos = vOut_WorldPos;
    vec3 fIn_WorldNormal = vOut_WorldNormal;
    vec3 fIn_WorldTangent = vOut_WorldTangent;
    vec3 fIn_WorldBitangent = vOut_WorldBitangent;
    vec2 fIn_ScreenUV = GetScreenUV();

    //Module: Setup
    //--------

    vec3 fIn_TrueWorldNormal = fIn_WorldNormal;
    if (s_UseNormalMap)
    {
        //Module: NormalMapping
        fIn_WorldNormal = H_TransformVector(H_GetNormalMapTransform(),
                                            H_UnpackNormalMapN(texture(u_NormalMap, fIn_UV)));
        //--------
    }

    //Module: SetupPostNormalMap
    //--------
    
    //Module: LightGather
    vec3 v_fragToCamDir = normalize(u_Camera.Pos - fIn_WorldPos);
    vec3 v_totalLight = 0.0f;
    //Module: DirectionalLightGather
    for (int i = 0; i < u_Lights_NDirectional; ++i)
        v_totalLight += H_GetLightColor(u_Lights_Directional[i],
                                        fIn_WorldPos, fIn_WorldNormal, v_fragToCamDir);
    //--------
    //Module: PointLightGather
    for (int i = 0; i < u_Lights_NPoint; ++i)
        v_totalLight += H_GetLightColor(u_Lights_Point[i],
                                        fIn_WorldPos, fIn_WorldNormal, v_fragToCamDir);
    //--------
    //--------
    
    vec3 v_diffuseColor = u_SurfaceTint;
    float v_alpha = 1.0f;
    
    //Module: GetDiffuse
    vec4 v_diffuse4 = texture(u_DiffuseMap, fIn_UV);
    v_diffuseColor *= v_diffuse4.rgb;
    v_alpha = v_diffuse4.a;
    //--------

    vec3 v_surfaceColor = vec3(1.0f, 0.0f, 1.0f);
    //Module: ApplyLight
    v_surfaceColor = v_totalLight * v_diffuseColor;
    //--------

    fOut = vec4(v_surfaceColor, v_alpha);
    //Module: Finalize
    //--------
}