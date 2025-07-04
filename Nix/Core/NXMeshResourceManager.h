#pragma once
#include "BaseDefs/NixCore.h"
#include "BaseDefs/Math.h"
#include "BaseDefs/DX12.h"
#include "NXResourceManagerBase.h"

struct NXSubMeshReloadTaskPackage
{
	void	Push(NXSubMeshBase* pSubMesh) { submits.push_back(pSubMesh); }
	bool	IsEmpty()	{ return submits.empty(); }
	std::vector<NXSubMeshBase*> submits; // 本次异步提交的SubMesh
};

enum NXPlaneAxis;
class NXSubMeshBase;
class NXTextureReloadTask;
class NXMeshResourceManager : public NXResourceManagerBase
{
public:
	NXMeshResourceManager() : m_pWorkingScene(nullptr) {}
	virtual ~NXMeshResourceManager() {}

	void Init(NXScene* pScene);

	NXPrimitive* CreateBox(const std::string& name, const float width, const float height, const float length, const Vector3& translation = Vector3(0.f), const Vector3& rotation = Vector3(0.f), const Vector3& scale = Vector3(1.f));
	NXPrimitive* CreateSphere(const std::string& name, const float radius, const UINT segmentHorizontal, const UINT segmentVertical, const Vector3& translation = Vector3(0.f), const Vector3& rotation = Vector3(0.f), const Vector3& scale = Vector3(1.f));
	NXPrimitive* CreateSHSphere(const std::string& name, const int l, const int m, const float radius, const UINT segmentHorizontal, const UINT segmentVertical, const Vector3& translation = Vector3(0.f), const Vector3& rotation = Vector3(0.f), const Vector3& scale = Vector3(1.f));
	NXPrimitive* CreateCylinder(const std::string& name, const float radius, const float length, const UINT segmentCircle, const UINT segmentLength, const Vector3& translation = Vector3(0.f), const Vector3& rotation = Vector3(0.f), const Vector3& scale = Vector3(1.f));
	NXPrimitive* CreatePlane(const std::string& name, const float width, const float height, const NXPlaneAxis axis, const Vector3& translation = Vector3(0.f), const Vector3& rotation = Vector3(0.f), const Vector3& scale = Vector3(1.f));
	NXPrefab* CreateFBXPrefab(const std::string& name, const std::string& filePath, bool bAutoCalcTangents = true);

	// 创建地形
	// gridSize: 地形网格大小分辨率（不需要控制lod，每个terrain自带四叉树）
	// worldSize: 实际尺寸大小
	NXTerrain* CreateTerrain(const std::string& name, int gridSize, int worldSize, int terrainNodeX, int terrainNodeY);
	NXTerrainLayer* CreateTerrainLayer();

	void BindMaterial(NXRenderableObject* pRenderableObj, NXMaterial* pMaterial);
	void BindMaterial(NXSubMeshBase* pSubMesh, NXMaterial* pMaterial);

	void OnReload() override;
	void Release() override;

	void AddReplacingSubMesh(NXSubMeshBase* pSubMesh) { m_replacingSubMeshes.push_back(pSubMesh); }

	NXTextureReloadTask LoadMaterialAsync(const NXSubMeshReloadTaskPackage* pTaskData);
	void LoadMaterialSync(const NXSubMeshReloadTaskPackage* pTaskData);
	void OnLoadMaterialFinish(const NXSubMeshReloadTaskPackage* pTaskData);

private:
	NXScene* m_pWorkingScene;

	std::vector<NXSubMeshBase*> m_replacingSubMeshes;

	// 地形自增id
	int m_terrainIncreaseId;

	// 地形层自增id（注意这俩id 概念上是互相独立的，不一定非得相同）
	int m_terrainLayerIncreaseId;

	// 生命周期暂时先由这里管理
	// TODO: 该放到哪？
	std::vector<NXTerrainLayer*> m_terrainLayers;
};