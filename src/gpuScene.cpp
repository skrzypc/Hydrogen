
#include "gpuScene.h"

#include <DirectXMath.h>

#include "device.h"
#include "gpuUploader.h"
#include "graphicsContext.h"
#include "mesh.h"
#include "stringUtilities.h"
#include "verifier.h"

namespace Hydrogen
{
	static_assert(static_cast<uint32>(eLightType::Directional) == LightTypeDirectional);
	static_assert(static_cast<uint32>(eLightType::Point) == LightTypePoint);
	static_assert(static_cast<uint32>(eLightType::Spot) == LightTypeSpot);

	void GpuScene::Initialize(GpuDevice& device, GpuUploader& uploader, uint32 maxVertices, uint32 maxIndices, uint32 sceneCapacity, uint32 maxViews, uint32 maxLights)
	{
		m_pDevice = &device;
		m_pUploader = &uploader;
		m_maxVertices = maxVertices;
		m_maxIndices = maxIndices;
		m_sceneCapacity = sceneCapacity;
		m_maxLights = maxLights;

		ResourceState initialState{};

		m_positionBuffer = device.CreateBuffer(L"H2_SCENE_POSITIONS", Buffer::Desc{ .size = maxVertices * sizeof(DirectX::XMFLOAT3) }, initialState);
		m_normalBuffer = device.CreateBuffer(L"H2_SCENE_NORMALS", Buffer::Desc{ .size = maxVertices * sizeof(DirectX::XMFLOAT3) }, initialState);
		m_uvBuffer = device.CreateBuffer(L"H2_SCENE_UVS", Buffer::Desc{ .size = maxVertices * sizeof(DirectX::XMFLOAT2) }, initialState);
		m_indexBuffer = device.CreateBuffer(L"H2_SCENE_INDICES", Buffer::Desc{ .size = maxIndices * sizeof(uint32) }, initialState);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		srvDesc.Buffer.NumElements = maxVertices;
		srvDesc.Buffer.StructureByteStride = sizeof(DirectX::XMFLOAT3);
		m_positionSrv = device.CreateShaderResourceView(m_positionBuffer.get(), srvDesc);
		m_normalSrv = device.CreateShaderResourceView(m_normalBuffer.get(), srvDesc);

		srvDesc.Buffer.StructureByteStride = sizeof(DirectX::XMFLOAT2);
		m_uvSrv = device.CreateShaderResourceView(m_uvBuffer.get(), srvDesc);

		srvDesc.Buffer.NumElements = maxIndices;
		srvDesc.Buffer.StructureByteStride = sizeof(uint32);
		m_indexSrv = device.CreateShaderResourceView(m_indexBuffer.get(), srvDesc);

		srvDesc.Buffer.NumElements = m_sceneCapacity;
		srvDesc.Buffer.StructureByteStride = sizeof(DirectX::XMFLOAT4X4);

		for (uint32 i = 0; i < Config::FramesInFlight; ++i)
		{
			m_transformBuffers[i] = device.CreateUploadBuffer(L"H2_SCENE_TRANSFORMS", m_sceneCapacity * sizeof(DirectX::XMFLOAT4X4));
			m_transformSrvs[i] = device.CreateShaderResourceView(m_transformBuffers[i].get(), srvDesc);

			srvDesc.Buffer.StructureByteStride = sizeof(GpuMeshData);
			m_meshDataBuffers[i] = device.CreateUploadBuffer(L"H2_SCENE_MESH_DATA", m_sceneCapacity * sizeof(GpuMeshData));
			m_meshDataSrvs[i] = device.CreateShaderResourceView(m_meshDataBuffers[i].get(), srvDesc);

			srvDesc.Buffer.StructureByteStride = sizeof(GpuInstanceData);
			m_instanceDataBuffers[i] = device.CreateUploadBuffer(L"H2_SCENE_INSTANCE_DATA", m_sceneCapacity * sizeof(GpuInstanceData));
			m_instanceDataSrvs[i] = device.CreateShaderResourceView(m_instanceDataBuffers[i].get(), srvDesc);

			srvDesc.Buffer.NumElements = 1;
			srvDesc.Buffer.StructureByteStride = sizeof(GpuMaterialData);
			m_materialDataBuffers[i] = device.CreateUploadBuffer(L"H2_SCENE_MATERIAL_DATA", sizeof(GpuMaterialData));
			m_materialDataSrvs[i] = device.CreateShaderResourceView(m_materialDataBuffers[i].get(), srvDesc);

			m_instanceDescs[i] = device.CreateUploadBuffer(L"H2_SCENE_TLAS_INSTANCES", m_sceneCapacity * sizeof(D3D12_RAYTRACING_INSTANCE_DESC));

			srvDesc.Buffer.NumElements = m_sceneCapacity;
			srvDesc.Buffer.StructureByteStride = sizeof(DirectX::XMFLOAT4X4);
		}

		m_defaultMaterialData =
		{
			.baseColor = { 0.8f, 0.8f, 0.8f },
			.roughness = 1.0f,
			.emissive = { 0.0f, 0.0f, 0.0f },
			.metallic = 0.0f,
		};

		srvDesc.Buffer.NumElements = m_maxLights;
		srvDesc.Buffer.StructureByteStride = sizeof(GpuLight);

		for (uint32 i = 0; i < Config::FramesInFlight; ++i)
		{
			m_lightBuffers[i] = device.CreateUploadBuffer(L"H2_SCENE_LIGHTS", m_maxLights * sizeof(GpuLight));
			m_lightSrvs[i] = device.CreateShaderResourceView(m_lightBuffers[i].get(), srvDesc);
		}
	}

