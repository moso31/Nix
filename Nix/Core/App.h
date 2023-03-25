#pragma once
#include "Renderer.h"

class App
{
public:
	App();

	void Init();
	void Reload();
	void Update();
	void Draw();
	void Release();

private:
	Renderer* m_pRenderer;
};