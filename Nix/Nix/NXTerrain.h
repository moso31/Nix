#pragma once
#include "NXRenderableObject.h"
#include "NXSubMesh.h"
#include "NXConstantBuffer.h"
#include "NXTerrainLayer.h"

struct NXGPUTerrainBlockData
{
	int x;
	int y;
};

class NXTerrain : public NXRenderableObject
{
	friend class NXSubMeshGeometryEditor;
public:
	NXTerrain(int gridSize, int worldSize, int terrainId);
	virtual ~NXTerrain() {}

	NXTerrainLayer* GetTerrainLayer() const { return m_pTerrainLayer; }
	void SetTerrainLayer(NXTerrainLayer* pTerrainLayer);

	virtual void SetRotation(const Vector3& value) override {} // 地形不可旋转
	virtual void SetScale(const Vector3& value) override {} // 地形不可缩放

	virtual NXTerrain* IsTerrain() { return this; }
	void AddSubMesh(NXSubMeshBase* pSubMesh);
	NXSubMeshBase* GetSubMesh() { return m_pSubMesh.get(); }
	bool RayCastPrimitive(const Ray& worldRay, NXHit& outHitInfo, float& outDist);

	void InitAABB() override;
	void Update(ID3D12GraphicsCommandList* pCmdList);

protected:
	int m_terrainId;
	int m_rawSize;
	int m_gridSize;
	int m_worldSize;

	NXTerrainLayer* m_pTerrainLayer;
	std::shared_ptr<NXSubMeshBase> m_pSubMesh; 

	ConstantBufferObject m_cbDataObject;
	NXConstantBuffer<ConstantBufferObject>	m_cbObject;

private:
	bool m_bGenerated = false;
};