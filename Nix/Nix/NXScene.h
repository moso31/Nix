#pragma once
#include "NXObject.h"

// temp include.
#include "ShaderStructures.h"

class NXScene : public NXObject
{
public:
	NXScene();
	~NXScene();

	void OnMouseDown(NXEventArg eArg);

	void Init();
	void InitScripts();
	void UpdateTransform(shared_ptr<NXObject> pObject = nullptr);
	void UpdateScripts();
	void UpdateCamera();
	void Release();

	BoundingSphere						GetBoundingSphere()		{ return m_boundingSphere; }
	AABB								GetAABB()				{ return m_aabb; }
	vector<shared_ptr<NXPrimitive>>		GetPrimitives()			{ return m_primitives; }

	// 目前只对第一个光源创建Parallel ShadowMap。
	void InitShadowMapTransformInfo(ConstantBufferShadowMapTransform& out_cb);

	ID3D11Buffer* GetConstantBufferLights() const { return m_cbLights; }

private:
	void InitBoundingStructures();

private:
	friend SceneManager;
	shared_ptr<SceneManager> m_sceneManager;

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

	AABB m_aabb;
	BoundingSphere m_boundingSphere;
};
