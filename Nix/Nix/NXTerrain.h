#pragma once
#include "NXRenderableObject.h"
#include "NXSubMesh.h"
#include "NXConstantBuffer.h"
#include "NXTerrainLayer.h"
#include "NXTerrainCommon.h"

class NXTerrain : public NXRenderableObject
{
	friend class NXSubMeshGeometryEditor;
public:
	NXTerrain(int gridSize, int worldSize, NXTerrainNodeId terrainNodeId, int terrainId);
	virtual ~NXTerrain() {}

	NXTerrainLayer* GetTerrainLayer() const { return m_pTerrainLayer; }
	void SetTerrainLayer(NXTerrainLayer* pTerrainLayer);

	const NXTerrainNodeId& GetTerrainNode() const { return m_terrainNodeId; }

	virtual void SetRotation(const Vector3& value) override {} // 地形不可旋转
	virtual void SetScale(const Vector3& value) override {} // 地形不可缩放

	virtual NXTerrain* IsTerrain() { return this; }
	void AddSubMesh(NXSubMeshBase* pSubMesh);
	NXSubMeshBase* GetSubMesh() { return m_pSubMesh.get(); }
	bool RayCastPrimitive(const Ray& worldRay, NXHit& outHitInfo, float& outDist);

	void InitAABB() override;
	void Update(ID3D12GraphicsCommandList* pCmdList);

protected:
	NXTerrainNodeId m_terrainNodeId;
	int m_terrainId;
	int m_rawSize;
	int m_gridSize;
	int m_worldSize;

	NXTerrainLayer* m_pTerrainLayer;
	std::shared_ptr<NXSubMeshBase> m_pSubMesh; 

	ConstantBufferObject m_cbDataObject;
	NXConstantBuffer<ConstantBufferObject>	m_cbObject;
};