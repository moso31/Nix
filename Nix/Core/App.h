#pragma once
#include "Renderer.h"

class App
{
public:
	App();

	void Init();
	void OnWindowResize(UINT width, UINT height);
	void OnResize(const Vector2& rtSize);
	void ResizeCheck();
	void Reload();
	void Update();
	void Draw();
	void ReleaseUnusedTextures();

	void Release();

public:
	void SetViewSize(const Vector2& val) { m_viewSize = val; }

private:
	Renderer* m_pRenderer;
	DirectResources* m_pDXResources;

	Vector2 m_lastViewSize;
	Vector2 m_viewSize;
};