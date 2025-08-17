#pragma once
#include "NXRGPass.h"
#include "NXRGResource.h"

class NXReadbackBufferPass : public NXRGPass
{
public:
	NXReadbackBufferPass();
	virtual ~NXReadbackBufferPass() {}

	virtual void SetupInternal() override {}
	virtual void Render(ID3D12GraphicsCommandList* pCmdList) override;

	void SetReadbackBuffer(NXRGResource* pRes) { m_pReadbackBuffer = pRes; }

private:
	NXRGResource* m_pReadbackBuffer;
};