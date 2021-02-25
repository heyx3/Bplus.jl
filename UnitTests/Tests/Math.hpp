#pragma once

#include <B+/Math/Math.hpp>

#define TEST_NO_MAIN
#include <acutest.h>

using namespace Bplus;


void PlainMath()
{
    TEST_CASE("Bplus::Math::PadI<>");

    TEST_CHECK_(Math::PadI(3, 5) == 5,
                "PadI(3, 5) should be 5, but it's %i",
                Math::PadI(3, 5));
    TEST_CHECK_(Math::PadI(-3, 5) == 0,
                "PadI(-3, 5) should be 0, but it's %i",
                Math::PadI(-3, 5));

    auto result1 = Math::PadI(glm::uvec4{ 0, 2, 5, 8 }, 5U);
    TEST_CHECK_(result1 == glm::uvec4{ 0 BP_COMMA 5 BP_COMMA 5 BP_COMMA 10 },
                "PadI({ 0, 2, 5, 8 }, 5) should be { 0, 5, 5, 10 }, but it's { %i, %i, %i, %i }",
                result1.x, result1.y, result1.z, result1.w);
}

void GLMHelpers()
{
    auto rotIdentity = Math::RotIdentity<float>();

    auto point1 = glm::one<glm::fvec3>();
    auto samePoint1 = Math::ApplyRotation(rotIdentity, point1);
    TEST_CHECK_(glm::all(glm::epsilonEqual(point1, samePoint1, std::numeric_limits<float>().epsilon())),
                "Point {1, 1, 1} should be unchanged after rotation by identity quaternion! Instead it's { %f, %f, %f }",
                samePoint1.x, samePoint1.y, samePoint1.z);

    auto rot180Z = glm::angleAxis(glm::radians(180.0f), glm::fvec3{ 0, 0, 1 });
    auto point1Rot180Z = Math::ApplyRotation(rot180Z, point1);
    TEST_CHECK_(glm::all(glm::epsilonEqual(point1Rot180Z, glm::fvec3{ -point1.x, -point1.y, point1.z }, 0.00001f)),
                "Point { 1, 1, 1 } should be flipped along X and Y from 'rot180z' quaternion! Instead it's { %f, %f, %f }",
                point1Rot180Z.x, point1Rot180Z.y, point1Rot180Z.x);


    TEST_CASE("Clockwise vs counter clockwise rotations");

    glm::fvec3 point2{ 5, 0, 0 };
    auto point2Rot90Y = Math::ApplyRotation(Math::MakeRotation(glm::fvec3{ 0, 1, 0 }, 90.0f),
                                            point2);
    TEST_CHECK_(glm::all(glm::epsilonEqual(point2Rot90Y, glm::fvec3{ 0, 0, -5 }, 0.000001f)),
                "Point { 5, 0, 0 } should become { 0, 0, -5 } after rotating +90 degrees (clockwise) along the Y axis. Instead it's at { %f, %f, %f }",
                point2Rot90Y.x, point2Rot90Y.y, point2Rot90Y.z);

    auto point2Rot90Z = Math::ApplyRotation(Math::MakeRotation(glm::fvec3{ 0, 0, 1 }, 90.0f),
                                            point2);
    TEST_CHECK_(glm::all(glm::epsilonEqual(point2Rot90Z, glm::fvec3{ 0, 5, 0 }, 0.000001f)),
                "Point { 5, 0, 0 } should become { 0, 5, 0 } after rotating +90 degrees (clockwise) along the Z axis. Instead it's at { %f, %f, %f }",
                point2Rot90Z.x, point2Rot90Z.y, point2Rot90Z.z);

    auto point3Rot90X = Math::ApplyRotation(Math::MakeRotation(glm::fvec3{ 1, 0, 0 }, 90.0f),
                                            glm::fvec3{ 0, 0, 4 });
    TEST_CHECK_(glm::all(glm::epsilonEqual(point3Rot90X, glm::fvec3{ 0, -4, 0 }, 0.000001f)),
                "Point { 0, 0, 4 } should become { 0, -4, 0 } after rotating +90 degrees (clockwise) along the X axis. Instead it's at { %f, %f, %f }",
                point3Rot90X.x, point3Rot90X.y, point3Rot90X.z);


    TEST_CASE("");
}