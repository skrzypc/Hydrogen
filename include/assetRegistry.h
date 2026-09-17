#pragma once

#include <vector>

#include "basicTypes.h"
#include "mesh.h"
#include "material.h"
#include "assetUploadQueue.h"

namespace Hydrogen
{
    class AssetRegistry
    {
    public:
        AssetRegistry()
        {
            RegisterMaterial(Material{.name = "Default"});
        }
        ~AssetRegistry() = default;
        AssetRegistry(const AssetRegistry&) = delete;
        AssetRegistry& operator=(const AssetRegistry&) = delete;
        AssetRegistry(AssetRegistry&&) noexcept = default;
        AssetRegistry& operator=(AssetRegistry&&) noexcept = default;

        MeshHandle RegisterMesh(MeshMetadata&& metadata, Mesh&& mesh)
        {
            MeshHandle handle{static_cast<uint32>(m_meshMetadata.size())};

            m_meshMetadata.push_back(std::move(metadata));
            m_uploadQueue.Push({handle, m_meshMetadata.back(), std::move(mesh)});

            return handle;
        }

        const MeshMetadata* GetMeshMetadata(MeshHandle handle) const
        {
            if (handle.id >= m_meshMetadata.size())
            {
                return nullptr;
            }
            return &m_meshMetadata[handle.id];
        }

        MaterialHandle RegisterMaterial(Material&& material)
        {
            MaterialHandle handle{static_cast<uint32>(m_materials.size())};

            m_materials.push_back(std::move(material));
            m_uploadQueue.PushMaterial({handle, m_materials.back()});

            return handle;
        }

        AssetUploadQueue& GetUploadQueue()
        {
            return m_uploadQueue;
        }

    private:
        std::vector<MeshMetadata> m_meshMetadata{};
        std::vector<Material> m_materials{};

        AssetUploadQueue m_uploadQueue{};
    };
} // namespace Hydrogen
