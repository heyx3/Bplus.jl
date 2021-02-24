#pragma once

#include <any>

#define TEST_NO_MAIN
#include <acutest.h>

#include <glm/gtx/string_cast.hpp>

#include <B+/RenderLibs.h>
#include <B+/GL/Textures/Target.h>
#include <B+/GL/Materials/CompiledShader.h>
#include <B+/GL/Buffers/MeshData.h>
#include <B+/Helpers/EditorCamControls.h>

#include "../SimpleApp.hpp"


void SimpleApp()
{
    glm::u8vec4 backCol1{ 45, 80, 206, 255 },
                backCol2{ 254, 2, 145, 150 };
    float colorT = 0.0f;

    Simple::Run(
        //Init:
        [&]() { },

        //Update:
        [&](float deltaT)
        {
            colorT += deltaT * 0.75f;
            colorT -= (int)colorT;

            ImGui::Text("I'm label 3.");

            int nKeys;
            const auto* keyPresses = SDL_GetKeyboardState(&nKeys);

            //Pressing Space constitutes a test failure.
            //Pressing Enter constitutes a test success.
            bool testPassed = TEST_CHECK_(!keyPresses[SDL_SCANCODE_SPACE],
                                          "The user pressed Space, \
indicating that not all ImGUI labels were visible.");
            if (!testPassed ||
                keyPresses[SDL_SCANCODE_KP_ENTER] ||
                keyPresses[SDL_SCANCODE_RETURN] ||
                keyPresses[SDL_SCANCODE_RETURN2])
            {
                Simple::App->Quit(true);
            }
        },

        //Render:
        [&](float deltaT)
        {
            ImGui::Text("I'm label 1.");

            auto backCol = glm::mix((glm::fvec4)backCol1,
                                    (glm::fvec4)backCol2,
                                    colorT);
            backCol /= 255.0f;

            auto context = Bplus::GL::Context::GetCurrentContext();
            context->ClearScreen(backCol);

            ImGui::Text("I'm label 2.");

            ImGui::Text("If you see all four labels (including this one),\nPress Enter. \
Else, press Space.");
        },

        //Quit:
        [&]()
        {

        });
}

void BasicRenderApp()
{
    using namespace Bplus::GL;
    using namespace Bplus::GL::Buffers;
    using namespace Bplus::GL::Textures;
    namespace MeshVertices = Bplus::GL::Buffers::VertexData;

    Buffer *trisCoordinates = nullptr,
           *trisIndices = nullptr;
    MeshData* tris = nullptr;
    CompiledShader* shader = nullptr;
    Texture2D* tex = nullptr;

    Simple::Run(
        //Init:
        [&]() {
            TEST_CASE("Creating a Buffer for two triangles");
            std::array<glm::fvec2, 6> trisCoordinatesData = {
                glm::fvec2(-0.75f, 0.75f),
                glm::fvec2(0, 0.75f),
                glm::fvec2(-0.75f, 0.5f),

                glm::fvec2(0.25f, -0.25f),
                glm::fvec2(0.5f, 0.25f),
                glm::fvec2(0.75f, -0.25f)
            };
            std::array<glm::u16, 6> trisIndexData = { 0, 1, 2,    3, 4, 5 };

            trisCoordinates = new Buffer(6, false, trisCoordinatesData.data());
            trisIndices = new Buffer(6, false, trisIndexData.data());

            TEST_CASE("Creating a MeshData for two triangles");
            tris = new MeshData(PrimitiveTypes::Triangle,
                                MeshDataSource(trisIndices, sizeof(glm::u16)),
                                IndexDataTypes::UInt16,
                                { MeshDataSource(trisCoordinates, sizeof(glm::fvec2)) },
                                { VertexDataField(0, 0, MeshVertices::Type::FVector<2>()) });

            TEST_CASE("Compiling the shader");
            OglPtr::ShaderProgram shaderPtr;
            #pragma region Shader compilation

            ShaderCompileJob compiler;

            compiler.VertexSrc =
R"(layout (location = 0) in vec2 vIn_Pos;
layout (location = 0) out vec2 vOut_Pos;
void main()
{
    gl_Position = vec4(vIn_Pos, 0, 1);
    vOut_Pos = vIn_Pos;
})";
            compiler.FragmentSrc =
R"(layout (location = 0) in vec2 fIn_Pos;
layout (location = 0) out vec4 fOut_Color;
layout (bindless_sampler) uniform sampler2D MyTexture;

void main()
{
    vec4 texCol = texture(MyTexture, fIn_Pos * 3.5);
    vec3 color = vec3(fract(fIn_Pos * 10),
                      abs(sin(gl_FragCoord.y / 15.0)));
    fOut_Color = vec4(mix(texCol.rrr, color, 0.5), 1);
})";

            compiler.PreProcessIncludes();

            std::string compileError;
            bool dummyBool;
            std::tie(compileError, dummyBool) = compiler.Compile(shaderPtr);

            TEST_CHECK_(!shaderPtr.IsNull(), "Shader failed to compile:\n\t%s", compileError.c_str());
            if (shaderPtr.IsNull())
            {
                Simple::App->Quit(true);
                return;
            }

            #pragma endregion
            RenderState shaderRenderState;
            shaderRenderState.CullMode = FaceCullModes::Off;
            shaderRenderState.DepthTest = ValueTests::Off;
            shader = new CompiledShader(shaderRenderState, shaderPtr, { "MyTexture" });

            tex = new Texture2D(glm::uvec2{ 100, 100 },
                                SimpleFormat(FormatTypes::Float, SimpleFormatComponents::R, SimpleFormatBitDepths::B32),
                                0,
                                Sampler<2>(WrapModes::Repeat, PixelFilters::Rough));
            std::array<float, 100 * 100> pixels;
            for (uint32_t y = 0; y < tex->GetSize().y; ++y)
                for (uint32_t x = 0; x < tex->GetSize().x; ++x)
                {
                    pixels[x + (y * tex->GetSize().x)] = (rand() % RAND_MAX) / (float)RAND_MAX;
                }
            tex->Set_Color(pixels.data(), PixelIOChannels::Red);
            shader->SetUniform("MyTexture", tex->GetView());
        },

        //Update:
        [&](float deltaT) {
            ImGui::Text("Press 'escape' to quit.");

            auto keyStates = SDL_GetKeyboardState(nullptr);
            if (keyStates[SDL_SCANCODE_ESCAPE])
                Simple::App->Quit(true);
        },

        //Render:
        [&](float deltaT) {
            auto& context = *Context::GetCurrentContext();
            
            context.ClearScreen(glm::fvec4(0.25f, 0.25f, 0.1f, 0.0f));
            context.Draw(DrawMeshMode_Basic(*tris, 6), *shader,
                         DrawMeshMode_Indexed());
        },

        //Quit:
        [&]() {
            #define TRY_DELETE(x) \
                if (x != nullptr) { \
                    delete x; \
                    x = nullptr; \
                }

            TRY_DELETE(shader);
            TRY_DELETE(tex);
            TRY_DELETE(trisCoordinates);
            TRY_DELETE(trisIndices);
            TRY_DELETE(tris);
            #undef TRY_DELETE
        });
}

