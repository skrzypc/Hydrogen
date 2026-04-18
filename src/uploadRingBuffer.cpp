#include "uploadRingBuffer.h"
#include "device.h"
#include "verifier.h"

namespace Hydrogen
{
	void UploadRingBuffer::Initialize(GpuDevice& device, uint64 sizePerFrame)
	{
		// Align sizePerFrame to 256 bytes so each frame's base address satisfies CBV alignment.
		m_sizePerFrame = (sizePerFrame + 255ull) & ~255ull;

		const uint64 totalSize = m_sizePerFrame * Config::FramesInFlight;

		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = totalSize;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc = { .Count = 1, .Quality = 0 };
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // required for buffers.
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		H2_VERIFY_FATAL(device.GetDxDevice()->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_pBuffer)),
			"Failed to create upload ring buffer!"
		);

		m_pBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedPtr));
	}

	UploadRingBuffer::Allocation UploadRingBuffer::Allocate(uint64 size, uint64 alignment)
	{
		const uint64 alignedOffset = (m_currentOffset + alignment - 1) & ~(alignment - 1);

		H2_VERIFY_FATAL(alignedOffset + size <= m_sizePerFrame, "Upload ring buffer exhausted!");

		Allocation allocation{};
		allocation.pCpuData = m_mappedPtr + m_frameBase + alignedOffset;
		allocation.gpuAddress = m_pBuffer->GetGPUVirtualAddress() + m_frameBase + alignedOffset;

		m_currentOffset = alignedOffset + size;

		return allocation;
	}

	void UploadRingBuffer::NextFrame(uint32 frameIndex)
	{
		m_frameBase = frameIndex * m_sizePerFrame;
		m_currentOffset = 0;
	}
}
