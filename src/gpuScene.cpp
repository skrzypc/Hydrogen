#include "gpuScene.h"

#include <DirectXMath.h>

#include "device.h"
#include "gpuUploader.h"
#include "staticMesh.h"
#include "verifier.h"

namespace Hydrogen
{
	void GpuScene::Initialize(GpuDevice& device, GpuUploader& uploader, uint32 maxVertices, uint32 maxIndices, uint32 maxViews)
	{
		m_pDevice = &device;
		m_pUploader = &uploader;
		m_maxVertices = maxVertices;
		m_maxIndices = maxIndices;

		ResourceState initialState{};

		m_positionBuffer = device.CreateBuffer(L"H2_SCENE_POSITIONS",
			Buffer::Desc{ .size = maxVertices * sizeof(DirectX::XMFLOAT3) },
			initialState
		);
		m_normalBuffer = device.CreateBuffer(L"H2_SCENE_NORMALS",
			Buffer::Desc{ .size = maxVertices * sizeof(DirectX::XMFLOAT3) },
			initialState
		);
		m_uvBuffer = device.CreateBuffer(L"H2_SCENE_UVS",
			Buffer::Desc{ .size = maxVertices * sizeof(DirectX::XMFLOAT2) },
			initialState
		);
		m_indexBuffer = device.CreateBuffer(L"H2_SCENE_INDICES",
			Buffer::Desc{ .size = maxIndices * sizeof(uint32) },
			initialState
		);

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

		// View buffer — CPU-writable each frame, one entry per supported view.
		m_viewBuffer = std::make_unique<UploadBuffer>(device, maxViews * sizeof(ShaderInterop::ViewData), L"H2_SCENE_VIEW_BUFFER");

		srvDesc.Buffer.NumElements = maxViews;
		srvDesc.Buffer.StructureByteStride = sizeof(ShaderInterop::ViewData);
		m_viewBufferSrv = device.CreateShaderResourceView(m_viewBuffer.get(), srvDesc);
	}

	void GpuScene::UpdateView(const ShaderInterop::ViewData& viewData, uint32 viewIndex)
	{
		m_viewBuffer->Write(&viewData, sizeof(ShaderInterop::ViewData), viewIndex * sizeof(ShaderInterop::ViewData));
	}

	std::pair<GpuMesh, uint64> GpuScene::AddMesh(const StaticMesh& src)
	{
		const uint32 vertexCount = static_cast<uint32>(src.positions.size());
		const uint32 indexCount = static_cast<uint32>(src.indices.size());

		H2_VERIFY_FATAL(m_nextVertex + vertexCount <= m_maxVertices, "GpuScene vertex capacity exceeded!");
		H2_VERIFY_FATAL(m_nextIndex + indexCount <= m_maxIndices, "GpuScene index capacity exceeded!");

		m_pUploader->Upload(src.positions.data(), vertexCount * sizeof(DirectX::XMFLOAT3), m_positionBuffer.get(), m_nextVertex * sizeof(DirectX::XMFLOAT3));
		m_pUploader->Upload(src.normals.data(), vertexCount * sizeof(DirectX::XMFLOAT3), m_normalBuffer.get(), m_nextVertex * sizeof(DirectX::XMFLOAT3));
		m_pUploader->Upload(src.uvs.data(), vertexCount * sizeof(DirectX::XMFLOAT2), m_uvBuffer.get(), m_nextVertex * sizeof(DirectX::XMFLOAT2));
		m_pUploader->Upload(src.indices.data(), indexCount * sizeof(uint32), m_indexBuffer.get(), m_nextIndex * sizeof(uint32));

		GpuMesh& gpuMesh = m_meshes.emplace_back();
		gpuMesh.name = src.name;
		gpuMesh.baseVertex = m_nextVertex;
		gpuMesh.vertexCount = vertexCount;
		gpuMesh.baseIndex = m_nextIndex;
		gpuMesh.indexCount = indexCount;

		m_nextVertex += vertexCount;
		m_nextIndex += indexCount;

		uint64 fence = m_pUploader->Flush();
		return { gpuMesh, fence };
	}

	std::pair<std::vector<GpuMesh>, uint64> GpuScene::AddMeshes(std::span<const StaticMesh> meshes)
	{
		std::vector<GpuMesh> result;
		result.reserve(meshes.size());

		for (const StaticMesh& mesh : meshes)
		{
			const uint32 vertexCount = static_cast<uint32>(mesh.positions.size());
			const uint32 indexCount = static_cast<uint32>(mesh.indices.size());

			H2_VERIFY_FATAL(m_nextVertex + vertexCount <= m_maxVertices, "GpuScene vertex capacity exceeded!");
			H2_VERIFY_FATAL(m_nextIndex + indexCount <= m_maxIndices, "GpuScene index capacity exceeded!");

			m_pUploader->Upload(mesh.positions.data(), vertexCount * sizeof(DirectX::XMFLOAT3), m_positionBuffer.get(), m_nextVertex * sizeof(DirectX::XMFLOAT3));
			m_pUploader->Upload(mesh.normals.data(), vertexCount * sizeof(DirectX::XMFLOAT3), m_normalBuffer.get(), m_nextVertex * sizeof(DirectX::XMFLOAT3));
			m_pUploader->Upload(mesh.uvs.data(), vertexCount * sizeof(DirectX::XMFLOAT2), m_uvBuffer.get(), m_nextVertex * sizeof(DirectX::XMFLOAT2));
			m_pUploader->Upload(mesh.indices.data(), indexCount * sizeof(uint32), m_indexBuffer.get(), m_nextIndex * sizeof(uint32));

			GpuMesh& gpuMesh = m_meshes.emplace_back();
			gpuMesh.name = mesh.name;
			gpuMesh.baseVertex = m_nextVertex;
			gpuMesh.vertexCount = vertexCount;
			gpuMesh.baseIndex = m_nextIndex;
			gpuMesh.indexCount = indexCount;

			result.push_back(gpuMesh);

			m_nextVertex += vertexCount;
			m_nextIndex += indexCount;
		}

		uint64 fence = m_pUploader->Flush();
		return { result, fence };
	}
}
