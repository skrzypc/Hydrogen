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
#include "frameContext.h"

namespace Hydrogen
{
	class GpuDevice;
	class GpuUploader;
	struct Mesh;

	class GpuScene
	{
	public:
		void Initialize(GpuDevice& device, GpuUploader& uploader, uint32 maxVertices, uint32 maxIndices, uint32 sceneCapacity = 100'000, uint32 maxViews = 16, uint32 maxLights = 1024);

		// Enqueues a mesh for upload. Actual GPU upload happens during Update(), up to m_maxMeshUploadsPerFrame per frame.
		void RegisterMesh(MeshHandle handle, Mesh&& mesh);
		void RegisterMeshes(std::vector<MeshHandle>& meshHandles, std::vector<Mesh>& meshes);

		// TODO: Should this take the FrameContext at all? It only needs frameIndex and
		// renderScene.objects, and the context it gets holds a reference back to this scene.
		// While it does, nothing produced here (GPU instance count, ...) can live in the context.
		void Update(const FrameContext& frameContext);

		uint32 GetPositionBufferIndex() const { return m_positionSrv.index; }
		uint32 GetNormalBufferIndex() const { return m_normalSrv.index; }
		uint32 GetUvBufferIndex() const { return m_uvSrv.index; }
		uint32 GetIndexBufferIndex() const { return m_indexSrv.index; }
		uint32 GetMeshDataBufferIndex() const { return m_meshDataSrvs[m_currentFrameIndex].index; }
		uint32 GetInstanceDataBufferIndex() const { return m_instanceDataSrvs[m_currentFrameIndex].index; }
		uint32 GetTransformBufferIndex() const { return m_transformSrvs[m_currentFrameIndex].index; }
		uint32 GetMaterialDataBufferIndex() const { return m_materialDataSrvs[m_currentFrameIndex].index; }
		uint32 GetLightBufferIndex() const { return m_lightSrvs[m_currentFrameIndex].index; }
		uint32 GetLightCount() const { return m_lightCount; }

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

		void UpdateMeshData();
		void UpdateTransforms(std::span<const RenderObject> objects);
		void UpdateMaterials();
		void UpdateLights(std::span<const RenderLight> lights);

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

		// Written every frame, so each frame in flight needs its own copy.
		std::array<std::unique_ptr<UploadBuffer>, Config::FramesInFlight> m_transformBuffers{};
		std::array<ShaderResourceViewHandle, Config::FramesInFlight> m_transformSrvs{};

		// Gathered in cached memory and copied to the upload buffer in one go. Kept around so the
		// allocation is reused between frames.
		std::vector<DirectX::XMFLOAT4X4> m_transformStaging{};

		std::array<std::unique_ptr<UploadBuffer>, Config::FramesInFlight> m_meshDataBuffers{};
		std::array<ShaderResourceViewHandle, Config::FramesInFlight> m_meshDataSrvs{};
		std::vector<GpuMeshData> m_meshDataStaging{};

		std::array<std::unique_ptr<UploadBuffer>, Config::FramesInFlight> m_instanceDataBuffers{};
		std::array<ShaderResourceViewHandle, Config::FramesInFlight> m_instanceDataSrvs{};
		std::vector<GpuInstanceData> m_instanceDataStaging{};

		std::array<std::unique_ptr<UploadBuffer>, Config::FramesInFlight> m_materialDataBuffers{};
		std::array<ShaderResourceViewHandle, Config::FramesInFlight> m_materialDataSrvs{};
		GpuMaterialData m_defaultMaterialData{};

		std::array<std::unique_ptr<UploadBuffer>, Config::FramesInFlight> m_lightBuffers{};
		std::array<ShaderResourceViewHandle, Config::FramesInFlight> m_lightSrvs{};
		std::vector<GpuLight> m_lightStaging{};

		uint32 m_maxLights = 0;
		uint32 m_lightCount = 0;

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
