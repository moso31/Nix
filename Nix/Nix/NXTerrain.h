#pragma once
#include "NXRenderableObject.h"
#include "NXSubMesh.h"
#include "NXConstantBuffer.h"

class NXQuadTree;
class NXTerrain : public NXRenderableObject
{
	friend class NXSubMeshGeometryEditor;
public:
	NXTerrain(int gridSize, int worldSize);
	virtual ~NXTerrain() {}

	virtual NXTerrain* IsTerrain() { return this; }
	void AddSubMesh(NXSubMeshBase* pSubMesh);
	NXSubMeshBase* GetSubMesh() { return m_pSubMesh.get(); }
	bool RayCastPrimitive(const Ray& worldRay, NXHit& outHitInfo, float& outDist);

	void SetHeightRange(const Vector2& heightRange) { m_heightRange = heightRange; }

	void InitAABB() override;
	void Update(ID3D12GraphicsCommandList* pCmdList);

protected:
	int m_rawSize;
	int m_gridSize;
	int m_worldSize;
	Vector2 m_heightRange;
	std::shared_ptr<NXSubMeshBase> m_pSubMesh; 

	ConstantBufferObject m_cbDataObject;
	NXConstantBuffer<ConstantBufferObject>	m_cbObject;

private:
	NXQuadTree* m_pQuadTree; 

	bool m_bGenerated = false;
};