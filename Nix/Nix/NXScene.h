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
	void OnMouseMove(NXEventArgMouse eArg);
	void OnMouseUp(NXEventArgMouse eArg);
	void OnKeyDown(NXEventArgKey eArg);

private:
	Vector3 CastRayToEditorLine(const Ray& ray, const Ray& line) const;
	UINT m_bEditorSelectID;

public:

	void Init();
	void InitScripts();
	void UpdateTransform(NXObject* pObject = nullptr);
	void UpdateTransformOfEditorObjects();
	void UpdateScripts();
	void UpdateCamera();
	void UpdateLightData();
	void Release();

	// UI Picking
	void SetCurrentPickingSubMesh(NXSubMeshBase* pPickingObject = nullptr);
	NXSubMeshBase* GetCurrentPickingSubMesh() { return m_pPickingObject; }

	// RayCasts
	HBVHTree* GetBVHTree() { return m_pBVHTree; }
	bool RayCast(const Ray& ray, NXHit& out_hitInfo, float tMax = FLT_MAX);
	BoundingSphere	GetBoundingSphere() { return m_boundingSphere; }
	AABB GetAABB() { return m_aabb; }

	NXCamera* GetMainCamera() { return m_pMainCamera; }
	std::vector<NXRenderableObject*> GetRenderableObjects() { return m_renderableObjects; }
	std::vector<NXMaterial*> GetMaterials() { return m_materials; }
	std::vector<NXPBRLight*> GetPBRLights() { return m_pbrLights; }
	NXCubeMap* GetCubeMap() { return m_pCubeMap; }

	std::vector<NXPrimitive*> GetEditableObjects();

	// Ŀǰֻ�Ե�һ����Դ����Parallel ShadowMap��
	//void InitShadowMapTransformInfo(ConstantBufferShadowMapTransform& out_cb);

	ID3D11Buffer* GetConstantBufferLights() const { return m_cbLights.Get(); }

	// ���³���BVH��
	void BuildBVHTrees(const HBVHSplitMode SplitMode);
private:
	// ���ɱ༭������SelectionArrows�����⣩
	void InitEditorObjects();

	// ���㳡������������� AABB��
	void InitBoundingStructures();

private:
	friend class SceneManager;

	ComPtr<ID3D11Buffer> m_cbLights;
	ConstantBufferLight m_cbDataLights;

	AABB m_aabb;
	BoundingSphere m_boundingSphere;

	// ��ǰѡ�е�SubMesh��
	NXSubMeshBase* m_pPickingObject;

	// �󽻼��ٽṹ������NXRayTracer�����߼�⣩
	HBVHTree* m_pBVHTree;

	// ���صĸ�object
	NXObject* m_pRootObject;
	std::vector<NXObject*> m_objects;

	std::vector<NXRenderableObject*> m_renderableObjects;
	std::vector<NXMaterial*> m_materials;
	std::vector<NXPBRLight*> m_pbrLights;

	// ѡ�ж���2022.9.26 Ŀǰֻ֧�ֵ�ѡ����
	std::vector<NXPrimitive*> m_selectedObjects;

	NXEditorObjectManager* m_pEditorObjManager;

	NXCamera* m_pMainCamera;
	NXCubeMap* m_pCubeMap;
};
