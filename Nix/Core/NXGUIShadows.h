#pragma once
#include "Renderer.h"

class NXGUIShadows
{
public:
	NXGUIShadows(Renderer* pRenderer) : m_pRenderer(pRenderer) {}
	virtual ~NXGUIShadows() {}

	void Render();

private:
	Renderer* m_pRenderer;
};