void AdvancedTexturesApp()
{
    using namespace Bplus::Helpers;
    using namespace Bplus::GL;
    using namespace Bplus::GL::Buffers;
    using namespace Bplus::GL::Textures;
    namespace MeshVertices = Bplus::GL::Buffers::VertexData;
    
    Buffer *terrainUVs = nullptr,
           *terrainIndices = nullptr,
           *fullScreenTri = nullptr,
           *skyCubePoses = nullptr;
    MeshData *terrainMesh = nullptr,
             *fullScreenMesh = nullptr,
             *skyCubeMesh = nullptr;
    CompiledShader *noiseShader = nullptr,
                   *terrainShader = nullptr,
                   *skyNoiseShader = nullptr,
                   *skyShader = nullptr;
    Target *heightmapTarget = nullptr,
            *skyNoiseTarget = nullptr;
    TextureCube* skyTex = nullptr;

    float elapsedTime = 0;

    #pragma region Lighting

    float sunYawDegrees = 0,
          sunPitchDegrees = 45;
    glm::vec3 sunColor = { 1.0, 1.0, 0.35f };

    auto getSunDir = [&sunYawDegrees, &sunPitchDegrees]() {
        auto yawRot = glm::angleAxis(glm::radians(sunYawDegrees), glm::fvec3{ 0, 0, 1 }),
             pitchRot = glm::angleAxis(glm::radians(sunPitchDegrees), glm::fvec3{ 0, 1, 0 });
        return yawRot  * pitchRot * glm::fvec3{ 1, 0, 0 };
    };

    auto doGuiSun = [&sunYawDegrees, &sunPitchDegrees, &sunColor]() {
        ImGui::SliderFloat("Yaw", &sunYawDegrees, -360, 360);
        ImGui::SliderFloat("Pitch", &sunPitchDegrees, 0, 90);
        ImGui::ColorEdit3("Color", glm::value_ptr(sunColor), ImGuiColorEditFlags_NoLabel);
    };

    auto updateShaderSun = [&getSunDir, &sunColor](CompiledShader& shader) {
        shader.SetUniform("u_SunDir", getSunDir());
        shader.SetUniform("u_SunColor", sunColor);
    };

    auto sunShaderParams = std::vector<std::string>{
        "u_SunDir", "u_SunColor"
    };

    const char* const sunFunction = R"(

uniform vec3 u_SunDir = vec3(0.707106781, 0, -0.707106781);
uniform vec3 u_SunColor = vec3(1, 1, 0.35);

vec3 calcLighting(vec3 surfaceNormal) {
    return u_SunColor * dot(surfaceNormal, -u_SunDir);
}

)";

    #pragma endregion
    
    #pragma region Terrain noise

    int tNoiseOctaveCount = 7;
    float tNoiseScale = 5.875f,
          tNoisePersistence = 2.48120f;
    bool tNoiseRidged = false;


    auto doGuiTNoise = [&tNoiseOctaveCount, &tNoisePersistence, &tNoiseScale, &tNoiseRidged]() {
        ImGui::SliderInt("# Octaves", &tNoiseOctaveCount, 1, 10);
        ImGui::SliderFloat("Scale", &tNoiseScale, 1.0f, 100);
        ImGui::SliderFloat("Persistence", &tNoisePersistence, 0.00001f, 100, "%.5f", 3);
        ImGui::Checkbox("Ridged", &tNoiseRidged);
    };

    auto updateShaderTNoise = [&tNoiseOctaveCount, &tNoisePersistence, &tNoiseScale, &tNoiseRidged]
        (CompiledShader& shader) {
            shader.SetUniform("u_NoiseOctaves", tNoiseOctaveCount);
            shader.SetUniform("u_NoiseScale", tNoiseScale);
            shader.SetUniform("u_NoisePersistence", tNoisePersistence);
            shader.SetUniform("u_NoiseRidged", tNoiseRidged);
        };

    auto tNoiseShaderParams = std::vector<std::string>{
        "u_NoiseOctaves", "u_NoiseScale",
        "u_NoisePersistence", "u_NoiseRidged"
    };
    const char* const tNoiseShaderFunction = R"(

uniform int u_NoiseOctaves = 3;
uniform float u_NoiseScale = 2.0,
              u_NoisePersistence = 2.0;
uniform bool u_NoiseRidged = false;

