#pragma once
#include "NXRenderableObject.h"
#include "NXSubMesh.h"
#include "NXConstantBuffer.h"

class NXQuadTree;
class NXTerrain : public NXRenderableObject
{
	friend class NXSubMeshGeometryEditor;
public:
	NXTerrain(int rawSize, int gridSize, int worldSize, const std::filesystem::path& rawFile);
	virtual ~NXTerrain() {}

	virtual NXTerrain* IsTerrain() { return this; }
	void AddSubMesh(NXSubMeshBase* pSubMesh);
	NXSubMeshBase* GetSubMesh() { return m_pSubMesh.get(); }

	void InitAABB() override;
	void Update(ID3D12GraphicsCommandList* pCmdList);

protected:
	std::filesystem::path m_rawPath;
	int m_rawSize;
	int m_gridSize;
	int m_worldSize;
	std::shared_ptr<NXSubMeshBase> m_pSubMesh;

	ConstantBufferObject m_cbDataObject;
	NXConstantBuffer<ConstantBufferObject>	m_cbObject;

private:
	std::vector<uint16_t> m_rawData;
	NXQuadTree* m_pQuadTree; 
};