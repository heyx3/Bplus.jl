#pragma once

#include "../Utils.h"


namespace Bplus::Math
{
    //A fast, strong PRNG.
    struct PRNG
    {
        //Source: http://burtleburtle.net/bob/rand/smallprng.html

        uint32_t Seed1, Seed2, Seed3;
        uint32_t State;

        PRNG() : PRNG((uint32_t)time(nullptr)) { }
        PRNG(uint32_t seed)
            : Seed1(seed), Seed2(seed), Seed3(seed),
              State(0xf1ea5eed)
        {
            //Run some iterations before-hand.
            //I think this is to weed out any strange initial behavior,
            //    especially given that we initialize 3 of the 4 state variables
            //    to the same value.
            for (int i = 0; i < 20; ++i)
                NextUInt();
        }


        uint32_t NextUInt()
        {
            uint32_t seed4 = State - Rotate(Seed1, 27);
            
            State = Seed1 ^ Rotate(Seed2, 17);
            Seed1 = Seed2 + Seed3;
            Seed2 = Seed3 + seed4;
            Seed3 = seed4 + State;

            return Seed3;
        }
        uint32_t NextUInt(uint32_t maxExclusive)
        {
            return NextUInt() % maxExclusive;
        }
        uint32_t NextUInt(uint32_t min, uint32_t maxExclusive)
        {
            return NextUInt(maxExclusive - min) + min;
        }

        //Generates a random float in the range [0, 1).
        float NextFloat()
        {
            return NextFloat_1_2() - 1;
        }
        //Generates a random float in the given half-open range.
        float NextFloat(float min, float maxExclusive)
        {
            float t = NextFloat();
            return ((1 - t) * min) + (t * maxExclusive);
        }
        //Generates a random float given a midpoint value and a range.
        float NextFloat_MidAndRange(float midpoint, float range)
        {
            float t = NextFloat();
            return (t * range) + (midpoint - 0.5f);
        }
        //Efficiently generates a random float in the range [1, 2).
        //This is the low-level RNG function for generating floats;
        //    this other ones are just this plus some extra work.
        float NextFloat_1_2()
        {
            //Generate a random integer, then overwrite some of the bits
            //    to guarantee a float in the range [1, 2).

            static_assert(std::numeric_limits<float>::is_iec559,
                          "The platform doesn't use standard IEEE floats, which means"
                            " Bplus::Math::PRNG::NextFloat_1_2() needs to use different code"
                            " to generate its values");

            constexpr uint32_t header      = 0b00111111100000000000000000000000;
            constexpr uint32_t intBitsMask = 0b00000000011111111111111111111111;
            
            uint32_t outputU = header | (NextUInt() & intBitsMask);
            return Reinterpret<uint32_t, float>(outputU);
        }


    private:
        static uint32_t Rotate(uint32_t val, uint32_t amount)
        {
            return (val << amount) |
                   (val >> (32 - amount));
        }
    };
}