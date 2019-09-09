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

	AABB GetAABB() { return m_aabb; }
	vector<shared_ptr<NXPrimitive>> GetPrimitives() { return m_primitives; }

private:
	void InitAABB();
	bool Intersect(const Ray& worldRay, _Out_ shared_ptr<NXPrimitive>& outTarget, _Out_ Vector3& outHitPosition, _Out_ float& outDist);

private:
	vector<shared_ptr<NXLight>> m_lights;
	vector<shared_ptr<NXMaterial>> m_materials;
	vector<shared_ptr<NXPrimitive>> m_primitives;
	vector<shared_ptr<NXPrimitive>> m_blendingPrimitives;
	shared_ptr<NXCamera> m_mainCamera;

	ID3D11Buffer* m_cbLights;
	ConstantBufferLight m_cbDataLights;

	AABB m_aabb;
};
