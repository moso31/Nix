#pragma once
#include "NXTransform.h"
#include "NXSubMesh.h"

class NXPrimitive : public NXTransform
{
public:
	NXPrimitive();
	~NXPrimitive() {}

	virtual void UpdateViewParams();
	virtual void Release();

	// �Զ�����SubMesh�����ж�����������ݡ�
	void CalculateTangents(bool bUpdateVertexIndexBuffer = false);

	AABB GetAABBWorld();
	AABB GetAABBLocal() const;

	virtual bool RayCast(const Ray& worldRay, NXHit& outHitInfo, float& outDist);

	UINT GetSubMeshCount() { return (UINT)m_pSubMeshes.size(); }
	NXSubMesh* GetSubMesh(UINT index) { return m_pSubMeshes[index]; }

	UINT GetFaceCount();

protected:
	void InitAABB();

protected:
	std::vector<NXSubMesh*> m_pSubMeshes;
	std::vector<Vector3> m_points;	// vertices position ����
	AABB m_aabb;
};