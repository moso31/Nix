#pragma once
#include "NXGraphicPass.h"

class NXShadowMapRenderer;
class NXShadowTestRenderer : public NXGraphicPass
{
public:
	NXShadowTestRenderer() = default;
	virtual ~NXShadowTestRenderer() {};

	virtual void SetupInternal() override;
};
