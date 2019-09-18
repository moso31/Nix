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
	void Update();
	void Render();
	void Release();

	BoundingSphere GetBoundingSphere() { return m_boundingSphere; }
	AABB GetAABB() { return m_aabb; }
	vector<shared_ptr<NXPrimitive>> GetPrimitives() { return m_primitives; }
	vector<shared_ptr<NXPrimitive>> GetBlendingPrimitives() { return m_blendingPrimitives; }

private:
	void InitBoundingStructures();

	// 目前只对第一个光源创建Parallel ShadowMap。
	void InitShadowMap();

	bool Intersect(const Ray& worldRay, _Out_ shared_ptr<NXPrimitive>& outTarget, _Out_ Vector3& outHitPosition, _Out_ float& outDist);

private:
	vector<shared_ptr<NXLight>> m_lights;
	vector<shared_ptr<NXMaterial>> m_materials;
	vector<shared_ptr<NXPrimitive>> m_primitives;
	vector<shared_ptr<NXPrimitive>> m_blendingPrimitives;
	shared_ptr<NXCamera> m_mainCamera;

	shared_ptr<NXShadowMap>		m_pShadowMap;

	ID3D11Buffer* m_cbLights;
	ConstantBufferLight m_cbDataLights;

	AABB m_aabb;
	BoundingSphere m_boundingSphere;
};
