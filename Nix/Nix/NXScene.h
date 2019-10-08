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
	void PrevUpdate();
	void UpdateScripts();
	void UpdateCamera();
	void Release();

	BoundingSphere						GetBoundingSphere()		{ return m_boundingSphere; }
	AABB								GetAABB()				{ return m_aabb; }
	vector<shared_ptr<NXPrimitive>>		GetPrimitives()			{ return m_primitives; }

	// 目前只对第一个光源创建Parallel ShadowMap。
	void GetShadowMapTransformInfo(ConstantBufferShadowMapCamera& out_cb);

	ID3D11Buffer* GetConstantBufferLights() const { return m_cbLights; }

private:
	void InitBoundingStructures();

	bool Intersect(const Ray& worldRay, shared_ptr<NXPrimitive>& outTarget, Vector3& outHitPosition, float& outDist);

private:
	vector<shared_ptr<NXLight>> m_lights;
	vector<shared_ptr<NXMaterial>> m_materials;
	vector<shared_ptr<NXPrimitive>> m_primitives;
	shared_ptr<NXCamera> m_mainCamera;

	ID3D11Buffer* m_cbLights;
	ConstantBufferLight m_cbDataLights;

	AABB m_aabb;
	BoundingSphere m_boundingSphere;
};
