#pragma once
#include "NXRendererPass.h"

class NXDepthRenderer : public NXRendererPass
{
public:
	NXDepthRenderer() {}
	virtual ~NXDepthRenderer() {}

	void Init();
};
