#include "NodeTransform.h"

#include <glm/gtx/matrix_decompose.hpp>


using namespace Bplus::ST;


NodeTransform::NodeTransform(ECS& world,
                             glm::fvec3 localPos, glm::fquat localRot, glm::fvec3 localScale,
                             std::optional<NodeID> parent)
    : localPos(localPos), localRot(localRot), localScale(localScale),
      world(world), parent(parent)
{

}


const glm::fmat4& NodeTransform::GetLocalMatrix() const
{
    if (!cachedLocalMatrix.has_value())
    {
        cachedLocalMatrix = glm::mat4(1);
        cachedLocalMatrix = glm::translate(*cachedLocalMatrix, localPos);
        cachedLocalMatrix = *cachedLocalMatrix * glm::mat4_cast(localRot);
        cachedLocalMatrix = glm::scale(*cachedLocalMatrix, localScale);
    }

    return *cachedLocalMatrix;
}

void NodeTransform::SetLocalPos(const glm::fvec3& newPos)
{
    localPos = newPos;

    cachedLocalMatrix = std::nullopt;
    InvalidateWorldMatrix(false);
}
void NodeTransform::SetLocalRot(const glm::fquat& newRot)
{
    localRot = newRot;

    cachedLocalMatrix = std::nullopt;
    InvalidateWorldMatrix(true);
}
void NodeTransform::SetLocalScale(const glm::fvec3& newScale)
{
    localScale = newScale;

    cachedLocalMatrix = std::nullopt;
    InvalidateWorldMatrix(false);
}
bool NodeTransform::SetLocalMatrix(const glm::fmat4& newMat)
{
    //Pull the position, rotation, etc. out of the new matrix.
    glm::fvec3 newPos, newScale, newSkew;
    glm::fquat newRot;
    glm::fvec4 newPerspective;
    bool success = glm::decompose(newMat, newScale, newRot, newPos, newSkew, newPerspective);

    if (!success)
        return false;

    localPos = newPos;
    localRot = newRot;
    localScale = newScale;
    cachedLocalMatrix = newMat;

    InvalidateWorldMatrix(true);

    return true;
}


glm::fvec3 NodeTransform::GetWorldPos() const
{
    glm::fvec4 pos4 = GetWorldMatrix() * glm::fvec4(GetLocalPos(), 1);
    return glm::fvec3(pos4.x, pos4.y, pos4.z) / pos4.w;
}
const glm::fquat& NodeTransform::GetWorldRot() const
{
    //Update the cache if needed.
    if (!cachedWorldRot.has_value())
    {
        cachedWorldRot = localRot;
        if (parent.has_value())
            *cachedWorldRot *= world.get<NodeTransform>(*parent).GetWorldRot();
    }

    return *cachedWorldRot;
}
const glm::fmat4& NodeTransform::GetWorldMatrix() const
{
    //Update the cache if needed.
    if (!cachedWorldMatrix.has_value())
    {
        cachedWorldMatrix = GetLocalMatrix();
        if (parent.has_value())
            *cachedWorldMatrix *= world.get<NodeTransform>(*parent).GetWorldMatrix();
    }
}

//TODO: SetWorldX()


//TODO: SetParent()

void NodeTransform::InvalidateWorldMatrix(bool includeRot) const
{
    //Skip the work if it's already invalidated.
    if ((!cachedWorldMatrix.has_value()) & ((!includeRot) | (!cachedWorldRot.has_value())))
    {
        //Assert that no child has a "valid" world cache, while this one was already invalidated.
        if (BPIsDebug)
        {
            for (NodeID childID : children)
            {
                auto child = world.get<NodeTransform>(childID);
                BPAssert(!child.cachedWorldMatrix.has_value(),
                         "Child node has a valid world matrix while the direct parent has an invalid one");
                BPAssert((!includeRot) | (!child.cachedWorldRot.has_value()),
                         "Child node has a valid world rotation while the direct parent has an invalid one");
            }
        }

        return;
    }

    //Assert that our parent is already invalidated, since we are about to be.
    if (BPIsDebug && parent.has_value())
    {
        const auto& parentNode = world.get<NodeTransform>(*parent);
        BPAssert(!parentNode.cachedWorldMatrix.has_value(),
                 "Parent node's cached world matrix is still valid while this node's is being invalidated");
        BPAssert((!includeRot) | (!parentNode.cachedWorldRot.has_value()),
                 "Parent node's cached world rotation is still valid while this node's is being invalidated");
    }

    //Invalidate this node's caches.
    cachedWorldMatrix = std::nullopt;
    if (includeRot)
        cachedWorldRot = std::nullopt;

    //Invalidate all childrens' caches.
    for (NodeID child : children)
        world.get<NodeTransform>(child).InvalidateWorldMatrix(includeRot);
}