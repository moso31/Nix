#pragma once
#include "NXRenderPass.h"
#include "NXPassMaterial.h"

class NXComputePass : public NXRenderPass
{
public:
	NXComputePass();
	virtual ~NXComputePass() {}

	virtual void SetupInternal() = 0;

	NXComputePassMaterial* GetPassMaterial() { return m_pMaterial; }
	void SetPassMaterial(NXComputePassMaterial* pMaterial) { m_pMaterial = pMaterial; }

private:
	NXComputePassMaterial* m_pMaterial;
};