#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "mesh.h"
#include "transform.h"

namespace Hydrogen
{
    struct ModelNode
    {
        std::string name{};
        Transform localTransform{};
        uint32 parentIndex = std::numeric_limits<uint32>::max(); // root node
        std::optional<uint32> meshIndex{}; // index into Model::meshes
    };

    struct Model
    {
        std::string name{};
        std::vector<Mesh> meshes{};
        std::vector<ModelNode> nodes{};
    };

    class ModelLoader
    {
    public:
        static Model Load(std::string_view path);
    };
}
