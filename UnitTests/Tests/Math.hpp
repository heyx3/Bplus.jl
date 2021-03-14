#pragma once

#include <B+/Math/Math.hpp>
#include <B+/Math/PRNG.hpp>

#define TEST_NO_MAIN
#include <acutest.h>

using namespace Bplus;
using namespace Bplus::Math;


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

    TEST_CASE("IsInRange() for uint8_t");
    TEST_ASSERT_(!Math::IsInRange<uint8_t>(256), "256 isn't in range of uint8_t");
    TEST_ASSERT_(!Math::IsInRange<uint8_t>(5000), "5000 isn't in range of uint8_t");
    TEST_ASSERT_(!Math::IsInRange<uint8_t>(-1), "-1 isn't in range of uint8_t");
    TEST_ASSERT_(Math::IsInRange<uint8_t>(255), "255 is in range of uint8_t");
    TEST_ASSERT_(Math::IsInRange<uint8_t>(0), "0 is in range of uint8_t");
    TEST_ASSERT_(Math::IsInRange<uint8_t>(11), "11 is in range of uint8_t");
    TEST_CASE("IsInRange() for int8_t");
    TEST_ASSERT_(!Math::IsInRange<int8_t>(128), "128 isn't in range of int8_t");
    TEST_ASSERT_(!Math::IsInRange<int8_t>(5000), "5000 isn't in range of int8_t");
    TEST_ASSERT_(!Math::IsInRange<int8_t>(-5000), "-5000 isn't in range of int8_t");
    TEST_ASSERT_(Math::IsInRange<int8_t>(-1), "-1 is in range of int8_t");
    TEST_ASSERT_(Math::IsInRange<int8_t>(127), "127 is in range of int8_t");
    TEST_ASSERT_(Math::IsInRange<int8_t>(0), "0 is in range of int8_t");
    TEST_ASSERT_(Math::IsInRange<int8_t>(11), "11 is in range of int8_t");
    TEST_ASSERT_(Math::IsInRange<int8_t>(-128), "-128 is in range of int8_t");

    TEST_CASE("Overflow-/Underflow-safe Add and Sub");
    for (uint16_t _u = 0; _u <= std::numeric_limits<uint8_t>().max(); ++_u)
    {
        auto u = (uint8_t)_u;

        for (uint8_t uLess = 0; uLess < u; ++uLess)
        {
            TEST_ASSERT_(Math::SafeSub(u, uLess).has_value(),
                         "Should be able to subtract uints %i from %i", uLess, u);
            TEST_ASSERT_(!Math::SafeSub(uLess, u).has_value(),
                         "Should not be able to subtract uints %i from %i", u, uLess);

            if (Math::IsInRange<uint8_t>(_u + uLess))
                TEST_ASSERT_(Math::SafeAdd(u, uLess).has_value(),
                             "Should not be able to add UInt8's %i and %i", u, uLess);
            else
                TEST_ASSERT_(!Math::SafeAdd(u, uLess).has_value(),
                             "Should be able to add UInt8's %i and %i", u, uLess);
        }
    }
    for (int16_t _i = std::numeric_limits<int8_t>().min(); _i <= std::numeric_limits<int8_t>().max(); ++_i)
    {
        auto i = (int8_t)_i;

        for (int8_t iLess = std::numeric_limits<int8_t>().min(); iLess < i; ++iLess)
        {
            TEST_ASSERT_(Math::SafeSub(i, iLess).has_value() == Math::IsInRange<int8_t>(_i - iLess),
                         "Subtracting int8 %i from %i. Expected to work: %s",
                         i, iLess, Math::IsInRange<int8_t>(_i - iLess) ? "yes" : "no");
            TEST_ASSERT_(Math::SafeAdd(i, iLess).has_value() == Math::IsInRange<int8_t>(_i + iLess),
                         "Adding int8 %i + %i. Expected to work: %s",
                         i, iLess, Math::IsInRange<int8_t>(_i + iLess) ? "yes" : "no");
        }
    }
}

void Box()
{
    //TODO: More.

    TEST_CHECK_(Box<2, float>().IsEmpty(),
                "Default box should be empty");
}

void PRNG()
{
    Bplus::Math::PRNG myRNG;
    const size_t nTrials = 10000000;
    const float fMin = 45,
                fMax = 67.85f;
    const uint32_t uMin = 3,
                   uMax = 9999;

    //Use a very rough estimate of randomness:
    //    about half the values should be above the midpoint.
    size_t nFloatsAboveMidpoint = 0,
           nUintsAboveMidpoint = 0;

    for (size_t i = 0; i < nTrials; ++i)
    {
        float rF = myRNG.NextFloat_1_2();
        if (rF > 1.5f)
            nFloatsAboveMidpoint += 1;
        TEST_CHECK_(rF >= 1 && rF < 2,
                   "PRNG::Next_1_2() is outside the expected range: %f", rF);

        decltype(myRNG) oldState = myRNG;
        rF = myRNG.NextFloat(fMin, fMax);
        TEST_CHECK_(rF >= fMin && rF <= fMax, //Note that we use <=max instead of <max due to floating-point error
                   "PRNG::Next(%f, %f) is outside the expected range: %f",
                   fMin, fMax, rF);

        uint32_t rU = myRNG.NextUInt();
        if (rU > std::numeric_limits<uint32_t>::max() / 2)
            nUintsAboveMidpoint += 1;

        rU = myRNG.NextUInt(uMin, uMax);
        TEST_CHECK_(rU >= uMin && rU < uMax,
                    "PRNG::NextUInt(%u, %u) is outside the expected range: %u",
                    uMin, uMax, rU);
    }

    double floatRatio = (double)nFloatsAboveMidpoint / nTrials,
           intRatio = (double)nUintsAboveMidpoint / nTrials;
    double epsilon = 0.01;
    TEST_CHECK_(floatRatio >= (0.5 - epsilon) && floatRatio < (0.5 + epsilon),
                "Rough randomness test for PRNG floats has failed: values were above"
                   " the expected midpoint %.1f%% of the time, instead of the expected 50%% (give or take %.1f)",
                floatRatio * 100, epsilon * 100);
    TEST_CHECK_(intRatio >= (0.5 - epsilon) && intRatio < (0.5 + epsilon),
                "Rough randomness test for PRNG UInts has failed: values were above"
                   " the expected midpoint %.1f%% of the time, instead of the expected 50%% (give or take %.1f)",
                intRatio * 100, epsilon * 100);
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


    TEST_CASE("5+ dimensional GLM");
    glm::vec<5, float> fV5 = { 1, 2, 3, 4, 5 };
    for (glm::length_t n = 0; n < fV5.size(); ++n)
        TEST_CHECK_(fV5[n] == n + 1, "Accessing elements of a 'fvec5'");
    fV5 *= 2;
    for (glm::length_t n = 0; n < fV5.size(); ++n)
        TEST_CHECK_(fV5[n] == (n + 1) * 2, "Modifying an 'fvec5'");
    //TODO: Test glm::length() as well
}