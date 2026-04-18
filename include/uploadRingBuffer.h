#pragma once

#include <d3d12.h>
#include <wrl.h>

#include "basicTypes.h"
#include "config.h"

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
		Microsoft::WRL::ComPtr<ID3D12Resource> m_pBuffer = nullptr;
		uint8* m_mappedPtr = nullptr;
		uint64 m_sizePerFrame = 0;
		uint64 m_frameBase = 0;
		uint64 m_currentOffset = 0;
	};
}
