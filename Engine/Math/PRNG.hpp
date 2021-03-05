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
            //I think this is to weed out any strange initial behavior for some seeds.
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
        //TODO: NextFloat()


    private:
        static uint32_t Rotate(uint32_t val, uint32_t amount)
        {
            return (val << amount) |
                   (val >> (32 - amount));
        }
    };
}