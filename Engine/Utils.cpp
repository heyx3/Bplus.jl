#include "Utils.h"

#include <iostream>
#include <assert.h>

namespace
{
    Bplus::AssertFuncSignature currentAssertFunc = Bplus::DefaultAssertFunc;
}

void Bplus::SetAssertFunc(Bplus::AssertFuncSignature f)
{
    currentAssertFunc = f;
}
Bplus::AssertFuncSignature Bplus::GetAssertFunc()
{
    return currentAssertFunc;
}

void Bplus::DefaultAssertFunc(bool expr, const char* msg)
{
    if (!expr)
    {
        std::cout << "BPAssert failed: " << msg << "\n";
        assert(false);
    }
    else
    {
        //Just for completeness:
        assert(true);
    }
}