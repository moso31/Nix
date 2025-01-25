#pragma once
#include "Renderer.h"

class NXGUISSAO
{
public:
	NXGUISSAO(Renderer* pRenderer);
	virtual ~NXGUISSAO() {}

	void Render();

private:
	Renderer* m_pRenderer;
	bool m_bDirty;
};