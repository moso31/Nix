#pragma once
#include "NXGraphicPass.h"

class NXSubSurfaceRenderer : public NXGraphicPass
{
public:
	NXSubSurfaceRenderer();
	virtual ~NXSubSurfaceRenderer();

	virtual void SetupInternal() override;
};
