#pragma once
#include "NXRGPass.h"

class NXReadbackBufferPass : public NXRGPass
{
public:
	NXReadbackBufferPass();
	virtual ~NXReadbackBufferPass() {}

	virtual void SetupInternal() override {}

	virtual void Render(ID3D12GraphicsCommandList* pCmdList) override {}
};