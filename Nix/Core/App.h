#pragma once
#include "Renderer.h"

class App
{
public:
	App();

	void Init();
	void Draw();
	void Release();

private:
	std::shared_ptr<Renderer> m_pRenderer;
};