#pragma once
#include "NXRendererPass.h"

class NXSubSurfaceRenderer : public NXRendererPass
{
public:
	NXSubSurfaceRenderer();
	virtual ~NXSubSurfaceRenderer();

	virtual void SetupInternal() override;
};
