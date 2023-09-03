#pragma once
#include "BaseDefs/Math.h"

class Renderer;
class DirectResources;
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

	void Release();

public:
	void SetViewSize(const Vector2& val) { m_viewSize = val; }

private:
	Renderer* m_pRenderer;
	DirectResources* m_pDXResources;

	Vector2 m_lastViewSize;
	Vector2 m_viewSize;
};