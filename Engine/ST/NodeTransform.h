#pragma once

#include "../Math/Math.hpp"

#include "Scene.h"


namespace Bplus::ST
{
    //An empty component that simply marks a node as being a "root" (i.e. it has no parent).
    using NodeRoot = entt::tag<ENTT_SYMBOL("B+ Node Root")>;

    inline auto AllRootNodes(const Scene& world) { return world.view<NodeRoot>(); }
    inline auto AllRootNodes(      Scene& world) { return world.view<NodeRoot>(); }


    //An enum distinguishing world-space from local-space.
    BETTER_ENUM(Spaces, uint8_t,
        World, Local
    );
    
    //A node in the scene tree,
    //    with a position, rotation, and scale relative to its parent.
    //Note that the Scene is stored in here as a pointer,
    //    and so it cannot move until all nodes are destroyed.
    //TODO: Support both 2- and 3-dimensional scene graphs using a template.
    struct BP_API NodeTransform
    {
        NodeTransform(Scene& world,
                      glm::fvec3 localPos = { 0, 0, 0 },
                      glm::fquat localRot = Math::RotIdentity<float>(),
                      glm::fvec3 localScale = { 1, 1, 1 },
                      NodeID parent = entt::null);


        const Scene& GetScene() const { return *world; }

        #pragma region Local-space getters/setters

        const glm::fvec3& GetLocalPos() const { return localPos; }
        const glm::fquat& GetLocalRot() const { return localRot; }
        const glm::fvec3& GetLocalScale() const { return localScale; }

        //Gets the matrix that applies this transform's local-space position, rotation, and scale.
        const glm::fmat4& GetLocalMatrix() const;


        void SetLocalPos(const glm::fvec3& newPos);
        void SetLocalRot(const glm::fquat& newRot);
        void SetLocalScale(const glm::fvec3& newScale);

        //Sets the matrix representing this object's local position, rotation, and scale.
        //If the matrix is not a valid transform, then
        //    this operation fails, nothing changes, and "false" is returned.
        //Otherwise, if it succeeded, returns "true".
        //Note that, while strange transforms like "skew" can be used here,
        //    they will disappear as soon as the position, rotation, or scale are changed.
        bool SetLocalMatrix(const glm::fmat4& newLocalMat);


        void AddLocalPos(const glm::fvec3& delta)              { SetLocalPos(delta + GetLocalPos()); }
        void AddLocalRot(const glm::fquat& delta)              { SetLocalRot(delta * GetLocalRot()); }
        void MultiplyLocalScale(const glm::fvec3& modifier)    { SetLocalScale(modifier * GetLocalScale()); }

        //Note that the transform you give here is effectively centered around this node's parent
        //    (or the world origin, if no parent).
        bool AddLocalTransform(const glm::fmat4& transform)    { return SetLocalMatrix(Math::ApplyTransform(GetLocalMatrix(), transform)); }

        #pragma endregion

        #pragma region World-space getters/setters


              glm::fvec3  GetWorldPos() const { return Math::ApplyToPoint(GetWorldMatrix(), GetLocalPos()); }
        const glm::fquat& GetWorldRot() const;
        //TODO: Create "GetWorldScale()", by chopping out the relevant bits of glm::decompose()

        //Gets the matrix that transforms from local space into world space.
        const glm::fmat4& GetWorldMatrix() const;
        //Gets the matrix that transforms from world space into local space.
        const glm::fmat4& GetWorldInverseMatrix() const;


        void SetWorldPos(const glm::fvec3& newPos) { SetLocalPos(Math::ApplyToPoint(GetWorldInverseMatrix(), newPos)); }
        void SetWorldRot(const glm::fquat& newRot) { SetLocalRot(Math::ApplyTransform(newRot, glm::inverse(GetWorldRot()))); }

