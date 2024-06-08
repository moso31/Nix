#pragma once
#include "NXRendererPass.h"

class NXTexture2D;
class NXDepthRenderer : public NXRendererPass
{
public:
	NXDepthRenderer() {}
	~NXDepthRenderer() {}

	void Init();
};
