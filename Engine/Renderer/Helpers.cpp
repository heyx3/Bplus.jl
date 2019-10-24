#include "Helpers.h"

#include "../RenderLibs.h"

using namespace Bplus;


void GL::Clear(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
}
void GL::Clear(float depth)
{
    glClearDepth(depth);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void GL::Clear(float r, float g, float b, float a, float depth)
{
    glClearColor(r, g, b, a);
    glClearDepth(depth);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
}

void GL::SetViewport(int minX, int minY, int width, int height)
{
    glViewport(minX, minY, width, height);
}


void GL::SetScissor(int minX, int minY, int width, int height)
{
    glEnable(GL_SCISSOR_TEST);
    glScissor(minX, minY, width, height);
}
void GL::DisableScissor()
{
    glDisable(GL_SCISSOR_TEST);
}
bool GL::IsScissorEnabled()
{
    GLboolean output;
    glGetBooleanv(GL_SCISSOR_TEST, &output);
    return output == GL_TRUE;
}