#include "gpuUploader.h"

#include <cstring>

#include "device.h"
#include "verifier.h"

namespace Hydrogen
{
	static constexpr uint64 kBlockSize = 4096;

	static uint64 AlignUp(uint64 value, uint64 alignment)
	{
		return (value + alignment - 1) & ~(alignment - 1);
	}

	void GpuUploader::Initialize(GpuDevice& device, uint64 stagingCapacity)
	{
		m_pDevice = &device;
		m_capacity = AlignUp(stagingCapacity, kBlockSize);
		m_stagingBuffer = m_pDevice->CreateUploadBuffer(L"H2_GPU_UPLOADER_STAGING_BUFFER", m_capacity);
	}

	void GpuUploader::RetireCompletedSegments()
	{
		const uint64 completed = m_pDevice->GetCompletedFenceValue<eQueueType::Copy>();
		while (!m_pendingSegments.empty() && m_pendingSegments.front().fenceValue <= completed)
		{
			m_pendingSegments.pop();
		}
	}

	uint64 GpuUploader::Allocate(uint64 byteSize)
	{
		const uint64 alignedSize = AlignUp(byteSize, kBlockSize);

		for (int attempt = 0; attempt < 2; ++attempt)
		{
			RetireCompletedSegments();

			const uint64 spaceUntilEnd = m_capacity - m_writeOffset;

			if (alignedSize <= spaceUntilEnd)
			{
				const bool blocked = !m_pendingSegments.empty()
					&& m_pendingSegments.front().start >= m_writeOffset
					&& m_pendingSegments.front().start < m_writeOffset + alignedSize;

				if (!blocked)
				{
					const uint64 offset = m_writeOffset;
					m_writeOffset += alignedSize;
					return offset;
				}

				// Oldest segment is in the way — wait for it and return.
				m_pDevice->Wait<eQueueType::Copy>(m_pendingSegments.front().fenceValue);
				RetireCompletedSegments();
				const uint64 offset = m_writeOffset;
				m_writeOffset += alignedSize;
				return offset;
			}

			// Doesn't fit before the end — waste the tail and wrap.
			if (attempt == 0)
			{
				m_writeOffset = 0;
				m_batchStart = 0;
			}
		}

		H2_VERIFY_FATAL(false, "GpuUploader staging buffer exhausted — allocation too large for ring!");
		return 0;
	}

	void GpuUploader::EnsureActiveContext()
	{
		if (!m_activeContext.has_value())
		{
			m_activeContext = m_pDevice->AcquireCopyContext();
			m_batchStart = m_writeOffset;
		}
	}

	void GpuUploader::Upload(const void* pData, uint64 byteSize, Buffer* pDstBuffer, uint64 dstOffset)
	{
		EnsureActiveContext();

		const uint64 stagingOffset = Allocate(byteSize);

		memcpy(m_stagingBuffer->GetMappedPtr() + stagingOffset, pData, byteSize);

		m_activeContext->CmdList()->CopyBufferRegion(
			pDstBuffer->GetResource(),
			dstOffset,
			m_stagingBuffer->GetResource(),
			stagingOffset,
			byteSize
		);

		m_bRequiresFlush = true;
	}

	void GpuUploader::Upload(const void* pData, Texture* pDstTexture, uint32 subresource)
	{
		EnsureActiveContext();

		const uint64 alignedOffset = AlignUp(m_writeOffset, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

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

		const uint8* pSrc = static_cast<const uint8*>(pData);
		uint8* pDst = m_stagingBuffer->GetMappedPtr() + footprint.Offset;
		for (UINT row = 0; row < numRows; ++row)
		{
			memcpy(pDst + row * footprint.Footprint.RowPitch, pSrc + row * rowSizeInBytes, rowSizeInBytes);
		}

		m_writeOffset = AlignUp(footprint.Offset + totalBytes, kBlockSize);

		D3D12_TEXTURE_COPY_LOCATION src{};
		src.pResource = m_stagingBuffer->GetResource();
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src.PlacedFootprint = footprint;

		D3D12_TEXTURE_COPY_LOCATION dst{};
		dst.pResource = pDstTexture->GetResource();
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst.SubresourceIndex = subresource;

		m_activeContext->CmdList()->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

		m_bRequiresFlush = true;
	}

	uint64 GpuUploader::Flush()
	{
		if (!m_bRequiresFlush)
		{
			return m_pendingSegments.empty() ? 0 : m_pendingSegments.back().fenceValue;
		}

		const uint64 fenceValue = m_pDevice->ExecuteCopyContext(std::move(*m_activeContext));
		m_activeContext.reset();
		m_bRequiresFlush = false;
		m_pendingSegments.push({ m_batchStart, m_writeOffset, fenceValue });
		return fenceValue;
	}
}
