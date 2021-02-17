#pragma once

#include <optional>

#include <llvm/SmallVector.hpp>

#include "../RenderLibs.h"
#include "../Utils.h"


namespace Bplus::ST
{
    //The Scene Tree is implemented on top of an Entity-Component System, or ECS.
    //The ECS library used is EnTT.

    using NodeID = uint_fast32_t;
    using ECS = entt::basic_registry<NodeID>;

    //A vector-like collection that can avoid heap allocations for the first few elements.
    template<typename T, unsigned N = llvm::CalculateSmallVectorDefaultInlinedElements<T>::value>
    using ShortVector = llvm::SmallVector<T>;
}