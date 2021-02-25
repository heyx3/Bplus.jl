#include "Scene.h"

#include "NodeTransform.h"


using namespace Bplus;
using namespace Bplus::ST;

Scene::Scene()
{
    on_destroy<NodeTransform>().connect<entt::invoke<&NodeTransform::_DisconnectParent, NodeID>>();
}