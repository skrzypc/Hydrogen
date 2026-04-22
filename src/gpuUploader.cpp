#include "gpuUploader.h"

#include <cstring>

#include "device.h"
#include "verifier.h"

namespace Hydrogen
{
	void GpuUploader::Initialize(GpuDevice& device, uint64 stagingCapacity)
	{
		m_pDevice = &device;
		m_capacity = stagingCapacity;

		ResourceState initialState{};
		m_stagingBuffer = m_pDevice->CreateBuffer(L"H2_GPU_UPLOADER_STAGING_BUFFER",
			Buffer::Desc{ .size = stagingCapacity, .heapType = D3D12_HEAP_TYPE_UPLOAD },
			initialState
		);

		m_stagingBuffer->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedPtr));
	}

	void GpuUploader::EnsureActiveContext()
	{
		if (!m_activeContext.has_value())
		{
			m_activeContext = m_pDevice->AcquireCopyContext();
		}
	}

	void GpuUploader::Upload(const void* pData, uint64 byteSize, Buffer* pDstBuffer, uint64 dstOffset)
	{
		H2_VERIFY_FATAL(m_currentOffset + byteSize <= m_capacity, "GpuUploader staging buffer exhausted!");

		EnsureActiveContext();

		memcpy(m_mappedPtr + m_currentOffset, pData, byteSize);

		m_activeContext->CmdList()->CopyBufferRegion(
			pDstBuffer->GetResource(),
			dstOffset,
			m_stagingBuffer->GetResource(),
			m_currentOffset,
			byteSize
		);

		m_currentOffset += byteSize;
	}

	void GpuUploader::Upload(const void* pData, Texture* pDstTexture, uint32 subresource)
	{
		EnsureActiveContext();

		// Align staging offset to texture placement alignment.
		const uint64 alignedOffset = (m_currentOffset + D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - 1)
			& ~uint64(D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - 1);

		D3D12_RESOURCE_DESC resourceDesc = pDstTexture->GetResource()->GetDesc();
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
		UINT numRows = 0;
		UINT64 rowSizeInBytes = 0;
		UINT64 totalBytes = 0;

		m_pDevice->GetDxDevice()->GetCopyableFootprints(
			&resourceDesc, subresource, 1, alignedOffset,
			&footprint, &numRows, &rowSizeInBytes, &totalBytes
		);

		H2_VERIFY_FATAL(totalBytes <= m_capacity, "GpuUploader staging buffer exhausted for texture upload!");

		// Copy source rows into staging, applying the required row pitch alignment.
		const uint8* pSrc = static_cast<const uint8*>(pData);
		uint8* pDst = m_mappedPtr + footprint.Offset;
		for (UINT row = 0; row < numRows; ++row)
		{
			memcpy(pDst + row * footprint.Footprint.RowPitch, pSrc + row * rowSizeInBytes, rowSizeInBytes);
		}

		m_currentOffset = totalBytes;

		D3D12_TEXTURE_COPY_LOCATION src{};
		src.pResource = m_stagingBuffer->GetResource();
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src.PlacedFootprint = footprint;

		D3D12_TEXTURE_COPY_LOCATION dst{};
		dst.pResource = pDstTexture->GetResource();
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst.SubresourceIndex = subresource;

		m_activeContext->CmdList()->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
	}

	uint64 GpuUploader::Flush()
	{
		if (!m_activeContext.has_value())
		{
			return 0;
		}

		uint64 fenceValue = m_pDevice->ExecuteCopyContext(std::move(*m_activeContext));
		m_activeContext.reset();
		m_currentOffset = 0;
		return fenceValue;
	}
}
