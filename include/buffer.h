#pragma once

#include "gpuResource.h"

namespace Hydrogen
{
    class Buffer : public GpuResource
    {
    public:
        struct Desc
        {
            uint64 size = 0u;
            D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
            D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
        };

        enum class Usage : uint8
        {
            None = 0,
            VertexBuffer = 1 << 0,
            IndexBuffer = 1 << 1,
            ConstantBuffer = 1 << 2,
            Structured = 1 << 3,
            UnorderedAccess = 1 << 4,
            Indirect = 1 << 5,
        };

        void SetDesc(const Desc& desc) { m_desc = desc; }

        uint64 GetSize() const { return m_desc.size; }

    private:
        Desc m_desc{};
        Usage m_usage{};
    };
}