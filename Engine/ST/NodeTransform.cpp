#include "NodeTransform.h"

#include <glm/gtx/matrix_decompose.hpp>


using namespace Bplus::ST;


NodeTransform::NodeTransform(Scene& _world,
                             glm::fvec3 localPos, glm::fquat localRot, glm::fvec3 localScale,
                             NodeID desiredParent)
    : localPos(localPos), localRot(localRot), localScale(localScale),
      world(&_world)
{
    SetParent(desiredParent);
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
        if (parent != entt::null)
            //Note: this is less readable than I'd like, but I think it's the highest-performance version,
            //    writing directly into the original value.
            *cachedWorldRot *= world->get<NodeTransform>(parent).GetWorldRot();
    }

    return *cachedWorldRot;
}
const glm::fmat4& NodeTransform::GetWorldMatrix() const
{
    //Update the cache if needed.
    if (!cachedWorldMatrix.has_value())
    {
        cachedWorldMatrix = GetLocalMatrix();
        if (parent != entt::null)
            //Note: this is less readable than I'd like, but I think it's the highest-performance version,
            //    writing directly into the original value.
            *cachedWorldMatrix *= world->get<NodeTransform>(parent).GetWorldMatrix();

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

void NodeTransform::SetParent(NodeID newParentID, Spaces preserve) 
{
    if (parent == newParentID)
        return;

    NodeID myID = entt::to_entity(*world, *this);

    BPAssert(!IsDeepChildOf(newParentID),
             "Trying to create a loop of parents");
     
    //Update this node's "NodeRoot" component.
    if (newParentID != entt::null)
        world->remove_if_exists<NodeRoot>(myID);
    else if (!world->has<NodeRoot>(myID))
        world->emplace<NodeRoot>(myID);

    //Get the parents so we can let them know about the change.
    NodeTransform* currentParent = (parent != entt::null) ?
                                     world->try_get<NodeTransform>(parent) :
                                     nullptr;
    NodeTransform* newParent = (newParentID != entt::null) ?
                                   world->try_get<NodeTransform>(parent) :
                                   nullptr;
     
    //Update the parents/siblings.
    if (parent != entt::null)
    {
        _DisconnectParent(myID, currentParent);
    }
    if (newParentID != entt::null)
    {
        //The simplest method is to insert this child at the beginning of the list.
        this->nextSibling = newParent->firstChild;
        this->prevSibling = entt::null;
        newParent->firstChild = myID;

        newParent->nChildren += 1;
        
        //Note that the setting of 'this->parent' is done later, below.
    }

    //Handle transform data and update the parent field:
    switch (preserve)
    {
        case Spaces::Local: {
            //Leave local position alone, but world position will change.
            parent = newParentID;
            InvalidateWorldMatrix(true);
        } break;

        case Spaces::World: {
            //The local position, rotation, and scale will change,
            //    but the world transform should stay the same,
            //    which means the cached world transform (and child nodes' caches)
            //    stays the same.

            const auto &worldMatrix = GetWorldMatrix(),
                       &inverseWorldMatrix = GetWorldInverseMatrix();
            cachedLocalMatrix = Math::ApplyTransform(worldMatrix, inverseWorldMatrix);

            glm::fvec3 newSkew;
            glm::fvec4 newPerspective;
            bool success = glm::decompose(*cachedLocalMatrix, localScale, localRot, localPos, newSkew, newPerspective);
            
            parent = newParentID;

            BPAssert(success, "Failed to recalculate local matrix on NodeTransform::SetParent()");
        } break;


        default: {
            parent = newParentID;

            std::string errMsg = "Unknown space mode ";
            errMsg += preserve._to_string();
            BPAssert(false, errMsg.c_str());
        } break;
    }
}

void NodeTransform::_DisconnectParent(NodeID myID, NodeTransform* parentPtr)
{
    //Find the parent.
    if (parentPtr == nullptr)
    {
        parentPtr = (parent != entt::null) ?
                        world->try_get<NodeTransform>(parent) :
                        nullptr;
    }

    //Handle the parent.
    if (parentPtr != nullptr && parentPtr->firstChild == myID)
    {
        BPAssert(prevSibling == entt::null,
                 "I am my parent's first child, but I have a previous sibling??");
        
        parentPtr->firstChild = nextSibling;
    }
    parentPtr->nChildren -= 1;

    //Handle any siblings.
    if (prevSibling != entt::null)
    {
        auto& sibling = world->get<NodeTransform>(prevSibling);
        BPAssert(sibling.nextSibling == myID,
                    "My 'previous' sibling has a different 'next' sibling; it isn't me");
        sibling.nextSibling = this->nextSibling;
    }
    if (nextSibling != entt::null)
    {
        auto& sibling = world->get<NodeTransform>(nextSibling);
        BPAssert(sibling.prevSibling == myID,
                    "My 'next' sibling has a different 'previous' sibling; it isn't me");
        sibling.prevSibling = this->prevSibling;
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
            for (NodeID childID : IterChildren())
            {
                auto child = world->get<NodeTransform>(childID);
                BPAssert(!child.cachedWorldMatrix.has_value(),
                         "Child node has a valid world matrix while the direct parent has an invalid one");
                BPAssert((!includeRot) | (!child.cachedWorldRot.has_value()),
                         "Child node has a valid world rotation while the direct parent has an invalid one");
            }
        }

        return;
    }

    //Assert that our parent is already invalidated, since we are about to be.
    if (BPIsDebug && parent != entt::null)
    {
        const auto& parentNode = world->get<NodeTransform>(parent);
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
    for (NodeID child : IterChildren())
        world->get<NodeTransform>(child).InvalidateWorldMatrix(includeRot);
}

bool NodeTransform::IsDeepChildOf(NodeID parentID) const
{
    //Walk upwards until we find a matching parent, or the root.
    auto iter = IterParents();
    return std::any_of(iter.begin(), iter.end(),
                       [parentID](NodeID deepParent) { return parentID == deepParent; });
}