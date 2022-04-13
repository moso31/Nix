#pragma once
#include "header.h"
#include "NXEvent.h"

#include "NXObject.h"
#include "HBVH.h"
#include "ShaderStructures.h"

class NXScene : public NXObject
{
public:
	NXScene();
	~NXScene();

	void OnMouseDown(NXEventArgMouse eArg);
	void OnKeyDown(NXEventArgKey eArg);

	void Init();
	void InitScripts();
	void UpdateTransform(NXObject* pObject = nullptr);
	void UpdateScripts();
	void UpdateCamera();
	void UpdateLightData();
	void Release();

	// UI Picking
	void SetCurrentPickingSubMesh(NXSubMesh* pPickingObject = nullptr) { m_pPickingObject = pPickingObject; }
	NXSubMesh* GetCurrentPickingSubMesh() { return m_pPickingObject; }

	// RayCasts
	HBVHTree* GetBVHTree() { return m_pBVHTree; }
	bool RayCast(const Ray& ray, NXHit& out_hitInfo, float tMax = FLT_MAX);
	BoundingSphere	GetBoundingSphere() { return m_boundingSphere; }
	AABB GetAABB() { return m_aabb; }

	NXCamera* GetMainCamera() { return m_pMainCamera; }
	std::vector<NXPrimitive*> GetPrimitives() { return m_primitives; }
	std::vector<NXMaterial*> GetMaterials() { return m_materials; }
	std::vector<NXPBRLight*> GetPBRLights() { return m_pbrLights; }
	NXCubeMap* GetCubeMap() { return m_pCubeMap; }

	// 目前只对第一个光源创建Parallel ShadowMap。
	//void InitShadowMapTransformInfo(ConstantBufferShadowMapTransform& out_cb);

	ID3D11Buffer* GetConstantBufferLights() const { return m_cbLights.Get(); }

	// 更新场景BVH树
	void BuildBVHTrees(const HBVHSplitMode SplitMode);
private:
	void InitBoundingStructures();

private:
	friend class SceneManager;

	ComPtr<ID3D11Buffer> m_cbLights;
	ConstantBufferLight m_cbDataLights;

	AABB m_aabb;
	BoundingSphere m_boundingSphere;

	// 指向当前选中物体的指针。
	NXSubMesh* m_pPickingObject;

	// 求交加速结构（用于NXRayTracer的射线检测）
	HBVHTree* m_pBVHTree;

	// 隐藏的根object
	NXObject* m_pRootObject;
	std::vector<NXObject*> m_objects;
	//std::vector<NXScript*> m_scripts;

	std::vector<NXPrimitive*> m_primitives;
	std::vector<NXMaterial*> m_materials;
	std::vector<NXPBRLight*> m_pbrLights;

	NXCamera* m_pMainCamera;
	NXCubeMap* m_pCubeMap;
};
