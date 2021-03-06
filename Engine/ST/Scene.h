#pragma once

#include <optional>

#include "../Dependencies.h"
#include "../Utils.h"


namespace Bplus::ST
{
    //The Scene Tree is implemented on top of an Entity-Component System, or ECS.
    //The ECS library used is EnTT.

    //An entity identifier that is just like the default EnTT identifier type,
    //     except that it default-initializes to null.
    struct NodeID
    {
        using Base_t = entt::entity;
        using Under_t = entt::entt_traits<Base_t>::entity_type;

        Base_t Value;

        NodeID() : NodeID(entt::null) { }
        NodeID(entt::null_t null) : Value(null) { }
        NodeID(Base_t value) : Value(value) { }
        NodeID(Under_t value) : Value((Base_t)value) { }

        operator Base_t() const { return Value; }
        operator Under_t() const { return (Under_t)Value; }

        bool operator==(NodeID other) const { return Value == other.Value; }
        bool operator!=(NodeID other) const { return !operator==(other); }
    };

    //Add type info for our custom EnTT entity type.
} namespace entt {
        template<>
        struct entt_traits<Bplus::ST::NodeID> :
            entt_traits<decltype(Bplus::ST::NodeID().Value)>
        { };
} namespace Bplus::ST {


    //TODO: It's apparently possible to guarantee the callbacks are hooked up without using a custom registry class, by defining a custom pool for the component. See the below comment from the EnTT dev:
    /*
        You can also create a custom pool on a per-type basis.
        Internally, there is a virtual method that is invoked upon destruction, before the entity gets removed.
        If I get it, you want to do something within the same pool, so you don't even need the registry for that.
        This way, things work implicitly out of the box and you don't have to wire any listener or so.
        Out of my mind, it should be something like this:
        template<typename Base>
        struct my_pool: Base {
        protected:
            void swap_and_pop(const std::size_t pos, void *) override {
                const auto entity = this->data()[pos]; // or this->at(pos)
                // do whatever you want with your entity here
            }
        };

        template<typename Entity>
        struct storage_traits<Entity, MyVerySpecificType> {
            using storage_type = sigh_storage_mixin<my_pool<basic_storage<Entity, Type>>>;
        };
        Ofc you can refine it to make it more type-aware, since you want it to be constrained to a specific type
        Or you can replace the basic_storage as a whole, but it doesn't really make sense since you would have to rewrite all its functionalities, so better to construct on top of it
        And I'd suggest to remove the sigh_storage_mixin part if you don't need that pool to expose signals for other uses, but it's up to you and very application specific :man_shrugging:
    */

    //An EnTT Entity-Component System, with particular callbacks set up for Scene Trees.
    //If you try to work with a normal entt::registry,
    //    then things will break when nodes get destroyed.
    struct BP_API Scene : public virtual entt::basic_registry<NodeID>
    {
        Scene();
        virtual ~Scene() = default;
    };
}
