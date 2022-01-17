#pragma once
#include "SceneManager.h"
#include "NXEvent.h"

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
	HBVHTree* GetBVHTree() { return m_sceneManager->m_pBVHTree; }
	bool RayCast(const Ray& ray, NXHit& out_hitInfo, float tMax = FLT_MAX);
	BoundingSphere	GetBoundingSphere() { return m_boundingSphere; }
	AABB GetAABB() { return m_aabb; }

	NXCamera* GetMainCamera() { return m_sceneManager->m_pMainCamera; }
	std::vector<NXPrimitive*> GetPrimitives() { return m_sceneManager->m_primitives; }
	std::vector<NXPBRLight*> GetPBRLights() { return m_sceneManager->m_pbrLights; }
	NXCubeMap* GetCubeMap() { return m_sceneManager->m_pCubeMap; }

	// Ŀǰֻ�Ե�һ����Դ����Parallel ShadowMap��
	void InitShadowMapTransformInfo(ConstantBufferShadowMapTransform& out_cb);

	ID3D11Buffer* GetConstantBufferLights() const { return m_cbLights.Get(); }

	// ���³���BVH��
	void BuildBVHTrees(const HBVHSplitMode SplitMode);
private:
	void InitBoundingStructures();

private:
	SceneManager* m_sceneManager;

	ComPtr<ID3D11Buffer> m_cbLights;
	ConstantBufferLight m_cbDataLights;

	AABB m_aabb;
	BoundingSphere m_boundingSphere;

	// ָ��ǰѡ�������ָ�롣
	NXSubMesh* m_pPickingObject;
};
