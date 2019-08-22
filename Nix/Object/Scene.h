#pragma once
#include "Header.h"

// temp include.
#include "ShaderStructures.h"

class Scene
{
public:
	Scene();
	~Scene();

	void Init();
	void Update();
	void Render();
	void Release();

private:
	vector<shared_ptr<DirectionalLight>> m_lights;
	vector<shared_ptr<Material>> m_materials;
	vector<shared_ptr<NXPrimitive>> m_primitives;
	shared_ptr<NXCamera> m_mainCamera;

	ID3D11Buffer* m_cbLights;
	ConstantBufferLight m_cbDataLights;
};
