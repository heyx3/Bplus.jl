#pragma once

#include <optional>

#include "../RenderLibs.h"
#include "../Utils.h"


namespace Bplus::ST
{
    //The Scene Tree is implemented on top of an Entity-Component System, or ECS.
    //The ECS library used is EnTT.

    using NodeID = entt::id_type;


    //An EnTT Entity-Component System, with particular callbacks set up for Scene Trees.
    //If you try to work with a normal entt::registry,
    //    then things will break when nodes get destroyed.
    struct BP_API Scene : public virtual entt::basic_registry<NodeID>
    {
        Scene();
        virtual ~Scene() = default;
    };
}