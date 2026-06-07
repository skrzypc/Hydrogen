#pragma once

#include <array>
#include <memory>
#include <optional>
#include <queue>
#include <span>
#include <vector>

#include "config.h"
#include "gpuMesh.h"
#include "mesh.h"
#include "assetUploadQueue.h"
#include "buffer.h"
#include "uploadBuffer.h"
#include "device.h"
#include "shaderInterop.h"
#include "renderScene.h"

namespace Hydrogen
{
	class GpuDevice;
	class GpuUploader;
	struct Mesh;

	struct SceneBindings
	{
		uint32 positionBufferIndex = 0;
		uint32 normalBufferIndex = 0;
		uint32 uvBufferIndex = 0;
		uint32 indexBufferIndex = 0;
		uint32 transformBufferIndex = 0;
		uint32 tlasIndex = 0;
	};

	class GpuScene
	{
	public:
		void Initialize(GpuDevice& device, GpuUploader& uploader, uint32 maxVertices, uint32 maxIndices, uint32 maxObjects = 100'000, uint32 maxViews = 16);

		// Enqueues a mesh for upload. Actual GPU upload happens during Update(), up to m_maxMeshUploadsPerFrame per frame.
		void RegisterMesh(MeshHandle handle, const Mesh& mesh);
		void RegisterMeshes(std::vector<MeshHandle>& meshHandles, std::vector<Mesh>& meshes);

		SceneBindings Update(const RenderScene& renderScene, uint32 frameIndex);

		const Buffer* GetIndexBuffer() const { return m_indexBuffer.get(); }
		const GpuMesh* GetGpuMesh(MeshHandle handle) const;

	private:
		struct PendingMeshUpload { uint32 meshIndex; uint32 handleId; uint64 copyFence; };
		struct PendingBLAS { uint32 meshIndex; uint32 handleId; uint32 blasBufferIndex; uint64 directFence; };
		struct QueuedMesh { MeshHandle handle; Mesh mesh; };

		void DrainUploadQueue();
		void StageMesh(MeshHandle handle, const Mesh& mesh);
		void PromoteCompletedUploads();
		void PromoteCompletedBLAS();
		void BuildPendingBLAS(std::span<const PendingMeshUpload> uploads);
		void UpdateTransforms(std::span<const RenderObject> objects);

		GpuDevice* m_pDevice = nullptr;
		GpuUploader* m_pUploader = nullptr;

		uint32 m_maxVertices = 0;
		uint32 m_maxIndices = 0;
		uint32 m_nextVertex = 0;
		uint32 m_nextIndex = 0;

		std::vector<GpuMesh> m_meshes{};
		std::vector<GpuMesh> m_gpuMeshCache{};

		std::unique_ptr<Buffer> m_positionBuffer;
		ShaderResourceViewHandle m_positionSrv{};

		std::unique_ptr<Buffer> m_indexBuffer;
		ShaderResourceViewHandle m_indexSrv{};

		std::unique_ptr<Buffer> m_normalBuffer;
		ShaderResourceViewHandle m_normalSrv{};

		std::unique_ptr<Buffer> m_uvBuffer;
		ShaderResourceViewHandle m_uvSrv{};

		std::unique_ptr<UploadBuffer> m_transformBuffer;
		ShaderResourceViewHandle m_transformBufferSrv{};

		// Async mesh pipeline
		std::queue<QueuedMesh> m_meshUploadQueue;
		uint32 m_maxMeshUploadsPerFrame = 32;
		std::vector<PendingMeshUpload> m_pendingUploads;
		std::vector<PendingBLAS> m_pendingBLASBuilds;

		// BLAS — one per registered mesh, built once
		std::array<std::unique_ptr<Buffer>, Config::FramesInFlight> m_blasScratch;
		std::vector<std::unique_ptr<Buffer>> m_blasBuffers;

		// Instance descs — triple-buffered, written by CPU each frame, read by BuildTlasPass
		std::array<std::unique_ptr<UploadBuffer>, Config::FramesInFlight> m_instanceDescs;
		uint32 m_lastInstanceCount = 0;

		uint32 m_maxObjects = 0;
		uint32 m_currentFrameIndex = 0;
	};
}
