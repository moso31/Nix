#pragma once
#include "NXComputePass.h"

class NXVTReadbackRenderer : public NXComputePass
{
public:
	NXVTReadbackRenderer() {}
	virtual ~NXVTReadbackRenderer() {}

	virtual void SetupInternal() override;

private:
};
