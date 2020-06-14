#pragma once

#include <random>

#define TEST_NO_MAIN
#include <acutest.h>

#include <B+/Renderer/Buffers/Buffer.h>
#include "../SimpleApp.hpp"

#include <glm/gtx/string_cast.hpp>

using BP_Buffer = Bplus::GL::Buffers::Buffer;


void BufferBasic()
{
    Simple::RunTest([&]()
    {
        //Buffer 1 contains a single 4D vector of doubles.
        
        TEST_CASE("Buffer1");
        BP_Buffer buffer1(sizeof(glm::dvec4), false);

        TEST_CASE("Write Buffer1");
        glm::dvec4 buffer1DataIn = { 5.0, 4.0, 3.0, 1.0 };
        TEST_CASE("Read Buffer1");
        glm::dvec4 buffer1DataOut = { -1, -1, -1, -1 };
        buffer1.Get<glm::dvec4>(&buffer1DataOut);
        TEST_CHECK_(buffer1DataIn == buffer1DataOut,
                    "Expected: %s    Got: %s",
                    glm::to_string(buffer1DataIn).c_str(),
                    glm::to_string(buffer1DataOut).c_str());


        //Buffer 2 contains 5 arbitrary bytes.

        TEST_CASE("Buffer2");
        std::byte data5[5];
        for (int i = 0; i < 5; ++i)
            data5[i] = (std::byte)(i + 1);
        BP_Buffer buffer2(5, true, data5, true);

        TEST_CASE("Read Buffer2");
        for (int i = 0; i < 5; ++i)
            data5[i] = (std::byte)0;
        buffer2.GetBytes(data5);
        for (int i = 0; i < 5; ++i)
        {
            TEST_CHECK_((int)data5[i] == (i + 1),
                        "data5[%i] == %i", (int)data5[i], (int)data5[i] + 1);
        }
    });
}


template<typename T, size_t N>
void _BufferGetSetData(BP_Buffer& buffer,
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
    buffer.Set<T>(data.data(),
                  Bplus::Math::IntervalUL::MakeSize(glm::u64vec1{ data.size() }));

    //Read the data back from the buffer into a new array.
    TEST_CASE_("Reading buffer data as a group of %s", elTypeName);
    auto data2 = data;
    std::fill(data2.begin(), data2.end(), nullElement);
    buffer.Get<T>(data2.data(),
                  Bplus::Math::IntervalUL::MakeSize(glm::u64vec1{ data2.size() }));

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
    Simple::RunTest([&]()
    {
        TEST_CASE("Creating buffer");

        BP_Buffer buffer(1024 * 1024, true);

        _BufferGetSetData<glm::dvec3, 30>(
            buffer, "glm::dvec3",
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
            buffer, "bool",
            [](const bool& b) { return std::to_string(b); },
            []() { return (Simple::Rng() > 0.5); },
            false);

        _BufferGetSetData<int16_t, 9999>(
            buffer, "int16",
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
            buffer, "float",
            [](const float& f) { return std::to_string(f); },
            []() { return (float)Simple::Rng() * 20; },
            -1);
    });
}