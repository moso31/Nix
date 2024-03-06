#pragma once
#include <string>
#include "BaseDefs/DX12.h"
#include "NXAllocatorManager.h"
#include "ShaderStructures.h"
#include "NXInstance.h"

struct NXMeshData
{
	D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddress;
	UINT pageIndex;
	UINT pageByteOffset;
};

struct NXMeshViews
{
	D3D12_VERTEX_BUFFER_VIEW vbv;
	D3D12_INDEX_BUFFER_VIEW ibv;
	UINT vertexCount;
	UINT indexCount;
};

enum NXPlaneAxis
{
	POSITIVE_X,
	POSITIVE_Y,
	POSITIVE_Z,
	NEGATIVE_X,
	NEGATIVE_Y,
	NEGATIVE_Z
};

class NXPrefab;
class NXPrimitive;
class NXSubMeshGeometryEditor : public NXInstance<NXSubMeshGeometryEditor>
{
public:
	NXSubMeshGeometryEditor();
	~NXSubMeshGeometryEditor();

	void Init(ID3D12Device* pDevice);

	void CreateFBXPrefab(NXPrefab* pPrefab, const std::string& filePath, bool bAutoCalcTangents);

	void CreateBox(NXPrimitive* pMesh, float x = 1.0f, float y = 1.0f, float z = 1.0f);
	void CreateCylinder(NXPrimitive* pMesh, float radius = 1.0f, float length = 3.0f, int segmentCircle = 16, int segmentLength = 4);
	void CreatePlane(NXPrimitive* pMesh, float width = 0.5f, float height = 0.5f, NXPlaneAxis Axis = POSITIVE_Y);
	void CreateSphere(NXPrimitive* pMesh, float radius = 1.0f, int segmentHorizontal = 16, int segmentVertical = 16);
	void CreateSHSphere(NXPrimitive* pMesh, int basis_l, int basis_m, float radius = 1.0f, int segmentHorizontal = 64, int segmentVertical = 64);

	// Editor objects
	void CreateMoveArrows(NXPrimitive* pMesh);

	template<class TVertex>
	void CreateVBIB(const std::vector<TVertex>& vertices, const std::vector<UINT>& indices, const std::string& name)
	{
		if (m_data.find(name) != m_data.end())
		{
			//throw std::exception("Mesh name already exists.");
			return;
		}

		UINT vbSize = (UINT)vertices.size() * sizeof(TVertex);
		UINT ibSize = (UINT)indices.size() * sizeof(UINT);

		// 1. 在默认堆中分配这个Mesh的顶点和索引内存
		NXMeshData vbData, ibData;
		bool vbAlloc = m_vbAllocator->Alloc(vbSize, ResourceType_Default, vbData.GPUVirtualAddress, vbData.pageIndex, vbData.pageByteOffset);
		bool ibAlloc = m_ibAllocator->Alloc(ibSize, ResourceType_Default, ibData.GPUVirtualAddress, ibData.pageIndex, ibData.pageByteOffset);

		if (vbAlloc && ibAlloc)
		{
			UINT vbDataSize = (UINT)(sizeof(TVertex) * vertices.size());
			UINT ibDataSize = (UINT)(sizeof(UINT) * indices.size());

			// 2. 将顶点和索引数据拷贝到默认堆中，但这需要先准备上传堆，然后从上传堆拷贝到默认堆。
			NX12Util::CreateHeapProperties(D3D12_HEAP_TYPE_UPLOAD);

			ComPtr<ID3D12Resource> uploadVB = NX12Util::CreateBuffer(m_pDevice.Get(), "Temp uploadVB", vbDataSize, D3D12_HEAP_TYPE_UPLOAD);
			ComPtr<ID3D12Resource> uploadIB = NX12Util::CreateBuffer(m_pDevice.Get(), "Temp uploadIB", ibDataSize, D3D12_HEAP_TYPE_UPLOAD);

			m_tempUploadList.emplace_back(uploadVB);
			m_tempUploadList.emplace_back(uploadIB);

			// 2.2 将顶点和索引数据拷贝到上传堆
			void* mappedData;
			uploadVB->Map(0, nullptr, &mappedData);
			memcpy(mappedData, vertices.data(), vbDataSize);
			uploadVB->Unmap(0, nullptr);

			uploadIB->Map(0, nullptr, &mappedData);
			memcpy(mappedData, indices.data(), ibDataSize);
			uploadIB->Unmap(0, nullptr);

			// 2.3 从上传堆拷贝到默认堆
			m_vbAllocator->SetResourceState(m_pCommandList.Get(), vbData.pageIndex, D3D12_RESOURCE_STATE_COPY_DEST);
			m_vbAllocator->UpdateData(m_pCommandList.Get(), uploadVB.Get(), vbDataSize, vbData.pageIndex, vbData.pageByteOffset);
			m_vbAllocator->SetResourceState(m_pCommandList.Get(), vbData.pageIndex, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

			m_ibAllocator->SetResourceState(m_pCommandList.Get(), ibData.pageIndex, D3D12_RESOURCE_STATE_COPY_DEST);
			m_ibAllocator->UpdateData(m_pCommandList.Get(), uploadIB.Get(), ibDataSize, ibData.pageIndex, ibData.pageByteOffset);
			m_ibAllocator->SetResourceState(m_pCommandList.Get(), ibData.pageIndex, D3D12_RESOURCE_STATE_INDEX_BUFFER);

			// 3. 将Mesh数据存储到m_data中
			NXMeshViews views;
			views.vbv.BufferLocation = vbData.GPUVirtualAddress;
			views.vbv.SizeInBytes = vbDataSize;
			views.vbv.StrideInBytes = sizeof(TVertex);
			views.ibv.BufferLocation = ibData.GPUVirtualAddress;
			views.ibv.SizeInBytes = ibDataSize;
			views.ibv.Format = DXGI_FORMAT_R32_UINT;
			views.indexCount = (UINT)indices.size();
			views.vertexCount = (UINT)vertices.size();
			m_data[name] = std::move(views);
		}
	}

	const NXMeshViews& GetMeshViews(const std::string& name) { return m_data[name]; }

private:
	void InitCommonMeshes();

private:
	// 最重要的Mesh数据存储
	std::unordered_map<std::string, NXMeshViews> m_data; 

	CommittedAllocator* m_vbAllocator;
	CommittedAllocator* m_ibAllocator;

	ComPtr<ID3D12Device> m_pDevice;
	ComPtr<ID3D12CommandAllocator> m_pCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList> m_pCommandList;

	// 临时资源存放列表
	// 用于存放临时的上传堆资源，等待拷贝到默认堆
	// todo：找合适的时机清空此资源队列。
	std::vector<ComPtr<ID3D12Resource>> m_tempUploadList;

};
