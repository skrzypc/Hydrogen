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

		m_buffer = std::make_unique<UploadBuffer>(device, totalSize, L"UploadRingBuffer");
	}

	UploadRingBuffer::Allocation UploadRingBuffer::Allocate(uint64 size, uint64 alignment)
	{
		const uint64 alignedOffset = (m_currentOffset + alignment - 1) & ~(alignment - 1);

		H2_VERIFY_FATAL(alignedOffset + size <= m_sizePerFrame, "Upload ring buffer exhausted!");

		Allocation allocation{};
		allocation.pCpuData   = m_buffer->GetMappedPtr() + m_frameBase + alignedOffset;
		allocation.gpuAddress = m_buffer->GetGpuAddress(m_frameBase + alignedOffset);

		m_currentOffset = alignedOffset + size;

		return allocation;
	}

	void UploadRingBuffer::NextFrame(uint32 frameIndex)
	{
		m_frameBase = frameIndex * m_sizePerFrame;
		m_currentOffset = 0;
	}
}
