#pragma once

#include <any>

#define TEST_NO_MAIN
#include <acutest.h>

#include <B+/RenderLibs.h>
#include <glm/gtx/string_cast.hpp>

#include "../SimpleApp.hpp"


void SimpleApps()
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

            ImGui::Text("If you see all four labels including this one,\nPress Enter. \
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

    Buffer* trisCoordinates = nullptr;
    MeshData* tris = nullptr;
    CompiledShader* shader = nullptr;

    //auto shaderPtr = CompiledShader::Compile()
    //CompiledShader shader(RenderState(), CompiledShader::Compile()

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
            trisCoordinates = new Buffer(6, false, trisCoordinatesData.data());

            TEST_CASE("Creating a MeshData for two triangles");
            tris = new MeshData(PrimitiveTypes::Triangle,
                                { MeshDataSource(trisCoordinates, sizeof(glm::fvec2)) },
                                { VertexDataField{0, 2 * sizeof(float), 0,
                                                  MeshVertices::SimpleFVectorType(MeshVertices::VectorSizes::XY,
                                                                                  MeshVertices::SimpleFVectorTypes::Float32)} });

            TEST_CASE("Compiling the shader");
            OglPtr::ShaderProgram shaderPtr;
            #pragma region Shader compilation

            ShaderCompileJob compiler;

            compiler.VertexSrc = R"(
layout (location = 0) in vec2 vIn_Pos;
layout (location = 0) out vec2 vOut_Pos;
void main()
{
    gl_Position = vec4(vIn_Pos, 0, 1);
    vOut_Pos = vIn_Pos;
})";
            compiler.FragmentSrc = R"(
layout (location = 0) in vec2 fIn_Pos;
layout (location = 0) out vec4 fOut_Color;

void main()
{
    vec3 color = vec3(fract(fIn_Pos * 10),
                      abs(sin(gl_FragCoord.y / 50.0)));
    fOut_Color = vec4(color, 1);
})";

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
            shader = new CompiledShader(RenderState(FaceCullModes::Off, ValueTests::Off),
                                        shaderPtr, { });
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
            Context::GetCurrentContext()->Draw(DrawMeshMode_Basic(*tris, 6), *shader);
        },

        //Quit:
        [&]() {
            #define TRY_DELETE(x) \
                if (x != nullptr) { \
                    delete x; \
                    x = nullptr; \
                }

            TRY_DELETE(shader);
            TRY_DELETE(trisCoordinates);
            TRY_DELETE(tris);
            #undef TRY_DELETE
        });
}