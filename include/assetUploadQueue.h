#pragma once

#include <vector>

#include "mesh.h"

namespace Hydrogen
{
    struct MeshUploadRequest
    {
        MeshHandle handle{};
        MeshMetadata metadata{};
        Mesh mesh{};
    };

    class AssetUploadQueue
    {
    public:
        void Push(MeshUploadRequest request)
        {
            // TODO: add mutex when multithreading is introduced
            m_pending.push_back(std::move(request));
        }

        std::vector<MeshUploadRequest> Drain()
        {
            // TODO: add mutex when multithreading is introduced
            std::vector<MeshUploadRequest> result{};
            result.swap(m_pending);

            return result;
        }

    private:
        std::vector<MeshUploadRequest> m_pending{};
    };
}
