#include <cstring>

#include "uploadBuffer.h"
#include "device.h"
#include "verifier.h"

namespace Hydrogen
{
    UploadBuffer::UploadBuffer(GpuDevice& device, uint64 sizeInBytes, std::wstring_view name)
    {
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width              = sizeInBytes;
        resourceDesc.Height             = 1;
        resourceDesc.DepthOrArraySize   = 1;
        resourceDesc.MipLevels          = 1;
        resourceDesc.Format             = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc         = { .Count = 1, .Quality = 0 };
        resourceDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags              = D3D12_RESOURCE_FLAG_NONE;

        Buffer::Desc desc{};
        desc.size     = sizeInBytes;
        desc.heapType = D3D12_HEAP_TYPE_UPLOAD;
        SetDesc(desc);

        H2_VERIFY_FATAL(
            device.GetDxDevice()->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(GetResourceAddress())),
            "Failed to create UploadBuffer!"
        );

        SetName(name);

        H2_VERIFY_FATAL(
            GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&m_pMapped)),
            "Failed to map UploadBuffer!"
        );
    }

    UploadBuffer::~UploadBuffer()
    {
        if (m_pMapped && GetResource())
        {
            GetResource()->Unmap(0, nullptr);
            m_pMapped = nullptr;
        }
    }

    void UploadBuffer::Write(const void* pData, uint64 sizeInBytes, uint64 offsetInBytes)
    {
        std::memcpy(m_pMapped + offsetInBytes, pData, sizeInBytes);
    }
}
