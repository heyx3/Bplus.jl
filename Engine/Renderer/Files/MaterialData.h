#pragma once

#include <string>
#include <memory>
#include "../Helpers.h"

namespace Bplus::Files
{
    //TODO: Scrap this file. Make a non-abstract Matrial class,
    //    then make each "Factory" a wrapper class inheriting from "MaterialFactory".


    //Abstract class for any kind of material.
    class BP_API Material
    {   
    public:

        Bplus::GL::FaceCullModes FaceCull = GL::FaceCullModes::On;

        //The user's custom "configuration" step, near the top of the shader file(s).
        std::string Code_Configuration;
        //The user's custom "definition" step, defining any helper functions,
        //    constants, etc.
        std::string Code_Definitions;


        static std::unique_ptr<Material> Load(std::istream& tomlFile);
        void Save(std::ostream& tomlFile) const;

        void GenerateShader_Vertex(std::ostream& outText) const;
        //TODO: Figure out geometry shaders.
        void GenerateShader_Fragment(std::ostream& outText) const;
    };
}