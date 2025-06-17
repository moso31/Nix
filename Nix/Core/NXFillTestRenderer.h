#pragma once
#include "NXComputePass.h"

class NXFillTestRenderer : public NXComputePass
{
public:
	NXFillTestRenderer() {}
	virtual ~NXFillTestRenderer() {}

	virtual void SetupInternal() override;

private:
};

class NXGPUTerrainPatcherRenderer : public NXComputePass
{
public:
	NXGPUTerrainPatcherRenderer() {}
	virtual ~NXGPUTerrainPatcherRenderer() {}

	virtual void SetupInternal() override;
};
