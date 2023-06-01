#pragma once
#include "Renderer.h"

class App
{
public:
	App();

	void Init();
	void OnResize(UINT width, UINT height);
	void Reload();
	void Update();
	void Draw();
	void ReleaseUnusedTextures();

	void Release();

private:
	Renderer* m_pRenderer;
	DirectResources* m_pDXResources;
};