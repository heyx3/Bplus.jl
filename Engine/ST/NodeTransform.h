#pragma once

#include "../Math/Math.h"

#include "DataTypes.h"


namespace Bplus::ST
{
    //An empty component that simply marks a node as being a "root" (i.e. it has no parent).
    struct BP_API NodeRoot { };

    inline auto AllRootNodes(const ECS& world) { return world.view<NodeRoot>(); }


    //An enum distinguishing world-space from local-space.
    BETTER_ENUM(Spaces, uint8_t,
        World, Local
    );


    //A node in the scene tree,
    //    with a position, rotation, and scale relative to its parent.
    //Note that the ECS "world" cannot be moved while nodes exist.
    struct BP_API NodeTransform
    {
        NodeTransform(ECS& world,
                      glm::fvec3 localPos = { 0, 0, 0 },
                      glm::fquat localRot = Math::RotIdentity<float>(),
                      glm::fvec3 localScale = { 1, 1, 1 },
                      std::optional<NodeID> parent = std::nullopt);


        ECS& GetOwner() const { return world; }

        #pragma region Local-space getters/setters

        const glm::fvec3& GetLocalPos() const { return localPos; }
        const glm::fquat& GetLocalRot() const { return localRot; }
        const glm::fvec3& GetLocalScale() const { return localScale; }
        const glm::fmat4& GetLocalMatrix() const;


        void SetLocalPos(const glm::fvec3& newPos);
        void SetLocalRot(const glm::fquat& newRot);
        void SetLocalScale(const glm::fvec3& newScale);

        //If the matrix is not a valid transform, then
        //    this operation fails, nothing changes, and "false" is returned.
        //Otherwise, if it succeeded, returns "true".
        //Note that, while strange transforms like "skew" can be used here,
        //    they will disappear as soon as the position, rotation, or scale are changed.
        bool SetLocalMatrix(const glm::fmat4& newLocalMat);


        void AddLocalPos(const glm::fvec3& delta)           { SetLocalPos(delta + GetLocalPos()); }
        void AddLocalRot(const glm::fquat& delta)           { SetLocalRot(delta * GetLocalRot()); }
        void MultiplyLocalScale(const glm::fvec3& modifier) { SetLocalScale(modifier * GetLocalScale()); }
        bool ApplyTransform(const glm::fmat4& transform)    { return SetLocalMatrix(transform * GetLocalMatrix()); }

        #pragma endregion

        #pragma region World-space getters/setters

        //TODO: Create "GetWorldScale()", by chopping out the relevant bits of glm::decompose()

        glm::fvec3 GetWorldPos() const;
        const glm::fquat& GetWorldRot() const;
        const glm::fmat4& GetWorldMatrix() const;


        void SetWorldPos(const glm::fvec3& newPos);
        void SetWorldRot(const glm::fquat& newRot);

        //If the matrix is not a valid transform, then
        //    this operation fails, nothing changes, and "false" is returned.
        //Otherwise, if it succeeded, returns "true".
        //Note that, while strange transforms like "skew" can be used here,
        //    they will disappear as soon as the position, rotation, or scale are changed.
        bool SetWorldMatrix(const glm::fmat4& newMat);


        void AddWorldPos(const glm::fvec3& delta)         { SetWorldPos(delta + GetWorldPos()); }
        void AddWorldRot(const glm::quat& delta)          { SetWorldRot(delta * GetWorldRot()); }
        bool ApplyTransform(const glm::fmat4& transform)  { return SetWorldMatrix(transform * GetWorldMatrix()); }

        #pragma endregion

        #pragma region Parent/child getters and setters

        std::optional<NodeID> GetParent() const { return parent; }
        const ShortVector<NodeID>& GetChildren() const { return children; }

        //Changes this node's parent, and leaves its local-space OR
        //    world-space transform unchanged.
        //Preserving local-space is much faster than world-space.
        void SetParent(std::optional<NodeID> newValue, Spaces preserve = Spaces::Local);

        #pragma endregion


    private:
        ECS& world;
        std::optional<NodeID> parent;
        ShortVector<NodeID> children;

        glm::fvec3 localPos, localScale;
        glm::fquat localRot;

        mutable std::optional<glm::fmat4> cachedLocalMatrix, cachedWorldMatrix;
        mutable std::optional<glm::fquat> cachedWorldRot;

        void InvalidateWorldMatrix(bool includeRot) const;
    };


    //TODO: An iterator for all nodes under a given one.
}