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
	void OnKeyUp(NXEventArgKey eArg);
	void OnKeyUpForce(NXEventArgKey eArg);

private:
	// �������ߺ�EditorObject�ϵ�ê�㡣ƽ�ơ���ת�����ŵȲ������������ê�㡣
	Vector3 GetAnchorOfEditorObject(const Ray& worldRay);
	Vector3 GetAnchorOfEditorTranslatorLine(const Ray& ray, const Ray& line) const;
	Vector3 GetAnchorOfEditorTranslatorPlane(const Ray& ray, const Plane& plane) const;
	EditorObjectID m_bEditorSelectID;
	// ��¼�϶�EditorObject��MouseDownʱ������ѡ�������λ����Ϣ��ʵ�ʴ��������Щλ�þ���Anchor��ƫ������
	std::vector<Vector3> m_selectObjHitOffset;

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
	void AddPickingSubMesh(NXSubMeshBase* pPickingObject = nullptr);
	std::vector<NXSubMeshBase*> GetPickingSubMeshes() const { return m_pSelectedSubMeshes; }

	// RayCasts
	HBVHTree* GetBVHTree() { return m_pBVHTree; }
	bool RayCast(const Ray& ray, NXHit& out_hitInfo, float tMax = FLT_MAX);
	BoundingSphere	GetBoundingSphere() { return m_boundingSphere; }
	AABB GetAABB() { return m_aabb; }

	NXCamera* GetMainCamera() { return m_pMainCamera; }
	std::vector<NXRenderableObject*> GetRenderableObjects() { return m_renderableObjects; }	
	std::vector<NXPBRLight*> GetPBRLights() { return m_pbrLights; }
	NXCubeMap* GetCubeMap() { return m_pCubeMap; }

	NXEditorObjectManager* GetEditorObjManager() { return m_pEditorObjManager; }

	// Ŀǰֻ�Ե�һ����Դ����Parallel ShadowMap��
	//void InitShadowMapTransformInfo(ConstantBufferShadowMapTransform& out_cb);

	ID3D11Buffer* GetConstantBufferLights() const { return m_cbLights.Get(); }

	// ���³���BVH��
	void BuildBVHTrees(const HBVHSplitMode SplitMode);

	void RegisterCubeMap(NXCubeMap* newCubeMap);
	void RegisterPrimitive(NXPrimitive* newPrimitive, NXObject* pParent = nullptr);
	void RegisterPrefab(NXPrefab* newPrefab, NXObject* pParent = nullptr);
	void RegisterCamera(NXCamera* newCamera, bool isMainCamera, NXObject* pParent = nullptr);
	void RegisterLight(NXPBRLight* newLight, NXObject* pParent = nullptr);

private:
	// ���ɱ༭������MoveArrows�����⣩
	void InitEditorObjectsManager();

	// ���㳡������������� AABB��
	void InitBoundingStructures();

private:
	ComPtr<ID3D11Buffer> m_cbLights;
	ConstantBufferLight m_cbDataLights;

	AABB m_aabb;
	BoundingSphere m_boundingSphere;

	// �󽻼��ٽṹ������NXRayTracer�����߼�⣩
	HBVHTree* m_pBVHTree;

	// ���صĸ�object
	NXObject* m_pRootObject;
	std::vector<NXObject*> m_objects;

	std::vector<NXRenderableObject*> m_renderableObjects;
	std::vector<NXPBRLight*> m_pbrLights;

	// ��ǰѡ�е�SubMesh�Ͷ�Ӧ��Objects
	std::vector<NXSubMeshBase*> m_pSelectedSubMeshes;
	std::vector<NXPrimitive*> m_pSelectedObjects; 
	bool m_bMultiSelectKeyHolding; // �Ƿ��ڶ�ѡ״̬(LCtrl)

	NXEditorObjectManager* m_pEditorObjManager;

	NXCamera* m_pMainCamera;
	NXCubeMap* m_pCubeMap;
};

