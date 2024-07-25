#pragma once
#include "NXRendererPass.h"

class NXShadowMapRenderer;
class NXShadowTestRenderer : public NXRendererPass
{
public:
	NXShadowTestRenderer() = default;
	virtual ~NXShadowTestRenderer() {};
};