        //Sets the matrix representing this object's world position, rotation, and scale.
        //If the matrix is not a valid transform (in local-space), then
        //    this operation fails, nothing changes, and "false" is returned.
        //Otherwise, if it succeeded, returns "true".
        //Note that, while strange transforms like "skew" can be used here,
        //    they will disappear as soon as the position, rotation, or scale are changed.
        bool SetWorldMatrix(const glm::fmat4& newMatrix) { return SetLocalMatrix(Math::ApplyTransform(newMatrix, GetWorldInverseMatrix())); }


        void AddWorldPos(const glm::fvec3& delta)            { SetWorldPos(delta + GetWorldPos()); }
        void AddWorldRot(const glm::quat& delta)             { SetWorldRot(delta * GetWorldRot()); }
        bool AddWorldTransform(const glm::fmat4& transform)  { return SetWorldMatrix(Math::ApplyTransform(GetWorldMatrix(), transform)); }

        #pragma endregion

        #pragma region Parent/child getters and setters

        //Returns this node's parent, or entt::null if it is a root object.
        NodeID GetParent() const { return parent; }

        //Gets the number of children this node has.
        uint_fast32_t GetNChildren() const { return nChildren; }
        //Returns this node's first child (children are stored as a linked list),
        //    or entt::null if it has none.
        NodeID GetFirstChild() const { return firstChild; }

        //Returns this node's next sibling in the linked list of children.
        //Returns entt::null if this is the last child.
        NodeID GetNextSibling() const { return nextSibling; }
        //Returns this node's previous sibling in the linked list of children.
        //Returns entt::null if this is the first child.
        NodeID GetPreviousSibling() const { return prevSibling; }

        //Changes this node's parent, and leaves its local-space OR
        //    world-space transform unchanged.
        //Preserving local-space is much faster than world-space.
        //This node will be inserted at the beginning of the child list.
        void SetParent(NodeID newValue, Spaces preserve = Spaces::Local);
        //Disconnects this node from its parent, turning it into a root node.
        //Preserves either its world-space or local-space position.
        void DisconnectParent(Spaces preserve = Spaces::Local) { SetParent(entt::null, preserve); }

        //TODO: Ability to move this transform inside its parent list, and/or swap with another index


        //Gets whether this node can be found somewhere underneath the given 'parent' node.
        bool IsDeepChildOf(NodeID parentID) const;

        #pragma endregion

        #pragma region Iterators

        //Returns an iterable object representing this node's children.
        auto IterChildren();
        //Returns an iterable object representing this node's children.
        auto IterChildren() const;

        //Returns an iterable object representing this node's parents,
        //    from immediate parent up to the root.
        auto IterParents();
        //Returns an iterable object representing this node's parents,
        //    from immediate parent up to the root.
        auto IterParents() const;

        //Returns an iterable object representing all nodes underneath this node.
        //Runs in breadth-first mode.
        auto IterTreeBreadth(bool includeSelf);
        //Returns an iterable object representing all nodes underneath this node.
        //Runs in breadth-first mode.
        auto IterTreeBreadth(bool includeSelf) const;

        //Returns an iterable object representing all nodes underneath this node.
        //Runs in depth-first mode.
        auto IterTreeDepth(bool includeSelf);
        //Returns an iterable object representing all nodes underneath this node.
        //Runs in depth-first mode.
        auto IterTreeDepth(bool includeSelf) const;

        #pragma endregion


    private:
        Scene* world; //Is a pointer instead of a reference so we can have move assignment.

        NodeID parent = entt::null;
        NodeID nextSibling = entt::null,
               prevSibling = entt::null;
        NodeID firstChild = entt::null;
        uint_fast32_t nChildren = 0;

        glm::fvec3 localPos, localScale;
        glm::fquat localRot;

        mutable std::optional<glm::fmat4> cachedLocalMatrix, cachedWorldMatrix;
        mutable std::optional<glm::fquat> cachedWorldRot;
        mutable glm::fmat4 cachedWorldInverseMatrix;


