#pragma once
#include "NXObject.h"
#include "NXMaterial.h"
#include "NXPBRMaterial.h"
#include "NXPBRLight.h"
#include "HBVH.h"

// temp include.
#include "ShaderStructures.h"

class NXScene : public NXObject
{
public:
	NXScene();
	~NXScene();

	void OnMouseDown(NXEventArg eArg);
	void OnKeyDown(NXEventArg eArg);

	void Init();
	void InitScripts();
	void UpdateTransform(shared_ptr<NXObject> pObject = nullptr);
	void UpdateScripts();
	void UpdateCamera();
	void Release();

	// 场景-射线相交测试
	bool RayCast(const Ray& ray, NXHit& out_hitInfo);

	BoundingSphere						GetBoundingSphere()		{ return m_boundingSphere; }
	AABB								GetAABB()				{ return m_aabb; }
	vector<shared_ptr<NXPrimitive>>		GetPrimitives()			{ return m_primitives; }

	// PBR场景数据
	vector<shared_ptr<NXPBRPointLight>>	GetPBRLights()			{ return m_pbrLights; }
	vector<shared_ptr<NXPBRMaterial>>	GetPBRMaterials()		{ return m_pbrMaterials; }

	// 目前只对第一个光源创建Parallel ShadowMap。
	void InitShadowMapTransformInfo(ConstantBufferShadowMapTransform& out_cb);

	ID3D11Buffer* GetConstantBufferLights() const { return m_cbLights; }

private:
	void InitBoundingStructures();
	void CreateBVHTrees(const HBVHSplitMode SplitMode);

private:
	friend SceneManager;
	shared_ptr<SceneManager> m_sceneManager;

	// 求交加速结构（用于NXRayTracer的射线检测）
	shared_ptr<HBVHTree> m_pBVHTree;

	// 隐藏的根object
	shared_ptr<NXObject> m_pRootObject;

	// object : light、camera、primitive均属于object。
	vector<shared_ptr<NXObject>> m_objects;
	vector<shared_ptr<NXLight>> m_lights;
	vector<shared_ptr<NXPrimitive>> m_primitives;
	shared_ptr<NXCamera> m_mainCamera;

	// 材质
	vector<shared_ptr<NXMaterial>> m_materials;

	ID3D11Buffer* m_cbLights;
	ConstantBufferLight m_cbDataLights;
	ID3D11Texture2D* m_pCubeMap;

	AABB m_aabb;
	BoundingSphere m_boundingSphere;

	// PBR材质
	vector<shared_ptr<NXPBRPointLight>> m_pbrLights;
	vector<shared_ptr<NXPBRMaterial>> m_pbrMaterials;
};
