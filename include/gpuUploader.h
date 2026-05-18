#pragma once

#include <memory>
#include <optional>
#include <queue>

#include "basicTypes.h"
#include "uploadBuffer.h"
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
		struct Segment {
			uint64 start;
			uint64 end;          // exclusive, 4KB-aligned
			uint64 fenceValue;
		};

		// Returns the staging offset to write at (4KB-aligned). Retires completed
		// segments, wraps if needed, waits for the oldest segment only if unavoidable.
		uint64 Allocate(uint64 byteSize);

		void RetireCompletedSegments();
		void EnsureActiveContext();

		GpuDevice* m_pDevice = nullptr;
		std::optional<CopyContext> m_activeContext;

		std::unique_ptr<UploadBuffer> m_stagingBuffer;
		uint64 m_capacity = 0;
		uint64 m_writeOffset = 0;
		uint64 m_batchStart = 0;    // start of the current unflushed batch

		std::queue<Segment> m_pendingSegments;

		bool m_bRequiresFlush = false;
	};
}