	void GpuScene::RegisterMesh(MeshHandle handle, Mesh&& mesh)
	{
		if (handle.id >= m_gpuMeshCache.size())
		{
			m_gpuMeshCache.resize(handle.id + 1);
		}

		if (m_gpuMeshCache[handle.id].state != GpuMeshState::Empty)
		{
			return;
		}

		m_gpuMeshCache[handle.id].state = GpuMeshState::Registered;
		m_meshUploadQueue.push(MeshUploadData{ handle, std::move(mesh) });
	}

	void GpuScene::RegisterMeshes(std::vector<MeshHandle>& meshHandles, std::vector<Mesh>& meshes)
	{
		for (uint32 i = 0; i < meshHandles.size(); ++i)
		{
			RegisterMesh(meshHandles[i], std::move(meshes[i]));
		}
	}

	void GpuScene::Update(const FrameContext& frameContext)
	{
		m_currentFrameIndex = frameContext.frameIndex;

		ProcessMeshUploads();
		ProcessBlasBuilds();
		PublishReadyMeshes();

		UpdateMeshData();
		UpdateTransforms(frameContext.renderScene.objects);
		UpdateMaterials();
		UpdateLights(frameContext.renderScene.lights);
	}

	void GpuScene::ProcessMeshUploads()
	{
		if (m_meshUploadQueue.empty())
		{
			return;
		}

		const uint32 firstPendingIdx = static_cast<uint32>(m_inFlightMeshUploads.size());
		uint32 count = 0;

		while (!m_meshUploadQueue.empty() && count < m_maxMeshUploadsPerFrame)
		{
			MeshUploadData& queued = m_meshUploadQueue.front();

			UploadMeshGeometry(queued.handle, queued.mesh);
			m_inFlightMeshUploads.push_back(InFlightMeshUploadData{ queued.handle, 0 });
			m_meshUploadQueue.pop();
			++count;
		}

		const uint64 fence = m_pUploader->Flush();
		for (uint32 i = firstPendingIdx; i < static_cast<uint32>(m_inFlightMeshUploads.size()); ++i)
		{
			m_inFlightMeshUploads[i].copyFence = fence;
		}
	}

	void GpuScene::ProcessBlasBuilds()
	{
		if (m_inFlightMeshUploads.empty())
		{
			return;
		}

		const uint64 completedCopy = m_pDevice->GetCompletedFenceValue<eQueueType::Copy>();

		std::vector<InFlightMeshUploadData> ready{};
		std::vector<InFlightMeshUploadData> remaining{};
		for (auto& entry : m_inFlightMeshUploads)
		{
			if (entry.copyFence <= completedCopy)
			{
				m_gpuMeshCache[entry.handle.id].state = GpuMeshState::GeometryReady;
				ready.push_back(entry);
			}
			else
			{
				remaining.push_back(entry);
			}
		}
		m_inFlightMeshUploads = std::move(remaining);

		if (!ready.empty())
		{
			BuildBlas(ready);
		}
	}

