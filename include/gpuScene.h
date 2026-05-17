#pragma once

#include <memory>
#include <optional>
#include <span>
#include <vector>

#include "gpuMesh.h"
#include "mesh.h"
#include "buffer.h"
#include "uploadBuffer.h"
#include "device.h"
#include "shaderInterop.h"

namespace Hydrogen
{
	class GpuDevice;
	class GpuUploader;
	struct Mesh;

	class GpuScene
	{
	public:
		void Initialize(GpuDevice& device, GpuUploader& uploader, uint32 maxVertices, uint32 maxIndices, uint32 maxObjects = 100'000, uint32 maxViews = 16);

		// Uploads the mesh to the GPU, registers it in the cache, and flushes the copy queue.
		uint64 UploadMesh(MeshHandle handle, const Mesh& mesh);

		const GpuMesh* GetGpuMesh(MeshHandle handle) const;
		const std::vector<GpuMesh>& GetMeshes() const { return m_meshes; }

		const Buffer* GetPositionBuffer() const { return m_positionBuffer.get(); }
		const Buffer* GetNormalBuffer() const { return m_normalBuffer.get(); }
		const Buffer* GetUvBuffer() const { return m_uvBuffer.get(); }
		const Buffer* GetIndexBuffer() const { return m_indexBuffer.get(); }

		ShaderResourceViewHandle GetPositionSrv() const { return m_positionSrv; }
		ShaderResourceViewHandle GetNormalSrv() const { return m_normalSrv; }
		ShaderResourceViewHandle GetUvSrv() const { return m_uvSrv; }
		ShaderResourceViewHandle GetIndexSrv() const { return m_indexSrv; }

		ShaderResourceViewHandle GetViewBufferSrv() const { return m_viewBufferSrv; }
		void UpdateView(const ShaderInterop::ViewData& viewData, uint32 viewIndex = 0);

		ShaderResourceViewHandle GetTransformBufferSrv() const { return m_transformBufferSrv; }
		void UpdateTransforms(std::span<const DirectX::XMFLOAT4X4> matrices);

	private:
		GpuDevice* m_pDevice = nullptr;
		GpuUploader* m_pUploader = nullptr;

		uint32 m_maxVertices = 0;
		uint32 m_maxIndices = 0;
		uint32 m_nextVertex = 0;
		uint32 m_nextIndex = 0;

		std::vector<GpuMesh> m_meshes;

		std::unique_ptr<Buffer> m_positionBuffer;
		std::unique_ptr<Buffer> m_normalBuffer;
		std::unique_ptr<Buffer> m_uvBuffer;
		std::unique_ptr<Buffer> m_indexBuffer;

		ShaderResourceViewHandle m_positionSrv{};
		ShaderResourceViewHandle m_normalSrv{};
		ShaderResourceViewHandle m_uvSrv{};
		ShaderResourceViewHandle m_indexSrv{};

		std::unique_ptr<UploadBuffer> m_viewBuffer;
		ShaderResourceViewHandle m_viewBufferSrv{};

		std::unique_ptr<UploadBuffer> m_transformBuffer;
		ShaderResourceViewHandle m_transformBufferSrv{};

		std::vector<GpuMesh> m_gpuMeshCache{};
	};
}
