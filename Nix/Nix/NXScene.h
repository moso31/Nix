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
	// 计算射线和EditorObject上的锚点。平移、旋转、缩放等操作都依赖这个锚点。
	Vector3 GetAnchorOfEditorObject(const Ray& worldRay);
	Vector3 GetAnchorOfEditorTranslatorLine(const Ray& ray, const Ray& line) const;
	Vector3 GetAnchorOfEditorTranslatorPlane(const Ray& ray, const Plane& plane) const;
	EditorObjectID m_bEditorSelectID;
	// 记录拖动EditorObject并MouseDown时，所有选中物体的位置信息（实际储存的是这些位置距离Anchor的偏移量）
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

	// 目前只对第一个光源创建Parallel ShadowMap。
	//void InitShadowMapTransformInfo(ConstantBufferShadowMapTransform& out_cb);

	ID3D11Buffer* GetConstantBufferLights() const { return m_cbLights.Get(); }

	// 更新场景BVH树
	void BuildBVHTrees(const HBVHSplitMode SplitMode);

	void RegisterCubeMap(NXCubeMap* newCubeMap);
	void RegisterPrimitive(NXPrimitive* newPrimitive, NXObject* pParent = nullptr);
	void RegisterPrefab(NXPrefab* newPrefab, NXObject* pParent = nullptr);
	void RegisterCamera(NXCamera* newCamera, bool isMainCamera, NXObject* pParent = nullptr);
	void RegisterLight(NXPBRLight* newLight, NXObject* pParent = nullptr);

private:
	// 生成编辑器对象（MoveArrows等玩意）
	void InitEditorObjectsManager();

	// 计算场景下所有物体的 AABB。
	void InitBoundingStructures();

private:
	ComPtr<ID3D11Buffer> m_cbLights;
	ConstantBufferLight m_cbDataLights;

	AABB m_aabb;
	BoundingSphere m_boundingSphere;

	// 求交加速结构（用于NXRayTracer的射线检测）
	HBVHTree* m_pBVHTree;

	// 隐藏的根object
	NXObject* m_pRootObject;
	std::vector<NXObject*> m_objects;

	std::vector<NXRenderableObject*> m_renderableObjects;
	std::vector<NXPBRLight*> m_pbrLights;

	// 当前选中的SubMesh和对应的Objects
	std::vector<NXSubMeshBase*> m_pSelectedSubMeshes;
	std::vector<NXPrimitive*> m_pSelectedObjects; 
	bool m_bMultiSelectKeyHolding; // 是否处于多选状态(LCtrl)

	NXEditorObjectManager* m_pEditorObjManager;

	NXCamera* m_pMainCamera;
	NXCubeMap* m_pCubeMap;
};

