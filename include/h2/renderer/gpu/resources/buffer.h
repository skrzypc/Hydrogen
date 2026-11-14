#pragma once

#include <h2/renderer/gpu/resources/resource.h>
#include <h2/core/basicTypes.h>
#include <h2/core/verifier.h>

namespace Hydrogen
{
    class Buffer : public Resource
    {
    public:
        Buffer() = default;
        virtual ~Buffer() = default;
        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;
        Buffer(Buffer&&) noexcept = default;
        Buffer& operator=(Buffer&&) noexcept = default;

        struct Desc
        {
            uint32 elementCount = 0;
            uint32 stride = 0;
            D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
            D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;

            bool operator==(const Buffer::Desc&) const = default;
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

        const Desc& GetDesc() const
        {
            return m_desc;
        }
        void SetDesc(const Desc& desc)
        {
            m_desc = desc;
        }

        uint32 GetSize() const
        {
            uint64 size = static_cast<uint64>(m_desc.elementCount) * m_desc.stride;

            H2_VERIFY_FATAL(std::in_range<uint32>(size), "Buffer size {} exceeds 4 GB!", size);

            return static_cast<uint32>(size);
        }

    private:
        Desc m_desc{};
    };

    struct BufferDescHash
    {
        uint64 operator()(const Buffer::Desc& k) const
        {
            uint64 seed = 0;
            auto combine = [&](auto v)
            { seed ^= std::hash<decltype(v)>{}(v) + 0x9e3779b9u + (seed << 6) + (seed >> 2); };
            combine(k.elementCount);
            combine(k.stride);
            combine(static_cast<uint32>(k.flags));
            combine(static_cast<uint32>(k.heapType));

            return seed;
        }
    };
} // namespace Hydrogen