vec2 hash( uvec2 x )
{
    //Source: https://stackoverflow.com/a/52207531

    const uint K = 1103515245U;

    x = ((x>>8U) ^ x.yx)* K;
    x = ((x>>8U) ^ x.yx)* K;
    x = ((x>>8U) ^ x.yx)* K;

    return x * (1.0 / float(0xffffffffU));
}
vec2 hash(vec2 x) { return hash(floatBitsToUint(x)); }

vec2 smoothNoise(vec2 p)
{
    vec2 minP = floor(p),
         maxP = minP + 1;
    vec2 t = p - minP;

    return mix(mix(hash(minP),                   hash(vec2(maxP.x, minP.y)), t.x),
               mix(hash(vec2(minP.x, maxP.y)),   hash(maxP),                 t.x),
               t.y);
}

float terrainNoise(vec2 uv)
{
    uv *= u_NoiseScale;

    float noiseSum = 0,
          noiseMax = 0.000000001,
          noiseWeight = 1.0;
    for (int i = 0; i < u_NoiseOctaves; ++i)
    {
        float octaveVal = smoothNoise(uv).r;
        if (u_NoiseRidged)
            octaveVal = abs(octaveVal - 0.5) * 2;

        noiseSum += noiseWeight * octaveVal;
        noiseMax += noiseWeight;
        
        noiseWeight /= u_NoisePersistence;
        uv = (uv + (2.7412 * mix(vec2(-1.0), vec2(1.0), hash(uvec2(i, i * 47))))) * u_NoisePersistence;
    }

    return noiseSum / noiseMax;
}

)";

    #pragma endregion
    
    #pragma region Sky noise

    int sSkyNoiseOctaveCount = 10;
    float sSkyNoiseScale = 5.325f,
          sSkyNoisePersistence = 3.441f;
    int sCloudNoiseOctaveCount = 7;
    float sCloudNoiseScale = 5.875f,
          sCloudNoisePersistence = 2.48120f;
    float sCloudSharpness = 1.0f;
    glm::fvec3 skyColor1 = { 0.152f, 0.152f, 1.0f },
               skyColor2 = { 0.27f, 0.548f, 0.966f },
               cloudColor = { 1, 1, 1 };

    auto doGuiSNoise = [&]() {
        ImGui::SliderInt("# Octaves", &sSkyNoiseOctaveCount, 1, 10);
        ImGui::SliderFloat("Scale", &sSkyNoiseScale, 1.0f, 100);
        ImGui::SliderFloat("Persistence", &sSkyNoisePersistence, 0.00001f, 100, "%.5f", 3);
        ImGui::ColorEdit3("Color 1", glm::value_ptr(skyColor1));
        ImGui::ColorEdit3("Color 2", glm::value_ptr(skyColor2));
        ImGui::Dummy({ 1, 5 });

        ImGui::Text("CLOUDS");
        ImGui::PushID("CLOUDS");
        ImGui::Indent();
        ImGui::SliderInt("# Octaves", &sCloudNoiseOctaveCount, 1, 10);
        ImGui::SliderFloat("Scale", &sCloudNoiseScale, 1.0f, 100);
        ImGui::SliderFloat("Persistence", &sCloudNoisePersistence, 0.00001f, 100, "%.5f", 3);
        ImGui::SliderFloat("Sharpness", &sCloudSharpness, 0.0001f, 10, "%.5f", 2);
        ImGui::ColorEdit3("##Color", glm::value_ptr(cloudColor));
        ImGui::Unindent();
        ImGui::PopID();
    };

    auto updateShaderSNoise = [&](CompiledShader& shader) {
            shader.SetUniform("u_SkyNoise.NOctaves", sSkyNoiseOctaveCount);
            shader.SetUniform("u_SkyNoise.Scale", sSkyNoiseScale);
            shader.SetUniform("u_SkyNoise.Persistence", sSkyNoisePersistence);
            shader.SetUniform("u_CloudNoise.NOctaves", sCloudNoiseOctaveCount);
            shader.SetUniform("u_CloudNoise.Scale", sCloudNoiseScale);
            shader.SetUniform("u_CloudNoise.Persistence", sCloudNoisePersistence);
            shader.SetUniform("u_CloudSharpness", sCloudSharpness);
            shader.SetUniform("u_SkyColor1", skyColor1);
            shader.SetUniform("u_SkyColor2", skyColor2);
            shader.SetUniform("u_CloudColor", cloudColor);
        };

    std::vector<std::string> sNoiseShaderParams = {
        "u_SkyNoise.NOctaves", "u_SkyNoise.Scale", "u_SkyNoise.Persistence",
        "u_CloudNoise.NOctaves", "u_CloudNoise.Scale", "u_CloudNoise.Persistence",
        "u_CloudSharpness",
        "u_SkyColor1", "u_SkyColor2", "u_CloudColor"
    };
    const char* const sNoiseShaderFunction = R"(#line 1 1
struct NoiseSettings {
    int NOctaves;
    float Scale,
          Persistence;
};
uniform NoiseSettings u_SkyNoise;
uniform NoiseSettings u_CloudNoise;
uniform float u_CloudSharpness;

uniform vec3 u_SkyColor1, u_SkyColor2, u_CloudColor;


vec3 hash( uvec3 x )
{
    //Source: https://stackoverflow.com/a/52207531

    const uint K = 1103515245U;

    x = ((x>>8U) ^ x.yzx)* K;
    x = ((x>>8U) ^ x.yzx)* K;
    x = ((x>>8U) ^ x.yzx)* K;

    return x * (1.0 / float(0xffffffffU));
}
vec3 hash(vec3 x) { return hash(floatBitsToUint(x)); }

vec3 smoothNoise(vec3 p)
{
    vec3 minP = floor(p),
         maxP = minP + 1;
    vec3 t = p - minP;

    return mix(mix(mix(hash(minP),                           hash(vec3(maxP.x, minP.yz)),         t.x),
                   mix(hash(vec3(minP.x, maxP.y, minP.z)),   hash(vec3(maxP.xy, minP.z)),         t.x),
                   t.y),
               mix(mix(hash(vec3(minP.xy, maxP.z)),          hash(vec3(maxP.x, minP.y, maxP.z)),  t.x),
                   mix(hash(vec3(minP.x, maxP.yz)),          hash(maxP),                          t.x),
                   t.y),
               t.z);
}

