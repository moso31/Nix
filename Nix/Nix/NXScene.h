#pragma once
#include "Header.h"

// temp include.
#include "ShaderStructures.h"

class Scene : public enable_shared_from_this<Scene>
{
public:
	Scene();
	~Scene();

	void Init();
	void PrevUpdate();
	void Update();
	void Render();
	void Release();

	vector<shared_ptr<NXScript>> GetScripts() { return m_scripts; }

private:
	vector<shared_ptr<NXLight>> m_lights;
	vector<shared_ptr<NXMaterial>> m_materials;
	vector<shared_ptr<NXPrimitive>> m_primitives;
	vector<shared_ptr<NXScript>> m_scripts;
	shared_ptr<NXCamera> m_mainCamera;

	shared_ptr<SceneManager> m_sceneManager;

	ID3D11Buffer* m_cbLights;
	ConstantBufferLight m_cbDataLights;
};