        void InvalidateWorldMatrix(bool includeRot) const;


        //A callback that is automatically hooked into by the ECS
        //    for when this component is destroyed.
        void _DisconnectParent(NodeID myID, NodeTransform* cachedParent = nullptr);

        //Allow the ECS to hook into _DisconnectParent()
        friend Scene;
    };

    #pragma region NodeTransform Iterators
    
    namespace Iterators
    {
        //TODO: Pull out the non-const stuff from these iterators so that you can't magically make a non-const iterator from a const NodeTransform

        //Iterates all children of a node.
        struct BP_API Children
        {
            const NodeTransform* Root;

            #pragma region Implementation

            //Type-defs to make copy-pasting less dangerous.
            using MyIter_t = Bplus::ST::Iterators::Children;

            struct Impl
            {
                const Scene* scene;
                NodeID currentChild;

                void begin(const MyIter_t* tr) { *this = { &tr->Root->GetScene(), tr->Root->GetFirstChild() }; }
                void end(const MyIter_t* tr) { *this = { &tr->Root->GetScene(), entt::null }; }

                void next(const MyIter_t* tr) { currentChild = scene->get<NodeTransform>(currentChild).GetNextSibling(); }
                void prev(const MyIter_t* tr) { currentChild = scene->get<NodeTransform>(currentChild).GetPreviousSibling(); }

                const NodeID get(const MyIter_t* tr) const { return currentChild; }
                      NodeID get(      MyIter_t* tr)       { return currentChild; }
                      
                bool cmp(const Impl& other) const
                {
                    //Use memcmp if possible, for speed.
                    if constexpr (sizeof(Impl) == (sizeof(decltype(scene)) + sizeof(decltype(currentChild))))
                    {
                        return memcmp(this, &other, sizeof(Impl)) == 0;
                    }
                    else
                    {
                        return (scene == other.scene) &
                               (currentChild == other.currentChild);
                    }
                }
            };

            VGSI_STL_TYPEDEFS(NodeID);
            VGSI_SETUP_ITERATORS(MyIter_t, NodeID, Impl);
            VGSI_SETUP_REVERSE_ITERATORS(MyIter_t, NodeID, Impl);

            #pragma endregion
        };
        
        //Iterates all parents of a node, from its immediate parent up to the root.
        struct BP_API Parents
        {
            const NodeTransform* Start;

            #pragma region Implementation

            //Type-defs to make copy-pasting less dangerous.
            using MyIter_t = Bplus::ST::Iterators::Parents;

            struct Impl
            {
                const Scene* scene;
                NodeID currentParent;

                void begin(const MyIter_t* tr) { *this = { &tr->Start->GetScene(), tr->Start->GetParent() }; }
                void end(const MyIter_t* tr) { *this = { &tr->Start->GetScene(), entt::null }; }

                void next(const MyIter_t* tr) { currentParent = scene->get<NodeTransform>(currentParent).GetParent(); }

                const NodeID get(const MyIter_t* tr) const { return currentParent; }
                      NodeID get(      MyIter_t* tr)       { return currentParent; }
                      
                bool cmp(const Impl& other) const
                {
                    //Use memcmp if possible, for speed.
                    if constexpr (sizeof(Impl) == (sizeof(decltype(scene)) + sizeof(decltype(currentParent))))
                    {
                        return memcmp(this, &other, sizeof(Impl)) == 0;
                    }
                    else
                    {
                        return (scene == other.scene) &
                               (currentParent == other.currentParent);
                    }
                }
            };

            VGSI_STL_TYPEDEFS(NodeID);
            VGSI_SETUP_ITERATORS(MyIter_t, NodeID, Impl);

            #pragma endregion
        };


        //Iterates everything underneath a node, including children, grand-children, etc.
        //Iterates in depth-first order.
        struct BP_API TreeDFS
        {
            const NodeTransform* Root;
            bool IncludeSelf = true;