float octaveNoise(vec3 p, NoiseSettings settings)
{
    p *= settings.Scale;

    float noiseSum = 0,
          noiseMax = 0.000000001,
          noiseWeight = 1.0;
    for (int i = 0; i < settings.NOctaves; ++i)
    {
        noiseSum += noiseWeight * smoothNoise(p).r;
        noiseMax += noiseWeight;
        
        noiseWeight /= settings.Persistence;
        p += 2.7412 * mix(vec3(-1.0), vec3(1.0), hash(uvec3(i, i * 47, i * 53)));
        p *= settings.Persistence;
    }

    return noiseSum / noiseMax;
}

vec3 getSkyColor(vec3 viewDir)
{
    viewDir = normalize(viewDir);
    float skyNoise = octaveNoise(viewDir, u_SkyNoise),
          cloudNoise = octaveNoise(viewDir, u_CloudNoise);

    return mix(mix(u_SkyColor1, u_SkyColor2, skyNoise),
               u_CloudColor,
               pow(cloudNoise, u_CloudSharpness));
}

)";

    #pragma endregion

    #pragma region Terrain positioning

    float terrainHorzSize = 2048,
          terrainVertSize = 500;

    auto doGuiTerrainTransform = [&terrainHorzSize, &terrainVertSize]() {
        ImGui::DragFloat("Length", &terrainHorzSize);
        ImGui::DragFloat("Height", &terrainVertSize);
    };

    auto terrainTransformParams = std::vector<std::string>{
        "u_TerrainLength", "u_TerrainHeight"
    };

    auto updateShaderTerrainTransform = [&terrainHorzSize, &terrainVertSize](CompiledShader& shader) {
        shader.SetUniform("u_TerrainLength", terrainHorzSize);
        shader.SetUniform("u_TerrainHeight", terrainVertSize);
    };

    const char* const terrainTransformFunction = R"(

uniform float u_TerrainLength, u_TerrainHeight;

vec3 getTerrainPos(vec2 uv, float heightmap) {
    float halfLength = u_TerrainLength / 2;
    return mix(vec2(-halfLength, 0).xxy,
               vec2(halfLength, u_TerrainHeight).xxy,
               vec3(uv, heightmap));
}

)";

    #pragma endregion
    
    #pragma region Terrain surface color

    glm::vec3 terrainColor = { 0.2f, 0.8f, 0.4f };

    auto doGuiTerrainColor = [&terrainColor]() {
        ImGui::ColorEdit3("##Color", glm::value_ptr(terrainColor));
    };

    auto terrainColorParams = std::vector<std::string>{
        "u_TerrainColor"
    };

    auto updateShaderTerrainColor = [&terrainColor](CompiledShader& shader) {
        shader.SetUniform("u_TerrainColor", terrainColor);
    };

    const char* const terrainColorFunction = R"(

uniform vec3 u_TerrainColor;

