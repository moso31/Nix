#pragma once
#include "Renderer.h"

class App
{
public:
	App();

	void Init();
	void Update();
	void Render();
	void Release();

private:
	shared_ptr<Renderer> m_pRenderer;
};