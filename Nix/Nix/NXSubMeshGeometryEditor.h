#pragma once
#include "NXSubMeshCommon.h"
#include "NXAllocatorManager.h"
#include "ShaderStructures.h"
#include "NXInstance.h"
#include "NXStructuredBuffer.h"

class NXPrefab;
class NXPrimitive;
class NXTerrain;
class NXSubMeshGeometryEditor : public NXInstance<NXSubMeshGeometryEditor>
{
public:
	NXSubMeshGeometryEditor();
	virtual ~NXSubMeshGeometryEditor();

	void Init(ID3D12Device* pDevice);

	void CreateFBXPrefab(NXPrefab* pPrefab, const std::string& filePath, bool bAutoCalcTangents);

	void CreateBox(NXPrimitive* pMesh, float x = 1.0f, float y = 1.0f, float z = 1.0f);
	void CreateCylinder(NXPrimitive* pMesh, float radius = 1.0f, float length = 3.0f, int segmentCircle = 16, int segmentLength = 4);
	void CreatePlane(NXPrimitive* pMesh, float width = 0.5f, float height = 0.5f, NXPlaneAxis Axis = POSITIVE_Y);
	void CreateSphere(NXPrimitive* pMesh, float radius = 1.0f, int segmentHorizontal = 16, int segmentVertical = 16);
	void CreateSHSphere(NXPrimitive* pMesh, int basis_l, int basis_m, float radius = 1.0f, int segmentHorizontal = 64, int segmentVertical = 64);
	void CreateTerrain(NXTerrain* pTerrain, int rawSize, int gridSize, int worldSize, const std::filesystem::path& rawFile);

	// Editor objects
	void CreateMoveArrows(NXPrimitive* pMesh);

	void CreateBuffers(std::vector<NXRawMeshView>& rawViews, const std::string& name)
	{
		NXMeshViews* pMeshView = nullptr;
		{
			std::lock_guard<std::mutex> lock(m_mutex);

			if (m_data.find(name) != m_data.end())
				return;

			pMeshView = new NXMeshViews(name, rawViews);
			m_data.emplace(name, pMeshView);
		}

		if (!pMeshView)
			return;

		// 异步创建rawViews：顶点/索引/instanceData等数据的vbv/ibv，并上传
		// 上层目前还能确保rawViews 都是成员变量，所以这里不需要担心生命周期问题，但将来不好说……
		
		// 线程A 执行 std::thread
		printf("主线程%s\n", name.c_str());
		std::thread([&rawViews, name, pMeshView]() { 
			for (auto& view : rawViews)
			{
				// 内有异步分配逻辑，由线程B执行
				NXStructuredBuffer pBuffer(view.stride, view.span.size());
				// 等待线程B完成分配 get GPUAddress.
				pBuffer.WaitCreateComplete();

				view.gpuAddress = pBuffer.GetGPUAddress();

				uint32_t byteSize = view.span.size_bytes();
				UploadTaskContext ctx(name + "_VB");
				if (NXUploadSystem->BuildTask(byteSize, ctx))
				{
					// ctx.pResourceData/pResourceOffset是 上传系统的UploadRingBuffer 的临时资源和偏移量
					// pVB.GetD3DResourceAndOffset(byteOffset) 是 D3D默认堆上 的偏移量，也就是GPU资源
					// 不要搞混了

					uint8_t* pDst = ctx.pResourceData + ctx.pResourceOffset;

					// 拷贝到上传堆
					memcpy(pDst, view.span.data(), byteSize);

					// 从上传堆拷贝到默认堆
					uint64_t byteOffset = 0;
					ID3D12Resource* pDstResource = pBuffer.GetD3DResourceAndOffset(byteOffset);
					ctx.pOwner->pCmdList->CopyBufferRegion(pDstResource, byteOffset, ctx.pResource, ctx.pResourceOffset, byteSize);

					// 上传数据并同步到gpu，异步线程C 负责执行
					NXUploadSystem->FinishTask(ctx, [pMeshView, name, taskID = ctx.pOwner->selfID]() {
						pMeshView->ProcessOne(); // 顶点数据上传完成，通知 loadCounter - 1
						});
				}
			}
		}).detach();
	}

	const NXMeshViews& GetMeshViews(const std::string& name);

	void Release();

private:
	void InitCommonMeshes();

private:
	// Mesh data
	std::unordered_map<std::string, NXMeshViews*> m_data; 

	std::mutex m_mutex;

	NXSubMesh<float>* m_subMeshUnknown;
	NXSubMesh<VertexPT>* m_subMeshRT;
};
