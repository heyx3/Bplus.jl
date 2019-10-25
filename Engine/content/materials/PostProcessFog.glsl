//This is an AUTO-GENERATED temporary GLSL file for editing
//    as part of the _____ editor.
//DO NOT modify outside of the specially-marked areas;
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
uniform bool s_HasColorGrab = true,
             s_HasDepthGrab = true;

//Vertex inputs:
layout (location = 0) in vec2 vIn_UV;

//...user configuration goes here...

//Vertex outputs:
out vec2 vOut_UV;


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
uniform mat4                u_M_View, u_M_Projection, u_M_VP,
                            u_M_iView, u_M_iProjection, u_M_iVP;
uniform S_Camera            u_Camera;
uniform S_Light_Directional u_Lights_Directional[c_MaxDirectionalLights];
uniform S_Light_Point       u_Lights_Point[c_MaxPointLights];
uniform uint                u_Lights_NDirectional, u_Lights_NPoint;

uniform sampler2D u_ScreenColor, u_ScreenDepth;

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

    //Module: GLPos
    gl_Position = vec4(-1.0f + (2.0f * vIn_UV.xy),
                       0.0f, 1.0f);
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

out vec4 fOut;

//...same defines/uniforms as in the vertex shader...

vec2 GetScreenUV() { return GetScreenUV(gl_fragCoord.xy); }

uniform vec3 u_FogColor;


void main()
{
    //Define local versions of the vertex inputs in case they'll be modified.
    vec2 fIn_UV = vOut_UV;
    vec2 fIn_ScreenPos = -1.0f + (2.0f * fIn_UV);

    //Module: Setup
    //--------

    vec3 v_color = vec3(1.0f, 0.0f, 1.0f);
    float v_depth = 0.0f;
    //Module: Sample Screen
    //Module: SampleColor
    if (s_HasColorGrab)
        v_color = texture(u_ScreenColor, fIn_UV).rgb;
    //--------
    //Module: SampleDepth
    if (s_HasDepthGrab)
        v_depth = texture(u_ScreenDepth, fIn_UV).r;
    //--------
    //--------

    //Module: DepthConversions
    float v_viewDepth; //The view-space depth (not including the space behind the near plane)
    {
        //Based on: https://stackoverflow.com/questions/1153114/converting-a-depth-texture-sample-to-a-distance
        vec4 screenPos4 = vec4(fIn_ScreenPos, (-1.0f + (2.0f * v_depth)), 1.0f);
        vec4 mRow3 = vec4(u_M_iProjection[0][3], u_M_iProjection[1][3], u_M_iProjection[2][3], u_M_iProjection[3][3]),
             mRow4 = vec4(u_M_iProjection[0][4], u_M_iProjection[1][4], u_M_iProjection[2][4], u_M_iProjection[3][4]);
        vec2 viewPositionZW = vec2(dot(mRow3, screenPos4),
                                   dot(mRow4, screenPos4));
        v_viewDepth = -(viewPositionZW.x / viewPositionZW.y);
    }
    //--------

    fOut.rgb = v_color;
    fOut.a = 1.0f;
    //Module: SetOutputs
    fOut.rgb = lerp(v_color, u_FogColor, v_viewDepth);
    //--------
}