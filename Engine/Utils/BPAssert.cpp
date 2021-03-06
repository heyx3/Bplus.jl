#include "BPAssert.h"

#include <assert.h>
#include <cstdio>
#include <exception>


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
    //NOTE: If we ever upgrade to C++ 20, we should use [[unlikely]]
    //    to hint to the compiler that these asserts usually pass.
    //For now, we exploit the fact that most compilers will
    //    assume the 'else' statement is less likely.
    if (expr)
    {
        //For completeness:
        assert(true);
    }
    else
    {
        printf("BP_ASSERT failed: ");
        printf(msg);
        printf("\n");

        assert(false);
        throw std::exception(msg);
    }
}