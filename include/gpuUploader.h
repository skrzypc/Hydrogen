#pragma once

#include <memory>
#include <optional>

#include "basicTypes.h"
#include "buffer.h"
#include "texture.h"
#include "copyContext.h"

namespace Hydrogen
{
	class GpuDevice;

	class GpuUploader
	{
	public:
		void Initialize(GpuDevice& device, uint64 stagingCapacity);

		// Copies [pData, pData+byteSize) into pDstBuffer at dstOffset.
		void Upload(const void* pData, uint64 byteSize, Buffer* pDstBuffer, uint64 dstOffset = 0);

		// Copies pData into pDstTexture subresource. Source is assumed tightly packed (no row padding).
		void Upload(const void* pData, Texture* pDstTexture, uint32 subresource = 0);

		uint64 Flush();

	private:
		void EnsureActiveContext();

		GpuDevice* m_pDevice = nullptr;
		std::optional<CopyContext> m_activeContext;

		std::unique_ptr<Buffer> m_stagingBuffer;
		uint8* m_mappedPtr = nullptr;
		uint64 m_capacity = 0;
		uint64 m_currentOffset = 0;
	};
}
