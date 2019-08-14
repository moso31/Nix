#pragma once
#include "Header.h"

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
	vector<shared_ptr<Box>> m_primitives;
	shared_ptr<Camera> m_mainCamera;
};