vec3 getTerrainColor(vec2 uv, vec3 worldNormal, float height) {
    return u_TerrainColor;
}

)";

    #pragma endregion

    #pragma region Camera Settings

    float camVerticalFOV = 90;

    EditorCamControls camera(glm::fvec3{ 0, 0, terrainVertSize + 10 },
                             CameraUpModes::KeepUpright,
                             glm::normalize(glm::fvec3{ 1, 1, -1 }));

    auto doGuiCamera = [&camVerticalFOV]() {
        ImGui::SliderFloat("Field of View", &camVerticalFOV, 0.00001f, 179.99f);
    };

    auto getFarClipPlane = [&terrainHorzSize]() { return terrainHorzSize * 2; };
    auto getProjectionMatrix = [&camVerticalFOV, &terrainHorzSize, &getFarClipPlane]() {
        glm::ivec2 windowSize;
        SDL_GetWindowSize(Simple::App->MainWindow, &windowSize.x, &windowSize.y);
        return glm::perspective(glm::radians(camVerticalFOV),
                                (float)windowSize.x / windowSize.y,
                                0.1f, getFarClipPlane());
    };

    #pragma endregion

    Simple::Run(
        //Init:
        [&]() {

            TEST_CASE("Creating the terrain data");
            #pragma region Create terrain data

            const uint_fast32_t terrainResolution = 512;
            using TerrainIdx = glm::u32;

            //Vertices:
            {
                std::vector<glm::vec2> terrainUVData;
                terrainUVData.reserve(terrainResolution * terrainResolution);
                const float texelSize = 1 / (float)(terrainResolution - 1);
                for (uint_fast32_t y = 0; y < terrainResolution; ++y)
                    for (uint_fast32_t x = 0; x < terrainResolution; ++x)
                        terrainUVData.push_back(glm::fvec2((float)x, (float)y) * texelSize);

                terrainUVs = new Buffer(terrainResolution * terrainResolution, false, terrainUVData.data());
            }

            //Indices:
            {
                std::vector<TerrainIdx> terrainIndexData;
                size_t nIndices = (terrainResolution - 1) * (terrainResolution - 1) * 6;
                terrainIndexData.reserve(nIndices);
                for (TerrainIdx y = 1; y < terrainResolution; ++y)
                    for (TerrainIdx x = 1; x < terrainResolution; ++x)
                    {
                        TerrainIdx baseI = (x - 1) + ((y - 1) * terrainResolution);
                        
                        terrainIndexData.push_back(baseI);
                        terrainIndexData.push_back(baseI + terrainResolution + 1);
                        terrainIndexData.push_back(baseI + terrainResolution);

                        terrainIndexData.push_back(baseI + terrainResolution + 1);
                        terrainIndexData.push_back(baseI);
                        terrainIndexData.push_back(baseI + 1);
                    }

                terrainIndices = new Buffer(nIndices, false, terrainIndexData.data());
            }

            TEST_CASE("Creating a MeshData for the terrain");
            terrainMesh = new MeshData(PrimitiveTypes::Triangle,
                                       MeshDataSource(terrainIndices, sizeof(TerrainIdx)),
                                       GetIndexType<TerrainIdx>(),
                                       { MeshDataSource(terrainUVs, sizeof(glm::fvec2)) },
                                       { VertexDataField(0, 0, MeshVertices::Type::FVector<2>()) });

            #pragma endregion

            TEST_CASE("Creating the full-screen triangle mesh");
            #pragma region Create full screen triangle mesh

            auto fullScreenTriData = std::array{ glm::fvec2{ -1, -1 },
                                                 glm::fvec2{ 3, -1 },
                                                 glm::fvec2{ -1, 3 } };
            fullScreenTri = new Buffer(3, false, fullScreenTriData.data());

            fullScreenMesh = new MeshData(PrimitiveTypes::Triangle,
                                          std::vector{ MeshDataSource(fullScreenTri, sizeof(glm::fvec2)) },
                                          std::vector{ VertexDataField(0, 0, MeshVertices::Type::FVector<2>()) });

            #pragma endregion

            TEST_CASE("Creating the sky-box mesh");
            #pragma region Create sky-box mesh

            {
                std::array skyCubeVertices = {
                    //+X
                    glm::fvec3(1, -1, -1),
                    glm::fvec3(1, 1, -1),
                    glm::fvec3(1, -1, 1),
                    //
                    glm::fvec3(1, 1, -1),
                    glm::fvec3(1, -1, 1),
                    glm::fvec3(1, 1, 1),

                    //-X
                    glm::fvec3(-1, -1, -1),
                    glm::fvec3(-1, 1, -1),
                    glm::fvec3(-1, -1, 1),
                    //
                    glm::fvec3(-1, 1, -1),
                    glm::fvec3(-1, -1, 1),
                    glm::fvec3(-1, 1, 1),

                    //+Y
                    glm::fvec3(-1, 1, -1),
                    glm::fvec3(1, 1, -1),
                    glm::fvec3(-1, 1, 1),
                    //
                    glm::fvec3(1, 1, -1),
                    glm::fvec3(-1, 1, 1),
                    glm::fvec3(1, 1, 1),

                    //-Y
                    glm::fvec3(-1, -1, -1),
                    glm::fvec3(1, -1, -1),
                    glm::fvec3(-1, -1, 1),
                    //
                    glm::fvec3(1, -1, -1),
                    glm::fvec3(-1, -1, 1),
                    glm::fvec3(1, -1, 1),

                    //+Z
                    glm::fvec3(-1, -1, 1),
                    glm::fvec3(1, -1, 1),
                    glm::fvec3(-1, 1, 1),
                    //
                    glm::fvec3(1, -1, 1),
                    glm::fvec3(-1, 1, 1),
                    glm::fvec3(1, 1, 1),

                    //-Z
                    glm::fvec3(-1, -1, -1),
                    glm::fvec3(1, -1, -1),
                    glm::fvec3(-1, 1, -1),
                    //
                    glm::fvec3(1, -1, -1),
                    glm::fvec3(-1, 1, -1),
                    glm::fvec3(1, 1, -1)
                };
                skyCubePoses = new Buffer(skyCubeVertices.size(), false, skyCubeVertices.data());
            }
            skyCubeMesh = new MeshData(PrimitiveTypes::Triangle,
                                       { MeshDataSource(skyCubePoses, sizeof(glm::fvec3)) },
                                       { VertexDataField(0, 0, MeshVertices::Type::FVector<3>()) });

            #pragma endregion


            OglPtr::ShaderProgram shaderPtr;
            ShaderCompileJob compiler;

            TEST_CASE("Compiling the noise shader");
            #pragma region Noise shader

            compiler.GeometrySrc = "";

            compiler.VertexSrc = std::string(R"(#line 1 0
layout (location = 0) in vec2 vIn_Pos;
layout (location = 0) out vec2 vOut_Pos;
void main()
{
    gl_Position = vec4(vIn_Pos, 0, 1);
    vOut_Pos = vIn_Pos;
})");

            compiler.FragmentSrc = std::string(R"(#line 1 0
layout (location = 0) in vec2 fIn_Pos;
layout (location = 0) out vec4 fOut_Color;

)") + tNoiseShaderFunction + R"(

void main()
{
    float val = terrainNoise(fIn_Pos);
    fOut_Color = vec4(val.xxx, 1);
})";

            compiler.PreProcessIncludes();

            std::string compileError;
            bool dummyBool;
            std::tie(compileError, dummyBool) = compiler.Compile(shaderPtr);

            TEST_CHECK_(!shaderPtr.IsNull(), "Noise shader failed to compile:\n\t%s", compileError.c_str());
            if (shaderPtr.IsNull())
            {
                Simple::App->Quit(true);
                return;
            }

            RenderState noiseRenderState;
            noiseRenderState.CullMode = FaceCullModes::Off;
            noiseRenderState.DepthTest = ValueTests::Off;
            noiseRenderState.EnableDepthWrite = false;
            noiseShader = new CompiledShader(noiseRenderState, shaderPtr, tNoiseShaderParams);

            #pragma endregion

            TEST_CASE("Compiling the terrain shader");
            #pragma region Terrain shader

            compiler.GeometrySrc = "";

            compiler.VertexSrc = std::string(R"(#line 1 0
layout (location = 0) in vec2 vIn_UV;
layout (location = 0) out vec2 vOut_UV;

)") + terrainTransformFunction + R"(

layout(bindless_sampler) uniform sampler2D u_Heightmap;
uniform mat4 u_ViewProjMatrix;

void main()
{
    float heightmap = textureLod(u_Heightmap, vIn_UV, 0).r;
    vec3 worldPos = getTerrainPos(vIn_UV, heightmap);
    
    gl_Position = u_ViewProjMatrix * vec4(worldPos, 1);
    vOut_UV = vIn_UV;
})";

            compiler.FragmentSrc = std::string(R"(#line 1 0
layout (location = 0) in vec2 fIn_UV;
layout (location = 0) out vec4 fOut_Color;

)") + terrainTransformFunction + sunFunction + terrainColorFunction + R"(

layout(bindless_sampler) uniform sampler2D u_Heightmap;

void main()
{
    //Calculate the normal using finite differences.
    vec3 texel = vec3(1.0 / vec2(textureSize(u_Heightmap, 0)),
                      0.0);
    float heightMinX = textureLod(u_Heightmap, fIn_UV - texel.xz, 0).r,
          heightMaxX = textureLod(u_Heightmap, fIn_UV + texel.xz, 0).r,
          heightMinY = textureLod(u_Heightmap, fIn_UV - texel.zy, 0).r,
          heightMaxY = textureLod(u_Heightmap, fIn_UV + texel.zy, 0).r;
    vec3 vNormal = vec3((heightMaxX - heightMinX),
                        (heightMaxY - heightMinY),
                        4.0);
    vNormal.xy /= u_TerrainLength * texel.xy;
    vNormal.z /= u_TerrainHeight;
    vNormal = normalize(vNormal);

    //Calculate the surface color.
    fOut_Color.rgb = getTerrainColor(fIn_UV, vNormal, textureLod(u_Heightmap, fIn_UV, 0).r)
                      * calcLighting(vNormal);
    fOut_Color.a = 1;
})";

            compiler.PreProcessIncludes();
            std::tie(compileError, dummyBool) = compiler.Compile(shaderPtr);

            TEST_CHECK_(!shaderPtr.IsNull(), "Terrain shader failed to compile:\n\t%s", compileError.c_str());
            if (shaderPtr.IsNull())
            {
                Simple::App->Quit(true);
                return;
            }

            RenderState terrainRenderState;
            auto terrainShaderParams = Bplus::Concatenate<std::string>(sunShaderParams,
                                                                       terrainColorParams,
                                                                       terrainTransformParams,
                                                                       std::vector{ "u_Heightmap", "u_ViewProjMatrix" });
            terrainShader = new CompiledShader(terrainRenderState, shaderPtr, terrainShaderParams);

            #pragma endregion

            TEST_CASE("Compiling the sky shader");
            #pragma region Sky shader

            compiler.GeometrySrc = "";

            compiler.VertexSrc = R"(#line 1 0
layout (location = 0) in vec3 vIn_Pos;
layout (location = 0) out vec3 vOut_CubeUV;

uniform mat4 u_ViewProjMatrix;
uniform vec3 u_CamPos;
uniform float u_Length;

void main()
{
    vec3 worldPos = u_CamPos + (vIn_Pos * u_Length);
    
    vOut_CubeUV = vIn_Pos;
    gl_Position = u_ViewProjMatrix * vec4(worldPos, 1);

    //Don't allow the cube to escape the camera's far plane
    //    by capping its depth at 1.
    gl_Position.z = min(gl_Position.z, gl_Position.w);
})";

            compiler.FragmentSrc = R"(#line 1 0
layout (location = 0) in vec3 fIn_CubeUV;
layout (location = 0) out vec4 fOut_Color;

layout(bindless_sampler) uniform samplerCube u_Skybox;

void main()
{
    fOut_Color.rgb = texture(u_Skybox, fIn_CubeUV).rgb;
    fOut_Color.a = 1;
})";

            compiler.PreProcessIncludes();
            std::tie(compileError, dummyBool) = compiler.Compile(shaderPtr);

            TEST_CHECK_(!shaderPtr.IsNull(), "Skybox shader failed to compile:\n\t%s", compileError.c_str());
            if (shaderPtr.IsNull())
            {
                Simple::App->Quit(true);
                return;
            }

            RenderState skyboxRenderState;
            skyboxRenderState.EnableDepthWrite = false;
            skyboxRenderState.CullMode = FaceCullModes::Off;
            std::vector<std::string> skyboxShaderParams = { "u_ViewProjMatrix", "u_CamPos",
                                                            "u_Length", "u_Skybox" };
            skyShader = new CompiledShader(skyboxRenderState, shaderPtr, skyboxShaderParams);

            #pragma endregion

            TEST_CASE("Compiling the sky noise shader");
            #pragma region Sky noise shader

            compiler.GeometrySrc = "";

            compiler.VertexSrc = std::string(R"(#line 1 0
layout (location = 0) in vec2 vIn_Pos;
layout (location = 0) out vec2 vOut_Pos;
void main()
{
    gl_Position = vec4(vIn_Pos, 0, 1);
    vOut_Pos = 0.5 + (0.5 * vIn_Pos);
})");

            compiler.FragmentSrc = std::string(R"(#line 1 0
layout (location = 0) in vec2 fIn_Pos;

layout (location = 0) out vec4 fOut_PosX;
layout (location = 1) out vec4 fOut_NegX;
layout (location = 2) out vec4 fOut_PosY;
layout (location = 3) out vec4 fOut_NegY;
layout (location = 4) out vec4 fOut_PosZ;
layout (location = 5) out vec4 fOut_NegZ;

)") + sNoiseShaderFunction + R"(#line 11 0

void main()
{
    vec3 p = vec3(0, 0, 0);

#define COLOR_FACE(face,   mainAxis, mainPos,  horzAxis, horzMin, horzMax,    vertAxis, vertMin, vertMax) \
    p.mainAxis = mainPos; \
    p.horzAxis = mix(horzMin, horzMax, fIn_Pos.x); \
    p.vertAxis = mix(vertMin, vertMax, fIn_Pos.y); \
    face = vec4(getSkyColor(p), 1)

    COLOR_FACE(fOut_PosX,   x, 1,    z, 1, -1,   y, 1, -1);
    COLOR_FACE(fOut_NegX,   x, -1,   z, -1, 1,   y, 1, -1);
    COLOR_FACE(fOut_PosY,   y, 1,    x, -1, 1,   z, -1, 1);
    COLOR_FACE(fOut_NegY,   y, -1,   x, -1, 1,   z, 1, -1);
    COLOR_FACE(fOut_PosZ,   z, 1,    x, -1, 1,   y, 1, -1);
    COLOR_FACE(fOut_NegZ,   z, -1,   x, 1, -1,   y, 1, -1);
})";

            compiler.PreProcessIncludes();
            std::tie(compileError, dummyBool) = compiler.Compile(shaderPtr);

            TEST_CHECK_(!shaderPtr.IsNull(), "Skybox noise shader failed to compile:\n\t%s", compileError.c_str());
            if (shaderPtr.IsNull())
            {
                Simple::App->Quit(true);
                return;
            }

            RenderState sNoiseRenderState;
            sNoiseRenderState.CullMode = FaceCullModes::Off;
            sNoiseRenderState.DepthTest = ValueTests::Off;
            sNoiseRenderState.EnableDepthWrite = false;
            skyNoiseShader = new CompiledShader(sNoiseRenderState, shaderPtr, sNoiseShaderParams);

            #pragma endregion

            TEST_CASE("Creating the heightmap Target");
            #pragma region Heightmap texture
            
            TargetStates targetState;
            heightmapTarget = new Target(targetState, { terrainResolution, terrainResolution },
                                         SimpleFormat(FormatTypes::NormalizedUInt,
                                                      SimpleFormatComponents::R,
                                                      SimpleFormatBitDepths::B16),
                                         DepthStencilFormats::Depth_16U,
                                         true, 1U);

            TEST_CHECK_(targetState == +TargetStates::Ready,
                        "Heightmap Target not valid: %s", targetState._to_string());

            #pragma endregion

            TEST_CASE("Creating the sky texture");
            #pragma region Sky Texture

            const uint_fast32_t cubeFaceResolution = 256;
            auto cubeFaceTexel = 1.0f / glm::fvec2(cubeFaceResolution);
            skyTex = new TextureCube(cubeFaceResolution,
                                     SimpleFormat(FormatTypes::Float,
                                                  SimpleFormatComponents::RGB,
                                                  SimpleFormatBitDepths::B32));

            //Generate the data.
            {
                const auto cubeFaceOrientations = GetFacesOrientation();
                std::vector<glm::fvec3> cubePixels;
                cubePixels.reserve(cubeFaceOrientations.size() *
                                       sizeof(glm::fvec3) *
                                       (cubeFaceResolution * cubeFaceResolution));
                for (const auto& face : cubeFaceOrientations)
                    for (uint_fast32_t y = 0; y < cubeFaceResolution; ++y)
                        for (uint_fast32_t x = 0; x < cubeFaceResolution; ++x)
                        {
                            auto uv = (glm::fvec2(x, y) + 0.5f) * cubeFaceTexel;
                            cubePixels.push_back(glm::fvec3(0, 0, 0));
                        }
                skyTex->Set_Color(cubePixels.data());
            }

            #pragma endregion

            TEST_CASE("Creating the sky noise Target");
            #pragma region Sky Noise Target

            std::array skyNoiseOutputs = {
                TargetOutput(std::make_tuple(skyTex, CubeFaces::PosX)),
                TargetOutput(std::make_tuple(skyTex, CubeFaces::NegX)),
                TargetOutput(std::make_tuple(skyTex, CubeFaces::PosY)),
                TargetOutput(std::make_tuple(skyTex, CubeFaces::NegY)),
                TargetOutput(std::make_tuple(skyTex, CubeFaces::PosZ)),
                TargetOutput(std::make_tuple(skyTex, CubeFaces::NegZ))
            };
            skyNoiseTarget = new Target(targetState,
                                        skyNoiseOutputs.data(), (uint32_t)skyNoiseOutputs.size());

            #pragma endregion

            TEST_CASE("Running the ProcTerrain app loop");
        },

        //Update:
        [&](float deltaT) {

            #pragma region GUI logic

            ImGui::Text("Press 'escape' to quit.");

            ImGui::Text("SUN");
            ImGui::PushID("SUN");
            ImGui::Indent();
            doGuiSun();
            ImGui::Unindent();
            ImGui::Dummy({ 1, 10 });
            ImGui::PopID();

            ImGui::Text("CAMERA");
            ImGui::PushID("CAMERA");
            ImGui::Indent();
            ImGui::DragFloat("FoV (vertical)", &camVerticalFOV);
            ImGui::DragFloat("Speed", &camera.MoveSpeed);
            ImGui::Unindent();
            ImGui::Dummy({ 1, 10 });
            ImGui::PopID();

            ImGui::Text("TERRAIN");
            ImGui::PushID("TERRAIN");
            ImGui::Indent();
            doGuiTerrainTransform();
            doGuiTerrainColor();
            ImGui::Unindent();
            ImGui::Dummy({ 1, 10 });
            ImGui::PopID();
            
            ImGui::Text("HEIGHTMAP");
            ImGui::PushID("HEIGHTMAP");
            ImGui::Indent();
            doGuiTNoise();
            ImGui::Unindent();
            ImGui::Dummy({ 1, 10 });
            ImGui::PopID();

            ImGui::Text("SKY");
            ImGui::PushID("SKY");
            ImGui::Indent();
            doGuiSNoise();
            ImGui::Unindent();
            ImGui::Dummy({ 1, 10 });
            ImGui::PopID();

            #pragma endregion

            auto keyStates = SDL_GetKeyboardState(nullptr);
            
            if (keyStates[SDL_SCANCODE_ESCAPE])
            {
                Simple::App->Quit(true);
                return;
            }
            
            #pragma region Camera input

            bool ignoreKeyboard = ImGui::GetIO().WantCaptureKeyboard,
                 ignoreMouse = ImGui::GetIO().WantCaptureMouse;

            camera.InputMoveForward = ignoreKeyboard ? 0 :
                                        ((keyStates[SDL_SCANCODE_W] ? 1.0f : 0) +
                                         (keyStates[SDL_SCANCODE_S] ? -1.0f : 0));
            camera.InputMoveUp = ignoreKeyboard ? 0 :
                                     ((keyStates[SDL_SCANCODE_E] ? 1.0f : 0) +
                                      (keyStates[SDL_SCANCODE_Q] ? -1.0f : 0));
            camera.InputMoveRight = ignoreKeyboard ? 0 :
                                       ((keyStates[SDL_SCANCODE_D] ? 1.0f : 0) +
                                        (keyStates[SDL_SCANCODE_A] ? -1.0f : 0));
            camera.InputSpeedBoost = ignoreKeyboard ? false :
                                         (keyStates[SDL_SCANCODE_LSHIFT] |
                                          keyStates[SDL_SCANCODE_RSHIFT]);
            
            glm::ivec2 mouseMovement;
            auto mouseButtonMask = SDL_GetRelativeMouseState(&mouseMovement.x, &mouseMovement.y);

            camera.EnableRotation = (!ignoreMouse) &
                                    ((mouseButtonMask & SDL_BUTTON(SDL_BUTTON_LEFT)) |
                                     (mouseButtonMask & SDL_BUTTON(SDL_BUTTON_RIGHT)));
            camera.InputCamYawPitch = ignoreMouse ? glm::fvec2{ 0, 0 } : glm::fvec2(mouseMovement);
            camera.InputSpeedChange = ignoreKeyboard ? 0.0f : ImGui::GetIO().MouseWheel;

            #pragma endregion
            
            ImGui::LabelText("Camera Pos",
                             "%f,  %f,  %f",
                             camera.Position.x, camera.Position.y, camera.Position.z);
            ImGui::LabelText("Camera Forward",
                             "%f,  %f,  %f",
                             camera.Forward.x, camera.Forward.y, camera.Forward.z);
            ImGui::LabelText("Camera Up",
                             "%f,  %f,  %f",
                             camera.Up.x, camera.Up.y, camera.Up.z);
            ImGui::LabelText("Camera Turning",
                             "%f,  %f",
                             camera.InputCamYawPitch.x, camera.InputCamYawPitch.y);
            camera.Update(deltaT);

            elapsedTime += deltaT;
        },

        //Render:
        [&](float deltaT) {
            auto& context = *Context::GetCurrentContext();
            glm::ivec2 windowSize;
            SDL_GetWindowSize(Simple::App->MainWindow, &windowSize.x, &windowSize.y);
            
            //Draw into Targets:

            #pragma region Update heightmap

            updateShaderTNoise(*noiseShader);

            heightmapTarget->Activate();
            context.Draw(DrawMeshMode_Basic(*fullScreenMesh, 3), *noiseShader);

            #pragma endregion

            #pragma region Render skybox noise

            updateShaderSNoise(*skyNoiseShader);

            skyNoiseTarget->Activate();
            context.Draw(DrawMeshMode_Basic(*fullScreenMesh, 3), *skyNoiseShader);
            context.ClearActiveTarget();

            skyTex->RecomputeMips();

            #pragma endregion


            //Now draw the world:

            context.ClearActiveTarget();
            auto viewProjMatrix = getProjectionMatrix() * camera.GetViewMat();

            #pragma region Draw terrain

            updateShaderSun(*terrainShader);
            updateShaderTerrainColor(*terrainShader);
            updateShaderTerrainTransform(*terrainShader);

            Sampler<2> heightmapSampler(WrapModes::Clamp, PixelFilters::Smooth);
            auto heightmapView = heightmapTarget->GetOutput_Color()->GetTex2D()->GetView(heightmapSampler);
            terrainShader->SetUniform("u_Heightmap", heightmapView);

            terrainShader->SetUniform("u_ViewProjMatrix", viewProjMatrix);

            context.Draw(DrawMeshMode_Basic(*terrainMesh), *terrainShader,
                         DrawMeshMode_Indexed());

            #pragma endregion

            #pragma region Draw skybox

            skyShader->SetUniform("u_ViewProjMatrix", viewProjMatrix);
            skyShader->SetUniform("u_CamPos", camera.Position);
            skyShader->SetUniform("u_Length", getFarClipPlane());
            skyShader->SetUniform("u_Skybox", skyTex->GetView());

            context.Draw(DrawMeshMode_Basic(*skyCubeMesh, 2 * 3 * 6), *skyShader);

            #pragma endregion
        },

        //Quit:
        [&]() {
            #define TRY_DELETE(x) \
                if (x != nullptr) { \
                    delete x; \
                    x = nullptr; \
                }
            
            TRY_DELETE(fullScreenTri);
            TRY_DELETE(fullScreenMesh);
            TRY_DELETE(noiseShader);
            TRY_DELETE(heightmapTarget);

            TRY_DELETE(terrainUVs);
            TRY_DELETE(terrainIndices);
            TRY_DELETE(terrainMesh);
            TRY_DELETE(terrainShader);

            TRY_DELETE(skyCubePoses);
            TRY_DELETE(skyCubeMesh);
            TRY_DELETE(skyShader);
            TRY_DELETE(skyTex);

            TRY_DELETE(skyNoiseShader);
            TRY_DELETE(skyNoiseTarget);

            #undef TRY_DELETE
        });
}