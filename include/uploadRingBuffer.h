#pragma once

#include <d3d12.h>
#include <memory>

#include "basicTypes.h"
#include "config.h"
#include "uploadBuffer.h"

namespace Hydrogen
{
	class GpuDevice;

	class UploadRingBuffer
	{
	public:
		struct Allocation
		{
			void* pCpuData = nullptr;
			D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = 0;
		};

		void Initialize(GpuDevice& device, uint64 sizePerFrame);
		Allocation Allocate(uint64 size, uint64 alignment = 256);
		void NextFrame(uint32 frameIndex);

	private:
		std::unique_ptr<UploadBuffer> m_buffer{};
		uint64 m_sizePerFrame = 0;
		uint64 m_frameBase = 0;
		uint64 m_currentOffset = 0;
	};
}
