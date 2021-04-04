#pragma once

//Provides a way to access a matrix/vector as an array of floats.
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_access.hpp>

#include "../Context.h"
#include "UniformDataStructures.h"
#include "../Textures/TextureD.hpp"


namespace Bplus::GL::Uniforms
{
    //Manages GPU resources for shader uniforms.
    //For example, a "gradient" needs to be sent to the GPU as a Texture1D.
    class BP_API UniformStorage
    {
    public:

        UniformStorage(const UniformDefinitions& uniforms);


        const Textures::Texture1D& GetGradient(const std::string& name) const;
        void SetGradient(const std::string& name,
                         const Uniforms::GradientValue_t& newValue);

    private:

        std::unordered_map<std::string, Textures::Texture1D> gradients;

        std::vector<glm::fvec4> bufferRGBA;
    };
}