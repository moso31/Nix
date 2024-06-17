#pragma once
#include "NXRenderableObject.h"
#include "NXSubMesh.h"

class NXPrimitive : public NXRenderableObject
{
public:
	NXPrimitive();
	~NXPrimitive() {}

	virtual NXPrimitive* IsPrimitive() override { return this; }

	// 自动计算SubMesh下所有顶点的切线数据。
	void CalculateTangents(bool bUpdateVertexIndexBuffer = false);

	bool RayCastPrimitive(const Ray& worldRay, NXHit& outHitInfo, float& outDist);

	UINT GetSubMeshCount() { return (UINT)m_pSubMeshes.size(); }
	NXSubMeshBase* GetSubMesh(UINT index) { return m_pSubMeshes[index].get(); }

	void ClearSubMeshes();
	void AddSubMesh(NXSubMeshBase* pSubMesh);
	void ResizeSubMesh(UINT size);
	void ReloadSubMesh(UINT index, NXSubMeshBase* pSubMesh);

	void InitAABB() override;
	void InitCBData();
	void Update(ID3D12GraphicsCommandList* pCmdList);

protected:
	std::vector<std::shared_ptr<NXSubMeshBase>> m_pSubMeshes;
	NXBuffer<ConstantBufferObject>	m_cbObject;
};