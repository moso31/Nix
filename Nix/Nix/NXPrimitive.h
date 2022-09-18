#pragma once
#include "NXRenderableObject.h"
#include "NXSubMesh.h"

class NXPrimitive : public NXRenderableObject
{
public:
	NXPrimitive();
	~NXPrimitive() {}

	virtual NXPrimitive* IsPrimitive() override { return this; }

	virtual void UpdateViewParams();

	// 自动计算SubMesh下所有顶点的切线数据。
	void CalculateTangents(bool bUpdateVertexIndexBuffer = false);

	bool RayCastPrimitive(const Ray& worldRay, NXHit& outHitInfo, float& outDist);

	UINT GetSubMeshCount() { return (UINT)m_pSubMeshes.size(); }
	NXSubMesh* GetSubMesh(UINT index) { return m_pSubMeshes[index].get(); }

	UINT GetFaceCount();

	void ClearSubMeshes();
	void AddSubMesh(NXSubMesh* pSubMesh);
	void ResizeSubMesh(UINT size);
	void ReloadSubMesh(UINT index, NXSubMesh* pSubMesh);

	void InitAABB() override;

protected:
	std::vector<std::shared_ptr<NXSubMesh>> m_pSubMeshes;
	std::vector<Vector3> m_points;	// vertices position 序列
};