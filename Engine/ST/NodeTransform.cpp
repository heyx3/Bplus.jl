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
        cachedLocalMatrix = Math::ApplyTransform(glm::mat4_cast(localRot), *cachedLocalMatrix);
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


const glm::fquat& NodeTransform::GetWorldRot() const
{
    //Update the cache if needed.
    if (!cachedWorldRot.has_value())
    {
        cachedWorldRot = localRot;
        if (parent.has_value())
            //Note: this is less readable than I'd like, but I think it's the highest-performance version,
            //    writing directly into the original value.
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
            //Note: this is less readable than I'd like, but I think it's the highest-performance version,
            //    writing directly into the original value.
            *cachedWorldMatrix *= world.get<NodeTransform>(*parent).GetWorldMatrix();

        cachedWorldInverseMatrix = glm::inverse(*cachedWorldMatrix);
    }

    return *cachedWorldMatrix;
}
const glm::fmat4& NodeTransform::GetWorldInverseMatrix() const
{
    //Make sure the cached value is up-to-date first.
    GetWorldMatrix();

    return cachedWorldInverseMatrix;
}

void NodeTransform::SetParent(NodeID myID, std::optional<NodeID> newParentID,
                              Spaces preserve) 
{
    //Error-checking and edge cases:
    BPAssert(&world.get<NodeTransform>(myID) == this,
             "Passed the wrong ID for 'myID' in NodeTransform::SetParent()");
    if (parent == newParentID)
        return;

    //Get the parents so we can let them know about the change.
    NodeTransform* currentParent = parent.has_value() ? world.try_get<NodeTransform>(*parent) :
                                                        nullptr;
    NodeTransform* newParent = newParentID.has_value() ? world.try_get<NodeTransform>(*parent) :
                                                         nullptr;
     
    //Update this node's "NodeRoot" component.
    if (newParentID.has_value())
        world.remove_if_exists<NodeRoot>(myID);
    else
        world.get_or_emplace<NodeRoot>(myID);

    //Update the parents.
    if (parent.has_value())
    {
        auto& oldSiblings = currentParent->children;
        auto found = std::find(oldSiblings.begin(), oldSiblings.end(), myID);
        BPAssert(found != oldSiblings.end(), "Can't find myself in my parent's child list?");
        oldSiblings.erase(found);
    }
    if (newParentID.has_value())
    {
        auto& newSiblings = newParent->children;
        BPAssert(std::find(newSiblings.begin(), newSiblings.end(), myID) == newSiblings.end(),
                 "Somehow I already existed in the new parent's child list?");

        newSiblings.push_back(myID);
    }

    //Handle transform data and update the parent field:
    switch (preserve)
    {
        case Spaces::Local:
            //Leave local position alone, but world position will change.
            parent = newParentID;
            InvalidateWorldMatrix(true);
        break;

        case Spaces::World:
            //The local position, rotation, and scale will change,
            //    but the world transform should stay the same,
            //    which means the cached world transform (and child nodes' caches)
            //    stays the same.

            auto worldMatrix = GetWorldMatrix(),
                 inverseWorldMatrix = GetWorldInverseMatrix();
            cachedLocalMatrix = Math::ApplyTransform(worldMatrix, inverseWorldMatrix);

            glm::fvec3 newSkew;
            glm::fvec4 newPerspective;
            bool success = glm::decompose(*cachedLocalMatrix, localScale, localRot, localPos, newSkew, newPerspective);
            
            parent = newParentID;

            BPAssert(success, "Failed to recalculate local matrix on NodeTransform::SetParent()");
        break;


        default:
            parent = newParentID;

            std::string errMsg = "Unknown space mode ";
            errMsg += preserve._to_string();
            BPAssert(false, errMsg.c_str());
        break;
    }
}

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