            #pragma region Implementation

            //Type-defs to make copy-pasting less dangerous.
            using MyIter_t = Bplus::ST::Iterators::TreeDFS;

            struct Impl
            {
                const Scene* scene;
                NodeID rootNodeID, currentNodeID;

                void begin(const MyIter_t* tr)
                {
                    scene = &tr->Root->GetScene();
                    rootNodeID = entt::to_entity(*scene, tr->Root);

                    if (tr->IncludeSelf)
                        currentNodeID = rootNodeID;
                    else
                        currentNodeID = tr->Root->GetFirstChild();
                }
                void end(const MyIter_t* tr)
                {
                    begin(tr); //Set up the first few fields

                    currentNodeID = entt::null;
                }

                //Returns the change in depth from this iteration.
                //E.x. if we moved to an "uncle" node, then -1 is returned.
                //If we moved to a "child" node, +1 is returned.
                int_fast32_t next(const MyIter_t* tr)
                {
                    auto* prevNode = &scene->get<NodeTransform>(currentNodeID);
                    auto prevNodeID = currentNodeID;

                    int_fast32_t depthDelta = 0;

                    //If the current node has a child, use that.
                    //Otherwise, try a sibling.

                    currentNodeID = prevNode->GetFirstChild();
                    depthDelta += 1;

                    //Make sure not to look at siblings/parents if we're already at the root node.
                    if ((currentNodeID == entt::null) & (prevNodeID != rootNodeID))
                    {
                        currentNodeID = prevNode->GetNextSibling();
                        depthDelta -= 1;

                        //If there's no next sibling, go up one level and try the "uncle".
                        //Keep moving up until we find one, or we hit the root node.
                        while ((currentNodeID == entt::null) & (prevNode->GetParent() != rootNodeID))
                        {
                            //Move up one level.
                            prevNodeID = prevNode->GetParent();
                            prevNode = &scene->get<NodeTransform>(prevNodeID);
                            depthDelta -= 1;

                            //Try to select the "uncle".
                            currentNodeID = prevNode->GetNextSibling();
                        }
                    }
                }

                const NodeID get(const MyIter_t* tr) const { return currentNodeID; }
                      NodeID get(      MyIter_t* tr)       { return currentNodeID; }

                bool cmp(const Impl& other) const
                {
                    //Use memcmp if possible, for speed.
                    if constexpr (sizeof(Impl) ==
                                  (sizeof(decltype(scene)) +
                                   sizeof(decltype(rootNodeID)) +
                                   sizeof(decltype(currentNodeID))))
                    {
                        return memcmp(this, &other, sizeof(Impl)) == 0;
                    }
                    else
                    {
                        return (scene == other.scene) &
                               (rootNodeID == other.rootNodeID) &
                               (currentNodeID == other.currentNodeID);
                    }
                }
            };

            VGSI_STL_TYPEDEFS(NodeID);
            VGSI_SETUP_ITERATORS(MyIter_t, NodeID, Impl);

            #pragma endregion
        };

        //Iterates everything underneath a node, including children, grand-children, etc.
        //Iterates in breadth-first order.
        struct BP_API TreeBFS
        {
            //TODO: Also provide a queue-based BFS, which may be faster for deep trees. Do some profiling once we have a realistic test case.

            const NodeTransform* Root;
            bool IncludeSelf = true;
            
            #pragma region Implementation

            //Type-defs to make copy-pasting less dangerous.
            using MyIter_t = Bplus::ST::Iterators::TreeBFS;

            struct Impl
            {
                //The implementation is based on Iterative-Deepening,
                //    which is a DFS-based search that ends up acting like BFS.
                //It visits nodes more often than necessary, but does not require heap memory.
                TreeDFS::Impl dfs;
                uint_fast32_t currentTargetDepth, currentDFSDepth;
                bool isDone;

                //As we iterate across a depth level, we keep track of
                //    whether any more children exist at the next depth.
                bool doesNextDepthExist;


