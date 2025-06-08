#pragma once
#include "NXGraphicPass.h"
#include "NXConstantBuffer.h"

class NXDebugLayerRenderer : public NXGraphicPass
{
public:
	NXDebugLayerRenderer();

	virtual void SetupInternal() override;

private:
};
