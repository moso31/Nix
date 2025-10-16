#pragma once
#include "NXSubMeshCommon.h"
#include "NXAllocatorManager.h"
#include "ShaderStructures.h"
#include "NXInstance.h"
#include "NXStructuredBuffer.h"

class NXPrefab;
class NXPrimitive;
class NXTerrain;
class NXSubMeshTerrain;
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
	void CreateTerrain(NXTerrain* pTerrain, int gridSize, int sectorSize);

	// Editor objects
	void CreateMoveArrows(NXPrimitive* pMesh);

	void CreateBuffers(std::vector<NXRawMeshView>& rawViews, const std::string& name);

	const NXMeshViews& GetMeshViews(const std::string& name);

	void Release();

private:
	void CreateTerrainSingleLod(NXTerrain* pTerrain, NXSubMeshTerrain* pSubMesh, int sectorSize, int lod);
	void InitCommonMeshes();

private:
	// Mesh data
	std::unordered_map<std::string, NXMeshViews*> m_data; 

	std::mutex m_mutex;

	NXSubMesh<float>* m_subMeshUnknown;
	NXSubMesh<VertexPT>* m_subMeshRT;
};