                void begin(const MyIter_t* tr)
                {
                    TreeDFS dfsVersion{ tr->Root, tr->IncludeSelf };
                    dfs.begin(&dfsVersion);
                    
                    isDone = dfs.currentNodeID != entt::null;
                    if (isDone)
                        return;

                    currentTargetDepth = tr->IncludeSelf ? 0 : 1;
                    currentDFSDepth = currentTargetDepth;

                    //Assume the next depth is not there until we find an example.
                    doesNextDepthExist = false;
                }
                void end(const MyIter_t* tr)
                {
                    isDone = true;

                    //None of the other fields matter.
                }

                void next(const MyIter_t* tr)
                {
                    BPAssert(!isDone, "Trying to iterate past the end of a TreeBFS");

                    //Check out the node right here, and see if it has kids.
                    if (!doesNextDepthExist)
                        doesNextDepthExist = dfs.scene->get<NodeTransform>(dfs.currentNodeID).GetNChildren() > 0;

                    //Keep iterating the DFS until we find a node at the target depth.
                    //If the iterator finishes, restart it one level deeper.
                    do {
                        auto depthDelta = dfs.next(nullptr);
                        if (dfs.currentNodeID == entt::null)
                        {
                            //If we know there's nothing deeper, we're done the whole search.
                            if (!doesNextDepthExist)
                            {
                                isDone = true;
                                return;
                            }
                            else
                            {
                                TreeDFS dfsTree{ tr->Root, false }; //We know we can skip the root node at this point
                                dfs.begin(&dfsTree);

                                currentDFSDepth = 1;
                                doesNextDepthExist = false;
                                currentTargetDepth += 1;
                            }
                        }
                        else
                        {
                            //Update the current depth of the DFS.
                            BPAssert((depthDelta >= 0) | (((uint_fast32_t)(-depthDelta) > currentDFSDepth)),
                                     "An iteration of the DFS just took it above the root");
                            currentDFSDepth += depthDelta;
                        }
                    } while (currentDFSDepth != currentTargetDepth);
                }

                const NodeID get(const MyIter_t* tr) const { return dfs.get(nullptr); }
                      NodeID get(      MyIter_t* tr)       { return dfs.get(nullptr); }

                bool cmp(const Impl& other) const
                {
                    return (isDone & other.isDone) ||
                           ((!isDone) & (!other.isDone) &
                            dfs.cmp(other.dfs) &
                            (currentTargetDepth == other.currentDFSDepth));
                }
            };

            VGSI_STL_TYPEDEFS(NodeID);
            VGSI_SETUP_ITERATORS(MyIter_t, NodeID, Impl);

            #pragma endregion
        };
    }

    BP_API auto NodeTransform::IterChildren()       { return Iterators::Children{ this }; }
    BP_API auto NodeTransform::IterChildren() const { return Iterators::Children{ this }; }

    BP_API auto NodeTransform::IterParents()       { return Iterators::Parents{ this }; }
    BP_API auto NodeTransform::IterParents() const { return Iterators::Parents{ this }; }
    
    BP_API auto NodeTransform::IterTreeBreadth(bool includeSelf)       { return Iterators::TreeBFS{ this, includeSelf }; }
    BP_API auto NodeTransform::IterTreeBreadth(bool includeSelf) const { return Iterators::TreeBFS{ this, includeSelf }; }
    BP_API auto NodeTransform::IterTreeDepth(bool includeSelf)         { return Iterators::TreeDFS{ this, includeSelf }; }
    BP_API auto NodeTransform::IterTreeDepth(bool includeSelf)   const { return Iterators::TreeDFS{ this, includeSelf }; }

    #pragma endregion

    //TODO: An iterator for all nodes that are "direct children" in terms of a specific component type.


    //TODO: A utility to sort all or part of the NodeTransforms in a "world" by their parent ID, to make them more cache-friendly
}