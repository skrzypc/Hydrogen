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
		void Initialize(GpuDevice& device, GpuUploader& uploader, uint32 maxVertices, uint32 maxIndices, uint32 sceneCapacity = 100'000, uint32 maxViews = 16);

		// Enqueues a mesh for upload. Actual GPU upload happens during Update(), up to m_maxMeshUploadsPerFrame per frame.
		void RegisterMesh(MeshHandle handle, Mesh&& mesh);
		void RegisterMeshes(std::vector<MeshHandle>& meshHandles, std::vector<Mesh>& meshes);

		SceneBindings Update(const RenderScene& renderScene, uint32 frameIndex);

		const Buffer* GetIndexBuffer() const { return m_indexBuffer.get(); }
		const GpuMesh* GetGpuMesh(MeshHandle handle) const;

		const UploadBuffer* GetInstanceDescs() const { return m_instanceDescs[m_currentFrameIndex].get(); }
		uint32 GetInstanceCount() const { return m_lastInstanceCount; }

	private:
		struct MeshUploadData { MeshHandle handle{}; Mesh mesh{}; };
		struct InFlightMeshUploadData { MeshHandle handle{}; uint64 copyFence = 0; };
		struct InFlightBlasBuildData { MeshHandle handle{}; uint32 blasBufferIndex = 0; uint64 buildFence = 0; };

		void ProcessMeshUploads();
		void ProcessBlasBuilds();
		void PublishReadyMeshes();

		void UploadMeshGeometry(MeshHandle handle, const Mesh& mesh);
		void BuildBlas(std::span<const InFlightMeshUploadData> uploads);

		void UpdateTransforms(std::span<const RenderObject> objects);

		GpuDevice* m_pDevice = nullptr;
		GpuUploader* m_pUploader = nullptr;

		uint32 m_maxVertices = 0;
		uint32 m_maxIndices = 0;
		uint32 m_sceneCapacity = 0;
		uint32 m_nextVertex = 0;
		uint32 m_nextIndex = 0;

		std::vector<GpuMesh> m_gpuMeshCache{};

		std::unique_ptr<Buffer> m_positionBuffer{};
		ShaderResourceViewHandle m_positionSrv{};

		std::unique_ptr<Buffer> m_indexBuffer{};
		ShaderResourceViewHandle m_indexSrv{};

		std::unique_ptr<Buffer> m_normalBuffer{};
		ShaderResourceViewHandle m_normalSrv{};

		std::unique_ptr<Buffer> m_uvBuffer{};
		ShaderResourceViewHandle m_uvSrv{};

		std::unique_ptr<UploadBuffer> m_transformBuffer{};
		ShaderResourceViewHandle m_transformBufferSrv{};

		// Async mesh pipeline
		static constexpr uint32 m_maxMeshUploadsPerFrame = 32;
		std::queue<MeshUploadData> m_meshUploadQueue{};
		std::vector<InFlightMeshUploadData> m_inFlightMeshUploads{};
		std::vector<InFlightBlasBuildData> m_inFlightBlasBuilds{};

		std::array<std::unique_ptr<Buffer>, Config::FramesInFlight> m_blasScratchBuffers{};
		std::vector<std::unique_ptr<Buffer>> m_blasBuffers{};

		std::array<std::unique_ptr<UploadBuffer>, Config::FramesInFlight> m_instanceDescs{};
		uint32 m_lastInstanceCount = 0;

		uint32 m_currentFrameIndex = 0;
	};
}
