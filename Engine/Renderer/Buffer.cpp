#include "Buffer.h"

#include "Context.h"

using namespace Bplus;
using namespace Bplus::GL;


namespace
{
    thread_local struct {
        bool initializedYet = false;
    } threadData;

    void CheckInit()
    {
        if (!threadData.initializedYet)
        {
            threadData.initializedYet = true;

            std::function<void()> refreshContext = []()
            {
                
            };
            refreshContext();
            Context::RegisterCallback_RefreshState(refreshContext);

            Context::RegisterCallback_Destroyed([]()
            {

            });
        }
    }
}


Buffer::Buffer()
{
    CheckInit();

    glGenBuffers(1, &dataPtr.Get());
}
Buffer::~Buffer()
{
    if (dataPtr != OglPtr::Buffer::Null)
        glDeleteBuffers(1, &dataPtr.Get());
}

Buffer::Buffer(Buffer&& src)
    : dataPtr(src.dataPtr), mode(src.mode)
{
    src.dataPtr = OglPtr::Buffer(OglPtr::Buffer::Null);
}
Buffer& Buffer::operator=(Buffer&& src)
{
    //Call deconstructor, then move constructor.
    this->~Buffer();
    new (this) Buffer(std::move(src));

    return *this;
}


//TODO: Implement the rest.