	void GpuScene::BuildBlas(std::span<const InFlightMeshUploadData> uploads)
	{
		uint64 totalScratchSize = 0ull;
		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> rtGeometryDescs(uploads.size());
		std::vector<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO> blasPrebuildInfos(uploads.size());
		std::vector<uint64> scratchOffsets(uploads.size());

		for (uint32 i = 0; i < uploads.size(); ++i)
		{
			const GpuMesh& mesh = m_gpuMeshCache[uploads[i].handle.id];

			rtGeometryDescs[i] = {};
			rtGeometryDescs[i].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
			rtGeometryDescs[i].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
			rtGeometryDescs[i].Triangles.VertexBuffer.StartAddress = m_positionBuffer->GetResource()->GetGPUVirtualAddress() + mesh.baseVertex * sizeof(DirectX::XMFLOAT3);
			rtGeometryDescs[i].Triangles.VertexBuffer.StrideInBytes = sizeof(DirectX::XMFLOAT3);
			rtGeometryDescs[i].Triangles.VertexCount = mesh.vertexCount;
			rtGeometryDescs[i].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
			rtGeometryDescs[i].Triangles.IndexBuffer = m_indexBuffer->GetResource()->GetGPUVirtualAddress() + mesh.baseIndex * sizeof(uint32);
			rtGeometryDescs[i].Triangles.IndexCount = mesh.indexCount;
			rtGeometryDescs[i].Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;

			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
			inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
			inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
			inputs.NumDescs = 1;
			inputs.pGeometryDescs = &rtGeometryDescs[i];

			m_pDevice->GetDxDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &blasPrebuildInfos[i]);

			scratchOffsets[i] = totalScratchSize;
			const uint64 alignedScratch = (blasPrebuildInfos[i].ScratchDataSizeInBytes + D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT - 1)
				& ~static_cast<uint64>(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT - 1);
			totalScratchSize += alignedScratch;
		}

		auto& scratch = m_blasScratchBuffers[m_currentFrameIndex];
		if (!scratch || scratch->GetDesc().size < totalScratchSize)
		{
			ResourceState uavState{};
			scratch = m_pDevice->CreateBuffer(L"H2_BLAS_SCRATCH",
				Buffer::Desc{ .size = totalScratchSize, .flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS },
				uavState);
		}
		const D3D12_GPU_VIRTUAL_ADDRESS scratchBase = scratch->GetResource()->GetGPUVirtualAddress();

		GraphicsContext graphicsContext = m_pDevice->AcquireGraphicsContext();
		const uint32 firstBlasIdx = static_cast<uint32>(m_blasBuffers.size());

		std::vector<D3D12_BUFFER_BARRIER> blasBarriers;
		blasBarriers.reserve(uploads.size());

		for (uint32 i = 0; i < uploads.size(); ++i)
		{
			const GpuMesh& mesh = m_gpuMeshCache[uploads[i].handle.id];

			ResourceState asState{};
			asState.access = D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE;

			m_blasBuffers.push_back(m_pDevice->CreateBuffer(
				String::ToWide(String::Format("H2_BLAS_{}", String::ToUpper(mesh.name))),
				Buffer::Desc{
					.size = blasPrebuildInfos[i].ResultDataMaxSizeInBytes,
					.flags = D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE },
					asState));

			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
			inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
			inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
			inputs.NumDescs = 1;
			inputs.pGeometryDescs = &rtGeometryDescs[i];

			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc{};
			buildDesc.Inputs = inputs;
			buildDesc.DestAccelerationStructureData = m_blasBuffers.back()->GetResource()->GetGPUVirtualAddress();
			buildDesc.ScratchAccelerationStructureData = scratchBase + scratchOffsets[i];

			graphicsContext.CmdList()->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

			blasBarriers.push_back(D3D12_BUFFER_BARRIER
				{
					.SyncBefore = D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE,
					.SyncAfter = D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE,
					.AccessBefore = D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE,
					.AccessAfter = D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ,
					.pResource = m_blasBuffers.back()->GetResource(),
					.Offset = 0,
					.Size = UINT64_MAX,
				});

			m_inFlightBlasBuilds.push_back(InFlightBlasBuildData{ uploads[i].handle, firstBlasIdx + i, 0 });
		}

