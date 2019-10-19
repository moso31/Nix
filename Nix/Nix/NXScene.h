#pragma once
#include "NXObject.h"

// temp include.
#include "ShaderStructures.h"

class Scene : public NXObject
{
public:
	Scene();
	~Scene();

	void OnMouseDown(NXEventArg eArg);

	void Init();
	void UpdateTransform(shared_ptr<NXObject> pObject = nullptr);
	void UpdateScripts();
	void UpdateCamera();
	void Release();

	BoundingSphere						GetBoundingSphere()		{ return m_boundingSphere; }
	AABB								GetAABB()				{ return m_aabb; }
	vector<shared_ptr<NXPrimitive>>		GetPrimitives()			{ return m_primitives; }

	// Ŀǰֻ�Ե�һ����Դ����Parallel ShadowMap��
	void GetShadowMapTransformInfo(ConstantBufferShadowMapTransform& out_cb);

	ID3D11Buffer* GetConstantBufferLights() const { return m_cbLights; }

private:
	void InitBoundingStructures();

	bool Intersect(const Ray& worldRay, shared_ptr<NXPrimitive>& outTarget, Vector3& outHitPosition, float& outDist);

private:
	friend SceneManager;
	shared_ptr<SceneManager> m_sceneManager;

	shared_ptr<NXObject> m_pRootObject;
	// object : light��camera��primitive������object��
	vector<shared_ptr<NXObject>> m_objects;
	vector<shared_ptr<NXLight>> m_lights;
	vector<shared_ptr<NXPrimitive>> m_primitives;
	vector<shared_ptr<NXMaterial>> m_materials;
	shared_ptr<NXCamera> m_mainCamera;

	ID3D11Buffer* m_cbLights;
	ConstantBufferLight m_cbDataLights;

	AABB m_aabb;
	BoundingSphere m_boundingSphere;
};
