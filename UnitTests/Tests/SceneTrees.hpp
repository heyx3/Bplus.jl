#pragma once

#include <B+/ST/NodeTransform.h>

#define TEST_NO_MAIN
#include <acutest.h>

using namespace Bplus;
using namespace Bplus::ST;


struct NodeTag { std::string Name; };


void ST_BasicManipulation()
{
    TEST_CASE("Creating entities");
    Scene scene;
    NodeID e1 = scene.create(),
           e2 = scene.create(),
           e3 = scene.create(),
           e4 = scene.create();

    TEST_CASE("Creating NodeTransform components");
    NodeTransform &n1 = scene.emplace<NodeTransform>(e1),
                  &n2 = scene.emplace<NodeTransform>(e2),
                  &n3 = scene.emplace<NodeTransform>(e3),
                  &n4 = scene.emplace<NodeTransform>(e4);

    TEST_CASE("Arranging hierarchy");
    /*
       Desired structure:
             e1
           /    \
          e2    e4
          |
          e3
    */
    n4.SetParent(e1);
    n3.SetParent(e2);
    n2.SetParent(e1);
    TEST_CHECK_(n1.GetParent() == entt::null, "Node 1 isn't the root");
    TEST_CHECK_(scene.has<NodeRoot>(e1), "Node 1 isn't marked with NodeRoot");
    TEST_CHECK_(n2.GetParent() == e1, "Node 2 isn't a child of Node 1");
    TEST_CHECK_(!scene.has<NodeRoot>(e2), "Node 2 is erroneously tagged with NodeRoot");
    TEST_CHECK_(n3.GetParent() == e2, "Node 3 isn't a child of Node 2");
    TEST_CHECK_(!scene.has<NodeRoot>(e3), "Node 3 is erroneously tagged with NodeRoot");
    TEST_CHECK_(n4.GetParent() == e1, "Node 4 isn't a child of Node 1");
    TEST_CHECK_(!scene.has<NodeRoot>(e4), "Node 4 is erroneously tagged with NodeRoot");


    TEST_CASE("Messing with positions from top to bottom");

    for (NodeID entity : scene.view())
    {
        auto& node = scene.get<NodeTransform>(entity);
        TEST_CHECK_(node.GetLocalPos() == glm::zero<glm::fvec3>(),
                    "Node %i isn't at local origin anymore", (int)entity);
        TEST_CHECK_(node.GetLocalScale() == glm::one<glm::fvec3>(),
                    "Node %i has non-unit scale somehow", (int)entity);
        TEST_CHECK_(node.GetLocalRot() == Bplus::Math::RotIdentity<float>(),
                    "Node %i has non-zero local rotation somehow", (int)entity);
    }

    n1.SetLocalPos({ 1.5f, 9.45f, -200.0f });
    TEST_CHECK_(glm::all(glm::epsilonEqual(n1.GetLocalPos(), { 1.5f, 9.45f, -200.0f }, 0.001f)),
                "Setting node 1's local position didn't work; it's now at { %f, %f, %f }",
                n1.GetLocalPos().x, n1.GetLocalPos().y, n1.GetLocalPos().z);

    n2.SetLocalPos({ 6.7f, 4.5f, -100.0f });
    TEST_CHECK_(glm::all(glm::epsilonEqual(n2.GetLocalPos(), { 6.7f, 4.5f, -100.0f }, 0.001f)),
                "Setting node 2's local position didn't work; it's now at { %f, %f, %f }",
                n2.GetLocalPos().x, n2.GetLocalPos().y, n2.GetLocalPos().z);
    TEST_CHECK_(glm::all(glm::epsilonEqual(n2.GetWorldPos(), (n1.GetLocalPos() + n2.GetLocalPos()), 0.001f)),
                "Node 2's world position is wrong; it should be { %f, %f, %f } but it's { %f, %f, %f }",
                (n1.GetLocalPos().x + n2.GetLocalPos().x),
                (n1.GetLocalPos().y, + n2.GetLocalPos().y),
                (n1.GetLocalPos().z + n2.GetLocalPos().z),
                n2.GetWorldPos().x, n2.GetWorldPos().y, n2.GetWorldPos().z);

    TEST_CHECK_(glm::all(glm::epsilonEqual(n3.GetWorldPos(), n2.GetWorldPos(), 0.001f)),
                "Node 3 has local position 0 so it should have the same world position as its parent, Node 2. \
Instead, N2 is at { %f, %f, %f } while N3 is at { %f, %f, %f }.",
                n2.GetWorldPos().x, n2.GetWorldPos().y, n2.GetWorldPos().z,
                n3.GetWorldPos().x, n3.GetWorldPos().y, n3.GetWorldPos().z);

    n4.SetWorldPos({ 5, 10, 20 });
    TEST_CHECK_(glm::all(glm::epsilonEqual(n4.GetWorldPos(), { 5, 10, 20 }, 0.001f)),
                "Node 4 world position should be at { 5, 10, 20 } but it's at { %f, %f, %f }",
                n4.GetWorldPos().x, n4.GetWorldPos().y, n4.GetWorldPos().z);
    TEST_CHECK_(glm::all(glm::epsilonEqual(n4.GetLocalPos(), glm::fvec3{ 5, 10, 20 } - n1.GetWorldPos(), 0.001f)),
                "Node 4 local position should be at { %f, %f, %f } but it's at { %f, %f, %f }",
                5 - n1.GetWorldPos().x, 10 - n1.GetWorldPos().y, 20 - n1.GetWorldPos().z);
}