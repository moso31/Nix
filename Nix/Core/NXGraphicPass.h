#pragma once
#include "NXRenderPass.h"
#include "NXPassMaterial.h"

class NXGraphicPass : public NXRenderPass
{
public:
	NXGraphicPass();
	virtual ~NXGraphicPass() {}

	virtual void SetupInternal() = 0;

	NXGraphicPassMaterial* GetMaterial() { return m_pMaterial; }
	void SetMaterial(NXGraphicPassMaterial* pMaterial) { m_pMaterial = pMaterial; }

private:
	NXGraphicPassMaterial* m_pMaterial;
};
