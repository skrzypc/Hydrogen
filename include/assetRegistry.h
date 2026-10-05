#pragma once

#include <vector>

#include "basicTypes.h"
#include "mesh.h"
#include "material.h"
#include "textureAsset.h"
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

        void UpdateMaterial(MaterialHandle handle, Material&& material)
        {
            if (handle.id >= m_materials.size())
            {
                return;
            }

            m_materials[handle.id] = std::move(material);
            m_uploadQueue.PushMaterial({handle, m_materials[handle.id]});
        }

        const Material* GetMaterial(MaterialHandle handle) const
        {
            if (handle.id >= m_materials.size())
            {
                return nullptr;
            }
            return &m_materials[handle.id];
        }

        TextureHandle RegisterTexture(TextureMetadata&& metadata, TextureData&& data)
        {
            for (uint32 i = 0; i < m_textureMetadata.size(); ++i)
            {
                if (m_textureMetadata[i].path == metadata.path)
                {
                    return TextureHandle{i};
                }
            }

            TextureHandle handle{static_cast<uint32>(m_textureMetadata.size())};

            m_textureMetadata.push_back(std::move(metadata));
            m_uploadQueue.PushTexture({handle, m_textureMetadata.back(), std::move(data)});

            return handle;
        }

        const TextureMetadata* GetTextureMetadata(TextureHandle handle) const
        {
            if (handle.id >= m_textureMetadata.size())
            {
                return nullptr;
            }
            return &m_textureMetadata[handle.id];
        }

        AssetUploadQueue& GetUploadQueue()
        {
            return m_uploadQueue;
        }

    private:
        std::vector<MeshMetadata> m_meshMetadata{};
        std::vector<Material> m_materials{};
        std::vector<TextureMetadata> m_textureMetadata{};

        AssetUploadQueue m_uploadQueue{};
    };
} // namespace Hydrogen
