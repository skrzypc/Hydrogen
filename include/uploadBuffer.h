#pragma once

#include <string_view>

#include "basicTypes.h"
#include "buffer.h"

namespace Hydrogen
{
    class GpuDevice;

    class UploadBuffer : public Buffer
    {
        friend class GpuDevice;

    public:
        UploadBuffer() = default;
        ~UploadBuffer() override;

        UploadBuffer(const UploadBuffer&) = delete;
        UploadBuffer& operator=(const UploadBuffer&) = delete;
        UploadBuffer(UploadBuffer&&) noexcept = default;
        UploadBuffer& operator=(UploadBuffer&&) noexcept = default;

        void Write(const void* pData, uint64 sizeInBytes, uint64 offsetInBytes = 0);

        uint8* GetMappedPtr() const { return m_pMapped; }
        D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress(uint64 offsetInBytes = 0) const
        {
            return GetResource()->GetGPUVirtualAddress() + offsetInBytes;
        }

    private:
        uint8* m_pMapped = nullptr;
    };
}