		D3D12_BARRIER_GROUP blasBarrierGroup
		{
			.Type = D3D12_BARRIER_TYPE_BUFFER,
			.NumBarriers = static_cast<uint32>(blasBarriers.size()),
			.pBufferBarriers = blasBarriers.data(),
		};
		graphicsContext.CmdList()->Barrier(1, &blasBarrierGroup);

		const uint64 fence = m_pDevice->ExecuteGraphicsContext(std::move(graphicsContext));
		for (auto& entry : m_inFlightBlasBuilds)
		{
			if (entry.buildFence == 0)
			{
				entry.buildFence = fence;
			}
		}
	}

	void GpuScene::PublishReadyMeshes()
	{
		if (m_inFlightBlasBuilds.empty())
		{
			return;
		}

		const uint64 completedDirect = m_pDevice->GetCompletedFenceValue<eQueueType::Direct>();

		std::vector<InFlightBlasBuildData> remaining;
		for (auto& entry : m_inFlightBlasBuilds)
		{
			if (entry.buildFence <= completedDirect)
			{
				GpuMesh& gpuMesh = m_gpuMeshCache[entry.handle.id];
				gpuMesh.blasAddress = m_blasBuffers[entry.blasBufferIndex]->GetResource()->GetGPUVirtualAddress();
				gpuMesh.state = GpuMeshState::BlasReady;
			}
			else
			{
				remaining.push_back(entry);
			}
		}
		m_inFlightBlasBuilds = std::move(remaining);
	}

	void GpuScene::UpdateMeshData()
	{
		H2_VERIFY_FATAL(m_gpuMeshCache.size() <= m_sceneCapacity, "GpuScene mesh count exceeds scene capacity!");

		m_meshDataStaging.resize(m_gpuMeshCache.size());
		for (uint32 meshIndex = 0; meshIndex < static_cast<uint32>(m_gpuMeshCache.size()); ++meshIndex)
		{
			const GpuMesh& mesh = m_gpuMeshCache[meshIndex];
			m_meshDataStaging[meshIndex] =
			{
				.baseVertex = mesh.baseVertex,
				.vertexCount = mesh.vertexCount,
				.baseIndex = mesh.baseIndex,
				.indexCount = mesh.indexCount,
			};
		}

		if (!m_meshDataStaging.empty())
		{
			m_meshDataBuffers[m_currentFrameIndex]->Write(
				m_meshDataStaging.data(),
				m_meshDataStaging.size() * sizeof(GpuMeshData));
		}
	}

	void GpuScene::UpdateTransforms(std::span<const RenderObject> objects)
	{
		H2_VERIFY_FATAL(objects.size() <= m_sceneCapacity, "RenderScene object count exceeds scene capacity!");

		m_transformStaging.resize(objects.size());
		m_instanceDataStaging.resize(objects.size());

		auto* pDescs = reinterpret_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(
			m_instanceDescs[m_currentFrameIndex]->GetMappedPtr());

		uint32 instanceCount = 0;
		for (uint32 transformIndex = 0; transformIndex < static_cast<uint32>(objects.size()); ++transformIndex)
		{
			const RenderObject& obj = objects[transformIndex];

			m_transformStaging[transformIndex] = obj.worldMatrix;
			m_instanceDataStaging[transformIndex] =
			{
				.meshDataIndex = obj.mesh.id,
				.transformIndex = transformIndex,
				.materialDataIndex = obj.materialDataIndex,
			};

			const GpuMesh* pMesh = GetGpuMesh(obj.mesh);
			if (!pMesh || pMesh->state != GpuMeshState::BlasReady)
			{
				continue;
			}

			D3D12_RAYTRACING_INSTANCE_DESC& desc = pDescs[instanceCount];
			desc = {};
			DirectX::XMMATRIX worldMatrixT = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&obj.worldMatrix));
			memcpy(desc.Transform, &worldMatrixT, sizeof(desc.Transform));
			desc.InstanceID = transformIndex;
			desc.InstanceMask = 0xFF;
			desc.AccelerationStructure = pMesh->blasAddress;
			++instanceCount;
		}

		if (!m_transformStaging.empty())
		{
			m_transformBuffers[m_currentFrameIndex]->Write(
				m_transformStaging.data(),
				m_transformStaging.size() * sizeof(DirectX::XMFLOAT4X4));
			m_instanceDataBuffers[m_currentFrameIndex]->Write(
				m_instanceDataStaging.data(),
				m_instanceDataStaging.size() * sizeof(GpuInstanceData));
		}

		m_lastInstanceCount = instanceCount;
	}

	void GpuScene::UpdateMaterials()
	{
		m_materialDataBuffers[m_currentFrameIndex]->Write(&m_defaultMaterialData, sizeof(GpuMaterialData));
	}

	void GpuScene::UpdateLights(std::span<const RenderLight> lights)
	{
		H2_VERIFY_FATAL(lights.size() <= m_maxLights, "RenderScene light count exceeds light capacity!");

		m_lightCount = static_cast<uint32>(lights.size());
		m_lightStaging.resize(lights.size());

		for (uint32 lightIndex = 0; lightIndex < m_lightCount; ++lightIndex)
		{
			const RenderLight& renderLight = lights[lightIndex];
			const Light& light = renderLight.light;

			GpuLight& gpuLight = m_lightStaging[lightIndex];
			gpuLight = {};
			gpuLight.position = renderLight.position;
			gpuLight.type = static_cast<uint32>(light.type);
			gpuLight.color = light.color;
			gpuLight.intensity = light.intensity;
			gpuLight.direction = renderLight.direction;
			gpuLight.range = light.range.value_or(std::numeric_limits<float32>::max());
			gpuLight.cosInnerConeAngle = std::cos(light.innerConeAngle.value_or(0.0f));
			gpuLight.cosOuterConeAngle = std::cos(light.outerConeAngle.value_or(DirectX::XM_PIDIV4));
		}

		if (!m_lightStaging.empty())
		{
			m_lightBuffers[m_currentFrameIndex]->Write(m_lightStaging.data(), m_lightStaging.size() * sizeof(GpuLight));
		}
	}

	void GpuScene::UploadMeshGeometry(MeshHandle handle, const Mesh& src)
	{
		const uint32 vertexCount = static_cast<uint32>(src.positions.size());
		const uint32 indexCount = static_cast<uint32>(src.indices.size());

		H2_VERIFY_FATAL(m_nextVertex + vertexCount <= m_maxVertices, "GpuScene vertex capacity exceeded!");
		H2_VERIFY_FATAL(m_nextIndex + indexCount <= m_maxIndices, "GpuScene index capacity exceeded!");

		m_pUploader->Upload(src.positions.data(), vertexCount * sizeof(DirectX::XMFLOAT3), m_positionBuffer.get(), m_nextVertex * sizeof(DirectX::XMFLOAT3));
		m_pUploader->Upload(src.normals.data(), vertexCount * sizeof(DirectX::XMFLOAT3), m_normalBuffer.get(), m_nextVertex * sizeof(DirectX::XMFLOAT3));
		m_pUploader->Upload(src.uvs.data(), vertexCount * sizeof(DirectX::XMFLOAT2), m_uvBuffer.get(), m_nextVertex * sizeof(DirectX::XMFLOAT2));
		m_pUploader->Upload(src.indices.data(), indexCount * sizeof(uint32), m_indexBuffer.get(), m_nextIndex * sizeof(uint32));

		GpuMesh& gpuMesh = m_gpuMeshCache[handle.id];
		gpuMesh.name = src.name;
		gpuMesh.baseVertex = m_nextVertex;
		gpuMesh.vertexCount = vertexCount;
		gpuMesh.baseIndex = m_nextIndex;
		gpuMesh.indexCount = indexCount;

		m_nextVertex += vertexCount;
		m_nextIndex += indexCount;
	}

	const GpuMesh* GpuScene::GetGpuMesh(MeshHandle handle) const
	{
		if (handle.id >= m_gpuMeshCache.size()) { return nullptr; }
		return &m_gpuMeshCache[handle.id];
	}
}
