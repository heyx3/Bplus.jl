#pragma once

#include <random>

#define TEST_NO_MAIN
#include <acutest.h>

#include <B+/Renderer/Buffer.h>
#include "../SimpleApp.h"


void BufferCreation()
{
    std::unique_ptr<Bplus::GL::Buffer> buffer;
    Simple::Run(
        //Update:
        [&](float deltaT)
        {
            buffer = std::make_unique<Bplus::GL::Buffer>();

            Simple::App->Quit(true);
        },
        //Render:
        [&](float deltaT)
        {

        },
        //Quit:
        [&]()
        {
            buffer.reset();
        });
}


template<typename T, size_t N>
void _BufferGetSetData(Bplus::GL::Buffer& buffer,
                       const char* elTypeName,
                       std::function<std::string(const T&)> elToString,
                       std::function<T()> makeRandomElement,
                       T nullElement)
{
    //Generate the initial data into an array.
    std::array<T, N> data;
    for (T& t : data)
        t = makeRandomElement();

    //Feed the data into a buffer.
    TEST_CASE_("Setting buffer to a group of %s", elTypeName);
    buffer.SetData(data.data(), data.size(),
                   Bplus::GL::BufferHints_Frequency::SetOnce_ReadRarely,
                   Bplus::GL::BufferHints_Purpose::SetGPU_ReadCPU);

    //Read the data back from the buffer into a new array.
    TEST_CASE_("Reading buffer data as a group of %s", elTypeName);
    auto data2 = data;
    std::fill(data2.begin(), data2.end(), nullElement);
    buffer.GetData(data2.data());

    //Compare the original data to the data from the buffer.
    TEST_CASE_("Comparing buffer's '%s' values", elTypeName);
    for (size_t i = 0; i < data2.size(); ++i)
    {
        std::string msg = "Expected buffer[";
        msg += std::to_string(i);
        msg += "] to be ";
        msg += elToString(data[i]);
        msg += ", but it was ";
        msg += elToString(data2[i]);
        TEST_CHECK_(data[i] == data2[i], msg.c_str());
    }
}
void BufferGetSetData()
{
    std::unique_ptr<Bplus::GL::Buffer> buffer;

    //Couch the whole thing inside a "SimpleApp" because we need OpenGL to be initialized.
    Simple::Run(
        //Update:
        [&](float deltaT)
        {
            TEST_CASE("Creating buffer");
            buffer = std::make_unique<Bplus::GL::Buffer>();

            _BufferGetSetData<glm::dvec3, 30>(
                *buffer, "glm::dvec3",
                [](const glm::dvec3& v)
                {
                    std::string str = "{";
                    str += std::to_string(v.x);
                    str += ",";
                    str += std::to_string(v.y);
                    str += ",";
                    str += std::to_string(v.z);
                    str += "}";
                    return str;
                },
                []() { return glm::dvec3{Simple::Rng(), Simple::Rng(), Simple::Rng()} * 100.0; },
                glm::dvec3{-1, -1, -1});

            _BufferGetSetData<bool, 999>(
                *buffer, "bool",
                [](const bool& b) { return std::to_string(b); },
                []() { return (Simple::Rng() > 0.5); },
                false);

            _BufferGetSetData<int16_t, 9999>(
                *buffer, "int16",
                [](const int16_t& i) { return std::to_string(i); },
                []()
                {
                    double t = Simple::Rng();
                    auto i = glm::mix((double)std::numeric_limits<int16_t>().min(),
                                      (double)std::numeric_limits<int16_t>().max(),
                                      t);
                    return (int16_t)i;
                },
                -1);

            _BufferGetSetData<float, 1>(
                *buffer, "float",
                [](const float& f) { return std::to_string(f); },
                []() { return (float)Simple::Rng() * 20; },
                -1);

            TEST_CASE("Quitting");
            Simple::App->Quit(true);
        },
        //Render:
        [&](float deltaT)
        {

        },
        //Quit:
        [&]()
        {
            buffer.reset();
        });
}